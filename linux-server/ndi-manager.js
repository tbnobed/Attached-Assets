'use strict';

const EventEmitter = require('events');

class NDIManager extends EventEmitter {
  constructor(options = {}) {
    super();
    this.prefix = options.prefix || 'ZoomLink';
    this.sources = new Map();
    this.grandiose = null;
    this.available = false;
    this._init();
  }

  _init() {
    try {
      this.grandiose = require('grandiose');
      if (typeof this.grandiose.send !== 'function') {
        console.warn('[NDI] grandiose loaded but send() not available');
        this.available = false;
        return;
      }
      this.available = true;
      console.log('[NDI] grandiose loaded — NDI output available');
    } catch (err) {
      console.warn('[NDI] grandiose not available — NDI output disabled');
      console.warn('[NDI] Error:', err.message);
      this.available = false;
    }
  }

  isAvailable() {
    return this.available;
  }

  async createSource(userId, displayName, isoIndex) {
    const safeName = String(displayName).replace(/[^a-zA-Z0-9_-]/g, '_');
    const sourceName = `${this.prefix}_ISO${isoIndex || userId}_${safeName}`;

    if (this.sources.has(userId)) {
      const existing = this.sources.get(userId);
      if (existing.sourceName === sourceName && existing.sender && !existing.mock) {
        console.log(`[NDI] Source ${sourceName} already exists — reusing`);
        return existing;
      }
      this.destroySource(userId);
    }

    if (!this.available) {
      const mockSource = {
        userId,
        displayName,
        isoIndex,
        sourceName,
        sender: null,
        active: true,
        mock: true,
        createdAt: Date.now(),
        framesSent: 0,
      };
      this.sources.set(userId, mockSource);
      this.emit('source-created', { userId, sourceName, mock: true });
      console.log(`[NDI] Created mock source: ${sourceName}`);
      return mockSource;
    }

    try {
      const t0 = Date.now();
      const sender = await this.grandiose.send({
        name: sourceName,
        clockVideo: false,
        clockAudio: false,
      });
      const dt = Date.now() - t0;
      if (dt > 50) {
        console.warn(`[NDI] grandiose.send() took ${dt}ms`);
      }

      const source = {
        userId,
        displayName,
        isoIndex,
        sourceName,
        sender,
        active: true,
        mock: false,
        createdAt: Date.now(),
        framesSent: 0,
        _audioQueue: [],
        _audioSending: false,
        _audioSentCount: 0,
        _audioErrorCount: 0,
      };

      this.sources.set(userId, source);
      this.emit('source-created', { userId, sourceName, mock: false });
      console.log(`[NDI] Created source: ${sourceName}`);
      return source;
    } catch (err) {
      console.error(`[NDI] Failed to create source for ${displayName}:`, err.message);
      return null;
    }
  }

  destroySource(userId) {
    const source = this.sources.get(userId);
    if (!source) return;

    if (!source.mock && source.sender) {
      try {
        const t0 = Date.now();
        if (typeof source.sender.destroy === 'function') {
          source.sender.destroy();
        }
        const dt = Date.now() - t0;
        if (dt > 50) {
          console.warn(`[NDI] sender.destroy() took ${dt}ms for ${source.displayName}`);
        }
      } catch (err) {
        console.error(`[NDI] Error destroying source for ${source.displayName}:`, err.message);
      }
    }

    this.sources.delete(userId);
    this.emit('source-destroyed', { userId, displayName: source.displayName });
    console.log(`[NDI] Destroyed source for ${source.displayName}`);
  }

  toggleSource(userId, active) {
    const source = this.sources.get(userId);
    if (source) {
      source.active = active;
    }
  }

  sendVideoFrame(userId, frameBuffer, width, height) {
    const source = this.sources.get(userId);
    if (!source || !source.active) return;

    source.framesSent++;

    if (source.mock || !source.sender) return;

    try {
      source.sender.video({
        xres: width,
        yres: height,
        frameRateN: 30000,
        frameRateD: 1001,
        pictureAspectRatio: width / height,
        frameFormatType: 1,
        fourCC: this.grandiose.FOURCC_BGRA,
        lineStrideBytes: width * 4,
        data: frameBuffer,
      });
    } catch (err) {
      if (!source._videoErrorLogged) {
        console.error(`[NDI] Error sending video for ${source.displayName}:`, err.message);
        source._videoErrorLogged = true;
      }
    }
  }

  sendAudioData(userId, audioBuffer, sampleRate, channels) {
    const source = this.sources.get(userId);
    if (!source || !source.active) return;
    if (source.mock || !source.sender) return;

    const ch = channels || 1;
    const bytesPerSample = 2;
    const incomingSamples = Math.floor(audioBuffer.length / (bytesPerSample * ch));
    if (incomingSamples <= 0) return;

    const floatBuf = Buffer.alloc(incomingSamples * 4 * 2);
    for (let s = 0; s < incomingSamples; s++) {
      const int16 = audioBuffer.readInt16LE(s * ch * bytesPerSample);
      const fVal = int16 / 32768.0;
      floatBuf.writeFloatLE(fVal, s * 4);
      floatBuf.writeFloatLE(fVal, incomingSamples * 4 + s * 4);
    }

    source._audioQueue.push({
      samples: incomingSamples,
      data: floatBuf,
      sr: sampleRate || 48000,
    });

    const maxQueued = 10;
    if (source._audioQueue.length > maxQueued) {
      source._audioQueue = source._audioQueue.slice(-maxQueued);
    }

    if (!source._audioSending) {
      this._drainAudioQueue(userId);
    }
  }

  _drainAudioQueue(userId) {
    const source = this.sources.get(userId);
    if (!source || !source.sender || source._audioSending) return;
    if (source._audioQueue.length === 0) return;

    source._audioSending = true;
    const fourCC = this.grandiose.FOURCC_FLTp || 0x70544C46;

    const processNext = () => {
      if (!source._audioQueue.length || !source.sender) {
        source._audioSending = false;
        return;
      }

      const chunk = source._audioQueue.shift();
      source._audioSentCount++;

      try {
        const result = source.sender.audio({
          fourCC: fourCC,
          sampleRate: chunk.sr,
          noChannels: 2,
          noSamples: chunk.samples,
          channelStrideBytes: chunk.samples * 4,
          data: chunk.data,
        });

        if (result && typeof result.then === 'function') {
          result.then(() => setTimeout(processNext, 0)).catch(() => setTimeout(processNext, 0));
        } else {
          setTimeout(processNext, 0);
        }
      } catch (err) {
        source._audioErrorCount++;
        if (source._audioErrorCount <= 3) {
          console.error(`[NDI] Audio error #${source._audioErrorCount} for ${source.displayName}:`, err.message);
        }
        source._audioSending = false;
      }
    };

    processNext();
  }

  destroyAll() {
    for (const [userId] of this.sources) {
      this.destroySource(userId);
    }
  }

  destroy() {
    this.destroyAll();
    this.removeAllListeners();
  }

  getStatus() {
    const sources = {};
    for (const [userId, source] of this.sources) {
      sources[userId] = {
        displayName: source.displayName,
        isoIndex: source.isoIndex,
        sourceName: source.sourceName,
        active: source.active,
        mock: source.mock,
        framesSent: source.framesSent,
      };
    }
    return {
      available: this.available,
      sourceCount: this.sources.size,
      sources,
    };
  }
}

module.exports = { NDIManager };
