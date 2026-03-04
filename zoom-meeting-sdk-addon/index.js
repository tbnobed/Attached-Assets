const path = require('path');
const EventEmitter = require('events');

let nativeAddon = null;
let addonAvailable = false;

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

  joinMeeting(meetingId, password, displayName) {
    if (!addonAvailable) {
      console.warn('[ZoomMeetingSDK] Stub mode: joinMeeting() skipped');
      this.inMeeting = true;
      return true;
    }

    const cleanId = meetingId.replace(/[\s-]/g, '');
    return nativeAddon.joinMeeting(cleanId, password, displayName || 'PlexISO');
  }

  leaveMeeting() {
    if (!addonAvailable) {
      this.inMeeting = false;
      return true;
    }
    const ok = nativeAddon.leaveMeeting();
    this.inMeeting = false;
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
      switch (event.type) {
        case 'participant-joined':
          this.emit('participant-joined', {
            userId: event.userId,
            displayName: event.displayName,
          });
          break;
        case 'participant-left':
          this.emit('participant-left', {
            userId: event.userId,
            displayName: event.displayName,
          });
          break;
        case 'meeting-status':
          this._handleMeetingStatus(event.status);
          break;
      }
    });
  }

  _handleMeetingStatus(status) {
    switch (status) {
      case 'MEETING_STATUS_INMEETING':
        this.inMeeting = true;
        this.emit('meeting-joined');
        break;
      case 'MEETING_STATUS_ENDED':
      case 'MEETING_STATUS_FAILED':
        this.inMeeting = false;
        this.emit('meeting-ended', { status });
        break;
      case 'MEETING_STATUS_RECONNECTING':
        this.emit('meeting-reconnecting');
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
}

module.exports = { ZoomMeetingBridge };
