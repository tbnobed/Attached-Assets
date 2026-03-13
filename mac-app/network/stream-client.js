'use strict';

const net = require('net');
const EventEmitter = require('events');

const MAGIC = Buffer.from([0x5A, 0x4C, 0x4B, 0x31]);
const HEADER_SIZE = 4 + 1 + 4 + 2 + 2 + 8 + 4;

const PacketType = {
  VIDEO_FRAME: 0x01,
  AUDIO_DATA: 0x02,
  PARTICIPANT_JOIN: 0x10,
  PARTICIPANT_LEAVE: 0x11,
  ASSIGNMENT_UPDATE: 0x20,
  API_REQUEST: 0x30,
  API_RESPONSE: 0x31,
  HEARTBEAT: 0xFF,
};

function encodePacket(packet) {
  const {
    type,
    userId = 0,
    width = 0,
    height = 0,
    timestamp = Date.now(),
    payload = Buffer.alloc(0),
  } = packet;

  const buf = Buffer.alloc(HEADER_SIZE + payload.length);
  let offset = 0;

  MAGIC.copy(buf, offset);
  offset += 4;

  buf.writeUInt8(type, offset);
  offset += 1;

  buf.writeUInt32BE(userId, offset);
  offset += 4;

  buf.writeUInt16BE(width, offset);
  offset += 2;

  buf.writeUInt16BE(height, offset);
  offset += 2;

  const hi = Math.floor(timestamp / 0x100000000);
  const lo = timestamp >>> 0;
  buf.writeUInt32BE(hi, offset);
  offset += 4;
  buf.writeUInt32BE(lo, offset);
  offset += 4;

  buf.writeUInt32BE(payload.length, offset);
  offset += 4;

  if (payload.length > 0) {
    payload.copy(buf, offset);
  }

  return buf;
}

class RemoteSDIClient extends EventEmitter {
  constructor() {
    super();
    this._socket = null;
    this._host = null;
    this._port = 9300;
    this._connected = false;
    this._reconnecting = false;
    this._reconnectTimer = null;
    this._heartbeatTimer = null;
    this._destroyed = false;
    this._reconnectAttempts = 0;
    this._maxReconnectAttempts = Infinity;
    this._reconnectDelay = 2000;
    this._writing = false;
    this._pendingVideo = new Map();
    this._pendingAudio = new Map();
    this._frameDropCount = 0;
    this._framesSent = 0;
    this._bytesSent = 0;
    this._lastStatTime = Date.now();
    this._assignments = new Map();
    this._apiRequestId = 0;
    this._apiPending = new Map();
    this._inBuf = Buffer.alloc(0);
  }

  connect(host, port = 9300) {
    if (this._destroyed) return;

    this._teardownSocket();

    this._host = host;
    this._port = port;
    this._reconnectAttempts = 0;

    this._doConnect();
  }

  _teardownSocket() {
    this._stopHeartbeat();
    clearTimeout(this._reconnectTimer);
    this._reconnectTimer = null;
    this._reconnecting = false;
    this._connected = false;

    for (const [reqId, cb] of this._apiPending) {
      cb({ success: false, error: 'Disconnected' });
    }
    this._apiPending.clear();
    this._inBuf = Buffer.alloc(0);

    if (this._socket) {
      this._socket.removeAllListeners();
      this._socket.destroy();
      this._socket = null;
    }
  }

  _doConnect() {
    if (this._destroyed) return;

    console.log(`[RemoteSDI] Connecting to ${this._host}:${this._port}...`);
    this.emit('status', { state: 'connecting', host: this._host, port: this._port });

    this._socket = new net.Socket();
    this._socket.setNoDelay(true);
    this._socket.setKeepAlive(true, 10000);

    this._socket.connect(this._port, this._host, () => {
      console.log(`[RemoteSDI] Connected to ${this._host}:${this._port}`);
      this._connected = true;
      this._reconnecting = false;
      this._reconnectAttempts = 0;
      this._writing = false;
      this._pendingVideo.clear();
      this._pendingAudio.clear();
      this._framesSent = 0;
      this._bytesSent = 0;
      this._frameDropCount = 0;
      this._lastStatTime = Date.now();

      this._startHeartbeat();
      this._setupIncomingDecoder();

      this.emit('status', { state: 'connected', host: this._host, port: this._port });
    });

    this._socket.on('error', (err) => {
      console.error(`[RemoteSDI] Socket error: ${err.message}`);
      this.emit('error', { message: err.message });
    });

    this._socket.on('close', () => {
      this._connected = false;
      this._stopHeartbeat();
      console.log('[RemoteSDI] Connection closed');
      this.emit('status', { state: 'disconnected', host: this._host, port: this._port });

      if (!this._destroyed && this._host) {
        this._scheduleReconnect();
      }
    });

    this._socket.on('drain', () => {
      this._writing = false;
      this._flushPending();
    });
  }

  _scheduleReconnect() {
    if (this._destroyed || this._reconnecting) return;

    this._reconnecting = true;
    this._reconnectAttempts++;
    const delay = Math.min(this._reconnectDelay * Math.min(this._reconnectAttempts, 5), 10000);

    console.log(`[RemoteSDI] Reconnecting in ${delay}ms (attempt ${this._reconnectAttempts})`);
    this.emit('status', {
      state: 'reconnecting',
      attempt: this._reconnectAttempts,
      host: this._host,
      port: this._port,
    });

    this._reconnectTimer = setTimeout(() => {
      this._reconnecting = false;
      this._doConnect();
    }, delay);
  }

  disconnect() {
    this._teardownSocket();
    this._host = null;
    this._assignments.clear();
    this._pendingVideo.clear();
    this._pendingAudio.clear();
    this.emit('status', { state: 'disconnected' });
  }

  destroy() {
    this._destroyed = true;
    this.disconnect();
    this.removeAllListeners();
  }

  _startHeartbeat() {
    this._stopHeartbeat();
    this._heartbeatTimer = setInterval(() => {
      if (this._connected) {
        this._writeRaw(encodePacket({
          type: PacketType.HEARTBEAT,
          payload: Buffer.alloc(0),
        }));
      }
    }, 5000);
  }

  _stopHeartbeat() {
    if (this._heartbeatTimer) {
      clearInterval(this._heartbeatTimer);
      this._heartbeatTimer = null;
    }
  }

  _setupIncomingDecoder() {
    if (!this._socket) return;
    this._inBuf = Buffer.alloc(0);

    this._socket.on('data', (data) => {
      this._inBuf = Buffer.concat([this._inBuf, data]);

      while (this._inBuf.length >= HEADER_SIZE) {
        if (this._inBuf[0] !== 0x5A || this._inBuf[1] !== 0x4C ||
            this._inBuf[2] !== 0x4B || this._inBuf[3] !== 0x31) {
          let found = -1;
          for (let i = 1; i <= this._inBuf.length - 4; i++) {
            if (this._inBuf[i] === 0x5A && this._inBuf[i+1] === 0x4C &&
                this._inBuf[i+2] === 0x4B && this._inBuf[i+3] === 0x31) {
              found = i; break;
            }
          }
          if (found === -1) { this._inBuf = Buffer.alloc(0); break; }
          this._inBuf = this._inBuf.subarray(found);
          continue;
        }

        const payloadLen = this._inBuf.readUInt32BE(HEADER_SIZE - 4);
        const totalLen = HEADER_SIZE + payloadLen;
        if (this._inBuf.length < totalLen) break;

        const type = this._inBuf.readUInt8(4);
        const reqId = this._inBuf.readUInt32BE(5);
        const payload = Buffer.from(this._inBuf.subarray(HEADER_SIZE, totalLen));
        this._inBuf = this._inBuf.subarray(totalLen);

        if (type === PacketType.API_RESPONSE) {
          const cb = this._apiPending.get(reqId);
          if (cb) {
            this._apiPending.delete(reqId);
            try {
              cb(JSON.parse(payload.toString('utf8')));
            } catch (e) {
              cb({ success: false, error: 'Invalid response JSON' });
            }
          }
        }
      }
    });
  }

  apiRequest(path, body = {}, timeout = 10000) {
    return new Promise((resolve) => {
      if (!this._connected) {
        return resolve({ success: false, error: 'Not connected' });
      }
      const reqId = ++this._apiRequestId;
      const payload = Buffer.from(JSON.stringify({ path, body }));

      const timer = setTimeout(() => {
        this._apiPending.delete(reqId);
        resolve({ success: false, error: 'Request timed out' });
      }, timeout);

      this._apiPending.set(reqId, (response) => {
        clearTimeout(timer);
        resolve(response);
      });

      this._writeRaw(encodePacket({
        type: PacketType.API_REQUEST,
        userId: reqId,
        payload,
      }));
    });
  }

  _writeRaw(buf) {
    if (!this._connected || !this._socket || this._socket.destroyed) return false;
    const ok = this._socket.write(buf);
    this._bytesSent += buf.length;
    if (!ok) {
      this._writing = true;
    }
    return ok;
  }

  _writePacket(packet) {
    return this._writeRaw(encodePacket(packet));
  }

  _flushPending() {
    if (!this._connected || !this._socket || this._socket.destroyed) return;

    const videoBufs = [];
    for (const [userId, frame] of this._pendingVideo) {
      videoBufs.push(encodePacket({
        type: PacketType.VIDEO_FRAME,
        userId,
        width: frame.width,
        height: frame.height,
        payload: frame.payload,
      }));
      this._framesSent++;
    }
    this._pendingVideo.clear();

    for (const [userId, audio] of this._pendingAudio) {
      videoBufs.push(encodePacket({
        type: PacketType.AUDIO_DATA,
        userId,
        width: audio.sampleRate,
        height: audio.channels,
        payload: audio.payload,
      }));
    }
    this._pendingAudio.clear();

    if (videoBufs.length === 0) return;

    const combined = Buffer.concat(videoBufs);
    const ok = this._socket.write(combined);
    this._bytesSent += combined.length;

    if (!ok) {
      this._writing = true;
    }
  }

  sendVideoFrame(userId, buffer, width, height) {
    if (!this._connected) return;

    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);
    const payload = Buffer.isBuffer(buffer) ? buffer : Buffer.from(buffer);

    if (this._writing) {
      this._pendingVideo.set(numericId, { width, height, payload });
      this._frameDropCount++;
      return;
    }

    this._writePacket({
      type: PacketType.VIDEO_FRAME,
      userId: numericId,
      width,
      height,
      payload,
    });

    this._framesSent++;
  }

  sendAudioData(userId, buffer, sampleRate, channels) {
    if (!this._connected) return;

    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);
    const payload = Buffer.isBuffer(buffer) ? buffer : Buffer.from(buffer);

    if (this._writing) {
      this._pendingAudio.set(numericId, { sampleRate: sampleRate || 48000, channels: channels || 1, payload });
      return;
    }

    this._writePacket({
      type: PacketType.AUDIO_DATA,
      userId: numericId,
      width: sampleRate || 48000,
      height: channels || 1,
      payload,
    });
  }

  sendParticipantJoin(userId, displayName) {
    if (!this._connected) return;

    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);
    const payload = Buffer.from(JSON.stringify({ userId: numericId, displayName }));

    this._writePacket({
      type: PacketType.PARTICIPANT_JOIN,
      userId: numericId,
      payload,
    });
  }

  sendParticipantLeave(userId) {
    if (!this._connected) return;

    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);

    this._writePacket({
      type: PacketType.PARTICIPANT_LEAVE,
      userId: numericId,
      payload: Buffer.alloc(0),
    });
  }

  sendAssignment(userId, deviceIndex) {
    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);
    this._assignments.set(numericId, deviceIndex);

    if (!this._connected) return;

    const payload = Buffer.from(JSON.stringify({ userId: numericId, deviceIndex }));
    this._writePacket({
      type: PacketType.ASSIGNMENT_UPDATE,
      userId: numericId,
      payload,
    });
  }

  removeAssignment(userId) {
    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);
    this._assignments.delete(numericId);

    if (!this._connected) return;

    const payload = Buffer.from(JSON.stringify({ userId: numericId, deviceIndex: -1 }));
    this._writePacket({
      type: PacketType.ASSIGNMENT_UPDATE,
      userId: numericId,
      payload,
    });
  }

  _flushAssignments() {
    for (const [userId, deviceIndex] of this._assignments) {
      const payload = Buffer.from(JSON.stringify({ userId, deviceIndex }));
      this._writePacket({
        type: PacketType.ASSIGNMENT_UPDATE,
        userId,
        payload,
      });
    }
  }

  _hashUserId(str) {
    let hash = 0;
    const s = String(str);
    for (let i = 0; i < s.length; i++) {
      hash = ((hash << 5) - hash + s.charCodeAt(i)) >>> 0;
    }
    return hash;
  }

  isConnected() {
    return this._connected;
  }

  getStatus() {
    const now = Date.now();
    const elapsed = (now - this._lastStatTime) / 1000 || 1;

    return {
      connected: this._connected,
      host: this._host,
      port: this._port,
      framesSent: this._framesSent,
      framesDropped: this._frameDropCount,
      bytesSent: this._bytesSent,
      fps: Math.round(this._framesSent / elapsed),
      reconnecting: this._reconnecting,
      reconnectAttempt: this._reconnectAttempts,
      assignments: Object.fromEntries(this._assignments),
    };
  }
}

module.exports = { RemoteSDIClient, PacketType };
