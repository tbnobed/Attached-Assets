'use strict';

const net = require('net');
const EventEmitter = require('events');
const { PacketType, PacketTypeName, Decoder } = require('./protocol');

class FrameReceiver extends EventEmitter {
  constructor(options = {}) {
    super();
    this.port = options.port || 9300;
    this.host = options.host || '0.0.0.0';
    this.server = null;
    this.clients = new Map();
    this._nextClientId = 1;
    this._heartbeatInterval = null;
    this.participants = new Map();
  }

  start() {
    return new Promise((resolve, reject) => {
      this.server = net.createServer((socket) => this._handleConnection(socket));

      this.server.on('error', (err) => {
        console.error('[Receiver] Server error:', err.message);
        this.emit('error', err);
      });

      this.server.listen(this.port, this.host, () => {
        console.log(`[Receiver] Listening on ${this.host}:${this.port}`);
        this.emit('listening', { port: this.port, host: this.host });
        resolve();
      });

      this.server.once('error', reject);

      this._heartbeatInterval = setInterval(() => this._checkHeartbeats(), 10000);
    });
  }

  stop() {
    if (this._heartbeatInterval) {
      clearInterval(this._heartbeatInterval);
      this._heartbeatInterval = null;
    }

    for (const [id, client] of this.clients) {
      client.socket.destroy();
    }
    this.clients.clear();

    if (this.server) {
      this.server.close();
      this.server = null;
    }

    console.log('[Receiver] Stopped');
  }

  _handleConnection(socket) {
    const clientId = this._nextClientId++;
    const remoteAddr = `${socket.remoteAddress}:${socket.remotePort}`;
    const decoder = new Decoder();

    const client = {
      id: clientId,
      socket,
      remoteAddr,
      decoder,
      connectedAt: Date.now(),
      lastHeartbeat: Date.now(),
      packetsReceived: 0,
      bytesReceived: 0,
    };

    this.clients.set(clientId, client);
    console.log(`[Receiver] Client ${clientId} connected from ${remoteAddr}`);
    this.emit('client-connected', { clientId, remoteAddr });

    socket.on('data', (data) => {
      client.bytesReceived += data.length;
      client.lastHeartbeat = Date.now();

      const packets = decoder.push(data);
      for (const packet of packets) {
        client.packetsReceived++;
        this._handlePacket(clientId, packet);
      }
    });

    socket.on('close', () => {
      console.log(`[Receiver] Client ${clientId} disconnected`);
      this.clients.delete(clientId);
      this.emit('client-disconnected', { clientId, remoteAddr });
    });

    socket.on('error', (err) => {
      console.error(`[Receiver] Client ${clientId} error:`, err.message);
      this.clients.delete(clientId);
    });
  }

  _handlePacket(clientId, packet) {
    switch (packet.type) {
      case PacketType.VIDEO_FRAME:
        this.emit('video-frame', {
          userId: packet.userId,
          width: packet.width,
          height: packet.height,
          buffer: packet.payload,
          timestamp: packet.timestamp,
        });
        break;

      case PacketType.AUDIO_DATA:
        this.emit('audio-data', {
          userId: packet.userId,
          buffer: packet.payload,
          timestamp: packet.timestamp,
          sampleRate: packet.width || 48000,
          channels: packet.height || 1,
        });
        break;

      case PacketType.PARTICIPANT_JOIN: {
        let info = {};
        try {
          info = JSON.parse(packet.payload.toString('utf8'));
        } catch (e) {}
        const participant = {
          userId: packet.userId,
          name: info.name || `User ${packet.userId}`,
          ...info,
        };
        this.participants.set(packet.userId, participant);
        this.emit('participant-join', participant);
        console.log(`[Receiver] Participant joined: ${participant.name} (id=${packet.userId})`);
        break;
      }

      case PacketType.PARTICIPANT_LEAVE:
        this.participants.delete(packet.userId);
        this.emit('participant-leave', { userId: packet.userId });
        console.log(`[Receiver] Participant left: id=${packet.userId}`);
        break;

      case PacketType.ASSIGNMENT_UPDATE: {
        let assignment = {};
        try {
          assignment = JSON.parse(packet.payload.toString('utf8'));
        } catch (e) {}
        this.emit('assignment-update', assignment);
        break;
      }

      case PacketType.HEARTBEAT:
        break;

      default:
        console.warn(`[Receiver] Unknown packet type: 0x${packet.type.toString(16)}`);
    }
  }

  _checkHeartbeats() {
    const now = Date.now();
    const timeout = 30000;

    for (const [id, client] of this.clients) {
      if (now - client.lastHeartbeat > timeout) {
        console.warn(`[Receiver] Client ${id} timed out (no heartbeat for ${timeout}ms)`);
        client.socket.destroy();
        this.clients.delete(id);
        this.emit('client-disconnected', { clientId: id, remoteAddr: client.remoteAddr, reason: 'timeout' });
      }
    }
  }

  getStatus() {
    const clients = [];
    for (const [id, client] of this.clients) {
      clients.push({
        id: client.id,
        remoteAddr: client.remoteAddr,
        connectedAt: client.connectedAt,
        lastHeartbeat: client.lastHeartbeat,
        packetsReceived: client.packetsReceived,
        bytesReceived: client.bytesReceived,
      });
    }

    return {
      listening: !!this.server,
      port: this.port,
      host: this.host,
      clientCount: this.clients.size,
      clients,
      participantCount: this.participants.size,
      participants: Object.fromEntries(this.participants),
    };
  }
}

module.exports = { FrameReceiver };
