const path = require('path');
const EventEmitter = require('events');

let nativeAddon = null;
let addonAvailable = false;

const sdkLibDir = path.join(__dirname, 'build', 'Release');
const sdkDir = path.join(__dirname, 'sdk', 'lib');

if (process.platform === 'win32' && !process.env.PATH.includes(sdkLibDir)) {
  process.env.PATH = sdkLibDir + ';' + process.env.PATH;
} else if (process.platform === 'darwin') {
  const currentDyldPath = process.env.DYLD_LIBRARY_PATH || '';
  if (!currentDyldPath.includes(sdkLibDir)) {
    process.env.DYLD_LIBRARY_PATH = sdkLibDir + (currentDyldPath ? ':' + currentDyldPath : '');
  }
  const currentFrameworkPath = process.env.DYLD_FRAMEWORK_PATH || '';
  if (!currentFrameworkPath.includes(sdkDir)) {
    process.env.DYLD_FRAMEWORK_PATH = sdkDir + (currentFrameworkPath ? ':' + currentFrameworkPath : '');
  }
}

try {
  nativeAddon = require('./build/Release/zoom_meeting_sdk.node');
  addonAvailable = true;
} catch (err) {
  console.warn('[ZoomMeetingSDK] Native addon not available:', err.message);
  console.warn('[ZoomMeetingSDK] Running in stub mode — no actual Zoom connection.');
}

class ZoomMeetingBridge extends EventEmitter {
  constructor() {
    super();
    this.initialized = false;
    this.authenticated = false;
    this.inMeeting = false;
    this._selfDetected = false;
    this._selfUserId = 0;
    this._pendingJoins = [];
    this._knownParticipants = new Set();
    this._retryInterval = null;
  }

  isAvailable() {
    return addonAvailable;
  }

  init(config) {
    this._config = config;

    if (!addonAvailable) {
      console.warn('[ZoomMeetingSDK] Stub mode: init() skipped');
      this.initialized = true;
      return true;
    }

    const jwtToken = this._generateJWT(config.sdkKey, config.sdkSecret);

    const ok = nativeAddon.init({
      sdkKey: config.sdkKey,
      sdkSecret: config.sdkSecret,
      botName: config.botName || 'PlexISO',
      jwtToken,
    });

    if (ok) {
      this.initialized = true;
      this._setupCallbacks();
    }
    return ok;
  }

  auth() {
    if (!addonAvailable) {
      this.authenticated = true;
      return true;
    }

    const ok = nativeAddon.auth();
    if (!ok) return false;

    return true;
  }

  _generateJWT(sdkKey, sdkSecret) {
    const crypto = require('crypto');

    function base64UrlEncode(data) {
      return Buffer.from(data)
        .toString('base64')
        .replace(/\+/g, '-')
        .replace(/\//g, '_')
        .replace(/=+$/, '');
    }

    const iat = Math.floor(Date.now() / 1000) - 30;
    const exp = iat + 60 * 60 * 2;

    const header = { alg: 'HS256', typ: 'JWT' };
    const payload = {
      appKey: sdkKey,
      sdkKey: sdkKey,
      mn: 0,
      role: 1,
      iat,
      exp,
      tokenExp: exp,
    };

    const encodedHeader = base64UrlEncode(JSON.stringify(header));
    const encodedPayload = base64UrlEncode(JSON.stringify(payload));
    const signingInput = `${encodedHeader}.${encodedPayload}`;

    const signature = crypto
      .createHmac('sha256', sdkSecret)
      .update(signingInput)
      .digest('base64')
      .replace(/\+/g, '-')
      .replace(/\//g, '_')
      .replace(/=+$/, '');

    return `${signingInput}.${signature}`;
  }

  joinMeeting(meetingId, password, displayName, appPrivilegeToken) {
    if (!addonAvailable) {
      console.warn('[ZoomMeetingSDK] Stub mode: joinMeeting() skipped');
      this.inMeeting = true;
      return true;
    }

    this._selfDetected = false;
    this._selfUserId = 0;
    this._pendingJoins = [];
    this._knownParticipants = new Set();

    const cleanId = meetingId.replace(/[\s-]/g, '');
    const args = [cleanId, password, displayName || 'PlexISO', appPrivilegeToken || ''];
    console.log('[TokenDebug] joinMeeting args:', args.map((a, i) =>
      `[${i}] type=${typeof a} len=${String(a).length} val=${String(a).substring(0,30)}`
    ));
    if (appPrivilegeToken) {
      console.log('[ZoomBridge] Joining with app_privilege_token (local recording pre-authorized)');
    }
    return nativeAddon.joinMeeting(...args);
  }

  leaveMeeting() {
    if (!addonAvailable) {
      this.inMeeting = false;
      return true;
    }
    const ok = nativeAddon.leaveMeeting();
    this.inMeeting = false;
    this._selfDetected = false;
    this._selfUserId = 0;
    this._pendingJoins = [];
    this._knownParticipants = new Set();
    if (this._retryInterval) { clearInterval(this._retryInterval); this._retryInterval = null; }
    return ok;
  }

  startRawCapture() {
    if (!addonAvailable) return true;
    return nativeAddon.startRawCapture();
  }

  getParticipants() {
    if (!addonAvailable) return [];
    return nativeAddon.getParticipants();
  }

  getState() {
    if (!addonAvailable) {
      if (this.inMeeting) return 'in-meeting';
      if (this.authenticated) return 'authenticated';
      if (this.initialized) return 'initialized';
      return 'uninitialized';
    }
    return nativeAddon.getState();
  }

  cleanup() {
    if (addonAvailable) {
      nativeAddon.cleanup();
    }
    this.initialized = false;
    this.authenticated = false;
    this.inMeeting = false;
    this._selfDetected = false;
    this._selfUserId = 0;
    this._pendingJoins = [];
    if (this._retryInterval) { clearInterval(this._retryInterval); this._retryInterval = null; }
  }

  _setupCallbacks() {
    nativeAddon.onVideoFrame((userId, buffer, width, height) => {
      this.emit('video-frame', {
        userId,
        frameData: {
          buffer,
          width,
          height,
        },
      });
    });

    nativeAddon.onAudioFrame((userId, buffer, sampleRate, channels) => {
      if (this._selfUserId && userId === this._selfUserId) return;
      this.emit('audio-frame', {
        userId,
        audioData: {
          buffer,
          sampleRate,
          channels,
        },
      });
    });

    nativeAddon.onEvent((event) => {
      console.log('[ZoomBridge] Event:', event.type, event.status || '', event.displayName || '', event.userId || '');
      switch (event.type) {
        case 'participant-joined':
          this._handleParticipantJoined(event);
          break;
        case 'participant-left':
          this._handleParticipantLeft(event);
          break;
        case 'participant-name-updated':
          console.log(`[ZoomBridge] Name updated: userId=${event.userId} name=${event.displayName}`);
          this.emit('participant-name-updated', { userId: event.userId, displayName: event.displayName });
          break;
        case 'meeting-status':
          this._handleMeetingStatus(event.status, event);
          break;
      }
    });
  }

  _handleParticipantJoined(event) {
    const userId = event.userId;
    const displayName = event.displayName || 'Participant';
    const isSelf = !!event.isSelf;

    if (isSelf) {
      console.log(`[ZoomBridge] Participant ${displayName} (${userId}) flagged as self — ignoring`);
      this._selfUserId = userId;
      this._selfDetected = true;
      return;
    }

    if (this._selfUserId && userId === this._selfUserId) {
      console.log(`[ZoomBridge] Participant ${displayName} (${userId}) matches self — ignoring`);
      return;
    }

    if (!this._selfDetected) {
      console.log(`[ZoomBridge] Buffering participant ${displayName} (${userId}) — self not yet detected`);
      this._pendingJoins.push({ userId, displayName });
      return;
    }

    this._emitParticipantJoined(userId, displayName);
  }

  _emitParticipantJoined(userId, displayName) {
    if (this._knownParticipants.has(userId)) return;
    if (this._selfUserId && userId === this._selfUserId) return;

    this._knownParticipants.add(userId);
    console.log(`[ZoomBridge] Emitting participant-joined: ${displayName} (${userId})`);
    this.emit('participant-joined', { userId, displayName, isSelf: false });

    setTimeout(() => this._retrySubscriptions(), 500);
    setTimeout(() => this._retrySubscriptions(), 1500);
    setTimeout(() => this._retrySubscriptions(), 3000);
  }

  _flushPendingJoins() {
    if (this._pendingJoins.length === 0) return;
    console.log(`[ZoomBridge] Flushing ${this._pendingJoins.length} buffered participant-joined events (selfUserId=${this._selfUserId})`);

    const pending = this._pendingJoins.splice(0);
    for (const p of pending) {
      if (this._selfUserId && p.userId === this._selfUserId) {
        console.log(`[ZoomBridge] Flushing: skipping self ${p.displayName} (${p.userId})`);
        continue;
      }
      this._emitParticipantJoined(p.userId, p.displayName);
    }
  }

  _handleParticipantLeft(event) {
    const userId = event.userId;

    if (event.displayName === '_self_purged_') {
      console.log(`[ZoomBridge] Self purged (${userId}) — updating self detection`);
      this._selfUserId = userId;
      this._selfDetected = true;
      this._knownParticipants.delete(userId);
      this._flushPendingJoins();
      return;
    }

    this._knownParticipants.delete(userId);
    this.emit('participant-left', {
      userId,
      displayName: event.displayName,
    });
  }

  _retrySubscriptions() {
    if (!addonAvailable || !this.inMeeting) return;
    try {
      nativeAddon.retryVideoSubscriptions();
    } catch (err) {
      console.error('[ZoomBridge] Error retrying subscriptions:', err.message);
    }
  }

  _enumerateExistingParticipants() {
    if (!addonAvailable) return;
    try {
      nativeAddon.enumerateParticipants();

      const participants = nativeAddon.getParticipants();
      console.log('[ZoomBridge] Enumerating existing participants:', participants.length);
      for (const p of participants) {
        if (this._selfUserId && p.userId === this._selfUserId) continue;
        this._emitParticipantJoined(p.userId, p.displayName);
      }
    } catch (err) {
      console.error('[ZoomBridge] Error enumerating participants:', err.message);
    }
  }

  _handleMeetingStatus(status, data) {
    switch (status) {
      case 'MEETING_STATUS_INMEETING':
        this.inMeeting = true;
        console.log('[ZoomBridge] In meeting — initiating raw recording and participant enumeration');

        if (!this._selfDetected) {
          try {
            const selfId = nativeAddon.getSelfUserId ? nativeAddon.getSelfUserId() : 0;
            if (selfId) {
              this._selfUserId = selfId;
              this._selfDetected = true;
              console.log(`[ZoomBridge] Self detected from native: userId=${selfId}`);
            }
          } catch (e) {}
        }

        this._flushPendingJoins();

        this.emit('meeting-joined');

        setTimeout(() => { this._detectSelfAndFlush(); this._enumerateExistingParticipants(); }, 500);
        setTimeout(() => { this._detectSelfAndFlush(); this._enumerateExistingParticipants(); }, 2000);
        setTimeout(() => { this._detectSelfAndFlush(); this._enumerateExistingParticipants(); }, 5000);
        setTimeout(() => { this._detectSelfAndFlush(); this._enumerateExistingParticipants(); }, 10000);
        setTimeout(() => this._retrySubscriptions(), 3000);
        setTimeout(() => this._retrySubscriptions(), 8000);
        setTimeout(() => this._retrySubscriptions(), 15000);
        setTimeout(() => this._retrySubscriptions(), 25000);
        if (this._retryInterval) clearInterval(this._retryInterval);
        this._retryInterval = null;
        break;
      case 'MEETING_STATUS_ENDED':
      case 'MEETING_STATUS_FAILED':
        this.inMeeting = false;
        if (this._retryInterval) { clearInterval(this._retryInterval); this._retryInterval = null; }
        this.emit('meeting-ended', { status });
        break;
      case 'MEETING_STATUS_RECONNECTING':
        this.emit('meeting-reconnecting');
        break;
      case 'RAW_RECORDING_STARTED':
        console.log('[ZoomBridge] Raw recording started — video/audio capture active');
        this.emit('raw-recording-started');
        this._enumerateExistingParticipants();
        this._retrySubscriptions();
        setTimeout(() => this._retrySubscriptions(), 500);
        setTimeout(() => this._retrySubscriptions(), 1500);
        setTimeout(() => this._retrySubscriptions(), 3000);
        break;
      case 'RAW_RECORDING_ERROR':
        console.warn(`[ZoomBridge] Raw recording error (code=${data && data.errorCode}) — retries will continue`);
        this.emit('raw-recording-error', { errorCode: data && data.errorCode || 0 });
        break;
      case 'MEETING_STATUS_WAITINGFORHOST':
        console.log('[ZoomBridge] Waiting for host...');
        break;
      case 'MEETING_STATUS_CONNECTING':
        console.log('[ZoomBridge] Connecting to meeting...');
        break;
      case 'AUTH_SUCCESS':
        this.authenticated = true;
        this.emit('auth-success');
        break;
      case 'AUTH_FAILED':
        this.emit('auth-failed');
        break;
      default:
        this.emit('status-change', status);
        break;
    }
  }

  _detectSelfAndFlush() {
    if (this._selfDetected) return;
    try {
      const selfId = nativeAddon.getSelfUserId ? nativeAddon.getSelfUserId() : 0;
      if (selfId) {
        this._selfUserId = selfId;
        this._selfDetected = true;
        console.log(`[ZoomBridge] Delayed self detection: userId=${selfId}`);
        this._flushPendingJoins();
      }
    } catch (e) {}
  }
}

module.exports = { ZoomMeetingBridge };
