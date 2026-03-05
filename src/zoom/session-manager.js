const EventEmitter = require('events');
const { generateJWT } = require('./jwt-generator');
const { getLocalRecordingToken } = require('./recording-token');

class SessionManager extends EventEmitter {
  constructor(settings) {
    super();
    this.settings = settings;
    this.participants = new Map();
    this.connected = false;
    this.isoIndexPool = [];
    this.nextIsoIndex = 1;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 5;
    this.reconnectDelay = 3000;
    this.reconnectTimer = null;
    this.sessionName = settings.sessionName;
    this.meetingId = '';
    this.meetingPassword = '';
    this.zoomBridge = null;
    this.streamHandler = null;

    this._initZoomBridge();
  }

  _initZoomBridge() {
    try {
      const { ZoomMeetingBridge } = require('../../zoom-meeting-sdk-addon');
      this.zoomBridge = new ZoomMeetingBridge();

      const ok = this.zoomBridge.init({
        sdkKey: this.settings.zoomSdkKey,
        sdkSecret: this.settings.zoomSdkSecret,
        botName: this.settings.botName || 'PlexISO',
      });

      if (ok) {
        console.log('[SessionManager] SDK initialized, authenticating...');
        this.zoomBridge.auth();
      } else {
        console.error('[SessionManager] SDK initialization failed');
      }

      this.zoomBridge.on('participant-joined', ({ userId, displayName, isSelf }) => {
        if (isSelf) {
          console.log(`[SessionManager] Skipping self (bot) participant: ${displayName} (${userId})`);
          return;
        }
        this.addParticipant({ userId, displayName });
      });

      this.zoomBridge.on('participant-left', ({ userId }) => {
        this.removeParticipant(userId);
      });

      this.zoomBridge.on('video-frame', ({ userId, frameData }) => {
        if (this.streamHandler) {
          this.streamHandler.handleVideoFrame(userId, frameData);
        }
      });

      this.zoomBridge.on('audio-frame', ({ userId, audioData }) => {
        if (this.streamHandler) {
          this.streamHandler.handleAudioData(userId, audioData);
        }
      });

      this.zoomBridge.on('meeting-joined', () => {
        this.setConnected(true);
      });

      this.zoomBridge.on('meeting-ended', ({ status }) => {
        this.setConnected(false);
        this.emit('connection-error', new Error('Meeting ended: ' + status));
      });

      this.zoomBridge.on('meeting-reconnecting', () => {
        this.emit('reconnecting', { attempt: 1, maxAttempts: this.maxReconnectAttempts });
      });

      this.zoomBridge.on('raw-recording-started', () => {
        console.log('[SessionManager] Raw recording started — capturing video/audio');
        this.emit('raw-recording-started');
      });

      this.zoomBridge.on('auth-success', () => {
        console.log('[SessionManager] SDK authentication successful');
        this.emit('auth-success');
      });

      this.zoomBridge.on('auth-failed', () => {
        console.error('[SessionManager] SDK authentication FAILED');
        this.emit('connection-error', new Error('Zoom SDK authentication failed'));
      });

      console.log('[SessionManager] Zoom Meeting SDK bridge initialized, available:', this.zoomBridge.isAvailable());
    } catch (err) {
      console.warn('[SessionManager] Zoom Meeting SDK bridge not available:', err.message);
      this.zoomBridge = null;
    }
  }

  setStreamHandler(handler) {
    this.streamHandler = handler;
  }

  isZoomAvailable() {
    return this.zoomBridge !== null && this.zoomBridge.isAvailable();
  }

  _allocateIsoIndex() {
    if (this.isoIndexPool.length > 0) {
      return this.isoIndexPool.sort((a, b) => a - b).shift();
    }
    return this.nextIsoIndex++;
  }

  _releaseIsoIndex(index) {
    if (index && !this.isoIndexPool.includes(index)) {
      this.isoIndexPool.push(index);
    }
  }

  generateToken(role = 1) {
    return generateJWT(
      this.settings.zoomSdkKey,
      this.settings.zoomSdkSecret,
      this.sessionName,
      role,
      this.settings.sessionPassword
    );
  }

  async joinMeeting(meetingId, password, displayName) {
    if (!this.zoomBridge) {
      throw new Error('Zoom Meeting SDK not available. Build the native addon first.');
    }

    if (!this.zoomBridge.authenticated) {
      throw new Error('SDK not authenticated yet. Wait for authentication to complete before joining.');
    }

    this.meetingId = meetingId;
    this.meetingPassword = password || '';

    let appPrivilegeToken = '';
    const { zoomAccountId, zoomClientId, zoomClientSecret } = this.settings;
    if (zoomAccountId && zoomClientId && zoomClientSecret) {
      try {
        appPrivilegeToken = await getLocalRecordingToken(
          meetingId, zoomAccountId, zoomClientId, zoomClientSecret
        );
        console.log('[SessionManager] Local recording privilege token obtained');
      } catch (err) {
        console.warn('[SessionManager] Could not get local recording token:', err.message);
        console.warn('[SessionManager] Joining without pre-authorized recording — host may need to grant permission');
      }
    } else {
      console.log('[SessionManager] No ZOOM_ACCOUNT_ID/CLIENT_ID/CLIENT_SECRET — joining without recording token');
    }

    console.log('[SessionManager] Attempting to join meeting:', meetingId);

    const ok = this.zoomBridge.joinMeeting(
      meetingId,
      password || '',
      displayName || this.settings.botName || 'PlexISO',
      appPrivilegeToken
    );

    if (!ok) {
      console.error('[SessionManager] joinMeeting() returned false — SDK state:', this.zoomBridge.getState());
    }

    return ok;
  }

  leaveMeeting() {
    if (this.zoomBridge) {
      this.zoomBridge.leaveMeeting();
    }

    for (const [userId] of this.participants) {
      this.removeParticipant(userId);
    }

    this.setConnected(false);
  }

  getParticipants() {
    return Array.from(this.participants.values());
  }

  getParticipant(userId) {
    return this.participants.get(userId);
  }

  addParticipant(user) {
    if (this.participants.has(user.userId)) {
      return this.participants.get(user.userId);
    }

    const participant = {
      userId: user.userId,
      displayName: user.displayName || `Participant_${user.userId}`,
      isVideoOn: false,
      isAudioOn: false,
      joinedAt: Date.now(),
      isoIndex: this._allocateIsoIndex(),
    };
    this.participants.set(user.userId, participant);

    if (this.streamHandler) {
      this.streamHandler.startVideoCapture(user.userId, participant.displayName);
      this.streamHandler.startAudioCapture(user.userId, participant.displayName);
    }

    this.emit('participant-joined', participant);
    return participant;
  }

  removeParticipant(userId) {
    const participant = this.participants.get(userId);
    if (participant) {
      this._releaseIsoIndex(participant.isoIndex);
      this.participants.delete(userId);
      this.emit('participant-left', participant);
    }
    return participant;
  }

  updateParticipant(userId, updates) {
    const participant = this.participants.get(userId);
    if (participant) {
      Object.assign(participant, updates);
      this.emit('participant-updated', participant);
    }
    return participant;
  }

  handleConnectionError(error) {
    console.error('[SessionManager] Connection error:', error);
    this.connected = false;
    this.emit('connection-error', error);

    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++;
      const delay = this.reconnectDelay * this.reconnectAttempts;
      this.emit('reconnecting', {
        attempt: this.reconnectAttempts,
        maxAttempts: this.maxReconnectAttempts,
        delay,
      });
      this.reconnectTimer = setTimeout(() => {
        this.emit('reconnect-attempt', { attempt: this.reconnectAttempts });
      }, delay);
    } else {
      this.emit('reconnect-failed');
    }
  }

  setConnected(isConnected) {
    this.connected = isConnected;
    if (isConnected) {
      this.reconnectAttempts = 0;
    }
    this.emit('connection-status', isConnected);
  }

  getStatus() {
    return {
      connected: this.connected,
      meetingId: this.meetingId,
      sessionName: this.sessionName,
      participantCount: this.participants.size,
      participants: this.getParticipants(),
      reconnectAttempts: this.reconnectAttempts,
      zoomAvailable: this.isZoomAvailable(),
      sdkState: this.zoomBridge ? this.zoomBridge.getState() : 'unavailable',
    };
  }

  destroy() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
    }

    if (this.zoomBridge) {
      try { this.zoomBridge.leaveMeeting(); } catch (e) {}
      this.zoomBridge.cleanup();
      this.zoomBridge = null;
    }

    this.participants.clear();
    this.connected = false;
    this.removeAllListeners();
  }
}

module.exports = { SessionManager };
