const EventEmitter = require('events');
const { generateJWT } = require('./jwt-generator');

class SessionManager extends EventEmitter {
  constructor(settings) {
    super();
    this.settings = settings;
    this.client = null;
    this.stream = null;
    this.participants = new Map();
    this.connected = false;
    this.sessionName = settings.sessionName;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 5;
    this.reconnectDelay = 3000;
    this.reconnectTimer = null;
    this.isoIndexPool = [];
    this.nextIsoIndex = 1;
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

  getParticipants() {
    return Array.from(this.participants.values());
  }

  getParticipant(userId) {
    return this.participants.get(userId);
  }

  addParticipant(user) {
    const participant = {
      userId: user.userId,
      displayName: user.displayName || `Participant_${user.userId}`,
      isVideoOn: false,
      isAudioOn: false,
      joinedAt: Date.now(),
      isoIndex: this._allocateIsoIndex(),
    };
    this.participants.set(user.userId, participant);
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
      console.log(`[SessionManager] Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
      this.emit('reconnecting', {
        attempt: this.reconnectAttempts,
        maxAttempts: this.maxReconnectAttempts,
        delay,
      });
      this.reconnectTimer = setTimeout(() => {
        console.log(`[SessionManager] Attempting reconnect ${this.reconnectAttempts}...`);
        try {
          const token = this.generateToken(1);
          this.emit('reconnect-attempt', { attempt: this.reconnectAttempts, token });
        } catch (err) {
          console.error('[SessionManager] Reconnect token generation failed:', err);
          this.handleConnectionError(err);
        }
      }, delay);
    } else {
      console.error('[SessionManager] Max reconnection attempts reached');
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
      sessionName: this.sessionName,
      participantCount: this.participants.size,
      participants: this.getParticipants(),
      reconnectAttempts: this.reconnectAttempts,
    };
  }

  destroy() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
    }
    this.participants.clear();
    this.connected = false;
    this.removeAllListeners();
  }
}

module.exports = { SessionManager };
