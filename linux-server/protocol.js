'use strict';

const MAGIC = Buffer.from([0x5A, 0x4C, 0x4B, 0x31]);

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

const PacketTypeName = {};
for (const [name, value] of Object.entries(PacketType)) {
  PacketTypeName[value] = name;
}

const HEADER_SIZE = 4 + 1 + 4 + 2 + 2 + 8 + 4;
const MAX_PAYLOAD_SIZE = 50 * 1024 * 1024;

function encode(packet) {
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

class Decoder {
  constructor() {
    this._buffer = Buffer.alloc(0);
  }

  push(data) {
    this._buffer = Buffer.concat([this._buffer, data]);
    const packets = [];

    while (this._buffer.length >= HEADER_SIZE) {
      if (
        this._buffer[0] !== MAGIC[0] ||
        this._buffer[1] !== MAGIC[1] ||
        this._buffer[2] !== MAGIC[2] ||
        this._buffer[3] !== MAGIC[3]
      ) {
        const idx = this._findMagic(1);
        if (idx === -1) {
          this._buffer = Buffer.alloc(0);
          break;
        }
        this._buffer = this._buffer.subarray(idx);
        continue;
      }

      let off = 4;

      const type = this._buffer.readUInt8(off);
      off += 1;

      const userId = this._buffer.readUInt32BE(off);
      off += 4;

      const width = this._buffer.readUInt16BE(off);
      off += 2;

      const height = this._buffer.readUInt16BE(off);
      off += 2;

      const tsHi = this._buffer.readUInt32BE(off);
      off += 4;
      const tsLo = this._buffer.readUInt32BE(off);
      off += 4;
      const timestamp = tsHi * 0x100000000 + tsLo;

      const payloadLength = this._buffer.readUInt32BE(off);
      off += 4;

      if (payloadLength > MAX_PAYLOAD_SIZE) {
        this._buffer = this._buffer.subarray(off);
        continue;
      }

      const totalLength = HEADER_SIZE + payloadLength;
      if (this._buffer.length < totalLength) {
        break;
      }

      const payload = Buffer.from(this._buffer.subarray(off, off + payloadLength));

      packets.push({
        type,
        typeName: PacketTypeName[type] || 'UNKNOWN',
        userId,
        width,
        height,
        timestamp,
        payload,
      });

      this._buffer = this._buffer.subarray(totalLength);
    }

    return packets;
  }

  _findMagic(startOffset) {
    for (let i = startOffset; i <= this._buffer.length - 4; i++) {
      if (
        this._buffer[i] === MAGIC[0] &&
        this._buffer[i + 1] === MAGIC[1] &&
        this._buffer[i + 2] === MAGIC[2] &&
        this._buffer[i + 3] === MAGIC[3]
      ) {
        return i;
      }
    }
    return -1;
  }

  reset() {
    this._buffer = Buffer.alloc(0);
  }
}

module.exports = {
  MAGIC,
  PacketType,
  PacketTypeName,
  HEADER_SIZE,
  MAX_PAYLOAD_SIZE,
  encode,
  Decoder,
};
