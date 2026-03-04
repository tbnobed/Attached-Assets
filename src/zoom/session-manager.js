const EventEmitter = require('events');
const { generateJWT } = require('./jwt-generator');

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
        this.zoomBridge.auth();
      }

      this.zoomBridge.on('participant-joined', ({ userId, displayName }) => {
        this.addParticipant({ userId: String(userId), displayName });
      });

      this.zoomBridge.on('participant-left', ({ userId }) => {
        this.removeParticipant(String(userId));
      });

      this.zoomBridge.on('video-frame', ({ userId, frameData }) => {
        if (this.streamHandler) {
          this.streamHandler.handleVideoFrame(String(userId), frameData);
        }
      });

      this.zoomBridge.on('audio-frame', ({ userId, audioData }) => {
        if (this.streamHandler) {
          this.streamHandler.handleAudioData(String(userId), audioData);
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

      this.zoomBridge.on('auth-failed', () => {
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

  joinMeeting(meetingId, password, displayName) {
    if (!this.zoomBridge) {
      throw new Error('Zoom Meeting SDK not available. Build the native addon first.');
    }

    this.meetingId = meetingId;
    this.meetingPassword = password || '';

    return this.zoomBridge.joinMeeting(
      meetingId,
      password || '',
      displayName || this.settings.botName || 'PlexISO'
    );
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
