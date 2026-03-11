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
    this._maxReconnectAttempts = 10;
    this._reconnectDelay = 2000;
    this._writeQueue = [];
    this._draining = false;
    this._frameDropCount = 0;
    this._framesSent = 0;
    this._bytesSent = 0;
    this._lastStatTime = Date.now();
    this._assignments = new Map();
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

    this._socket.connect(this._port, this._host, () => {
      console.log(`[RemoteSDI] Connected to ${this._host}:${this._port}`);
      this._connected = true;
      this._reconnecting = false;
      this._reconnectAttempts = 0;
      this._framesSent = 0;
      this._bytesSent = 0;
      this._frameDropCount = 0;
      this._lastStatTime = Date.now();

      this._startHeartbeat();
      this._flushAssignments();

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
      this._draining = false;
    });
  }

  _scheduleReconnect() {
    if (this._destroyed || this._reconnecting) return;
    if (this._reconnectAttempts >= this._maxReconnectAttempts) {
      console.log('[RemoteSDI] Max reconnect attempts reached');
      this.emit('status', { state: 'failed', host: this._host, port: this._port });
      return;
    }

    this._reconnecting = true;
    this._reconnectAttempts++;
    const delay = Math.min(this._reconnectDelay * this._reconnectAttempts, 15000);

    console.log(`[RemoteSDI] Reconnecting in ${delay}ms (attempt ${this._reconnectAttempts}/${this._maxReconnectAttempts})`);
    this.emit('status', {
      state: 'reconnecting',
      attempt: this._reconnectAttempts,
      maxAttempts: this._maxReconnectAttempts,
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
        this._writePacket({
          type: PacketType.HEARTBEAT,
          payload: Buffer.alloc(0),
        });
      }
    }, 5000);
  }

  _stopHeartbeat() {
    if (this._heartbeatTimer) {
      clearInterval(this._heartbeatTimer);
      this._heartbeatTimer = null;
    }
  }

  _writePacket(packet) {
    if (!this._connected || !this._socket || this._socket.destroyed) return false;

    const buf = encodePacket(packet);
    const ok = this._socket.write(buf);
    this._bytesSent += buf.length;

    if (!ok) {
      this._draining = true;
    }

    return ok;
  }

  sendVideoFrame(userId, buffer, width, height) {
    if (!this._connected) return;

    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);

    if (this._draining) {
      this._frameDropCount++;
      return;
    }

    const payload = Buffer.isBuffer(buffer) ? buffer : Buffer.from(buffer);

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
    if (!this._connected || this._draining) return;

    const numericId = typeof userId === 'number' ? userId : this._hashUserId(userId);
    const payload = Buffer.isBuffer(buffer) ? buffer : Buffer.from(buffer);

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
