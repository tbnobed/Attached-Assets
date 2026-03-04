const EventEmitter = require('events');

class NDIManager extends EventEmitter {
  constructor(settings) {
    super();
    this.settings = settings;
    this.sources = new Map();
    this.grandiose = null;
    this.available = false;
    this.prefix = settings.ndi.prefix;
    this.testPatternActive = false;
    this.testPatternInterval = null;
    this._init();
  }

  _init() {
    try {
      this.grandiose = require('grandiose');
      if (typeof this.grandiose.send !== 'function') {
        console.warn('[NDI] grandiose loaded but send() not available — install rse/grandiose fork');
        console.warn('[NDI] Run: npm install github:rse/grandiose');
        this.available = false;
        return;
      }
      this.available = true;
      console.log('[NDI] grandiose loaded successfully - NDI output available');
    } catch (err) {
      console.warn('[NDI] grandiose not available - NDI output disabled');
      console.warn('[NDI] Install NDI SDK and run: npm install github:rse/grandiose');
      this.available = false;
    }
  }

  isAvailable() {
    return this.available;
  }

  async createSource(userId, displayName, isoIndex) {
    const safeName = displayName.replace(/[^a-zA-Z0-9_-]/g, '_');
    const sourceName = `${this.prefix}_ISO${isoIndex}_${safeName}`;

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
      return mockSource;
    }

    try {
      const sender = await this.grandiose.send({
        name: sourceName,
        clockVideo: false,
        clockAudio: false,
      });

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
      };

      this.sources.set(userId, source);
      this.emit('source-created', { userId, sourceName, mock: false });
      console.log(`[NDI] Created source: ${sourceName}`);
      return source;
    } catch (err) {
      console.error(`[NDI] Failed to create source for ${displayName}:`, err);
      this.emit('source-error', { userId, error: err.message });
      return null;
    }
  }

  destroySource(userId) {
    const source = this.sources.get(userId);
    if (!source) return;

    if (!source.mock) {
      try {
        if (source.sender && typeof source.sender.destroy === 'function') {
          source.sender.destroy();
        }
      } catch (err) {
        console.error(`[NDI] Error destroying source for ${source.displayName}:`, err);
      }
    }

    this.sources.delete(userId);
    this.emit('source-destroyed', { userId, displayName: source.displayName });
    console.log(`[NDI] Destroyed source for ${source.displayName}`);
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
        frameRateN: this.settings.video.frameRate * 1000,
        frameRateD: 1000,
        pictureAspectRatio: width / height,
        frameFormatType: 1,
        fourCC: this.grandiose.FOURCC_RGBA,
        lineStrideBytes: width * 4,
        data: frameBuffer,
      });
    } catch (err) {
      if (source.framesSent % 300 === 1) {
        console.error(`[NDI] Error sending video for ${source.displayName}:`, err.message);
      }
    }
  }

  sendAudioData(userId, audioBuffer, sampleRate, channels) {
    const source = this.sources.get(userId);
    if (!source || !source.active) return;

    if (source.mock || !source.sender) return;

    const ch = channels || this.settings.audio.channels;
    const sr = sampleRate || this.settings.audio.sampleRate;

    try {
      const bytesPerSample = 2;
      const noSamples = Math.floor(audioBuffer.length / (bytesPerSample * ch));
      if (noSamples <= 0) return;

      source.sender.audio({
        sampleRate: sr,
        noChannels: ch,
        noSamples: noSamples,
        channelStrideBytes: noSamples * bytesPerSample,
        data: audioBuffer,
      });
    } catch (err) {
      if (!source._audioErrorLogged) {
        console.error(`[NDI] Error sending audio for ${source.displayName}:`, err.message);
        source._audioErrorLogged = true;
      }
    }
  }

  toggleSource(userId, active) {
    const source = this.sources.get(userId);
    if (source) {
      source.active = active;
      this.emit('source-toggled', { userId, active });
    }
  }

  async startTestPattern() {
    if (this.testPatternActive) return;
    this.testPatternActive = true;

    const width = 1920;
    const height = 1080;
    const frameBuffer = Buffer.alloc(width * height * 4);

    for (let i = 0; i < 8; i++) {
      const testId = `test_${i + 1}`;
      await this.createSource(testId, `TestPattern_${i + 1}`, i + 1);
    }

    let frameCount = 0;
    this.testPatternInterval = setInterval(() => {
      frameCount++;
      this._generateTestFrame(frameBuffer, width, height, frameCount);

      for (let i = 0; i < 8; i++) {
        const testId = `test_${i + 1}`;
        this.sendVideoFrame(testId, frameBuffer, width, height);
      }
    }, 1000 / 30);

    this.emit('test-pattern-started');
    console.log('[NDI] Test pattern started - 8 sources created');
  }

  stopTestPattern() {
    if (!this.testPatternActive) return;
    this.testPatternActive = false;

    if (this.testPatternInterval) {
      clearInterval(this.testPatternInterval);
      this.testPatternInterval = null;
    }

    for (let i = 0; i < 8; i++) {
      this.destroySource(`test_${i + 1}`);
    }

    this.emit('test-pattern-stopped');
    console.log('[NDI] Test pattern stopped');
  }

  _generateTestFrame(buffer, width, height, frameCount) {
    const barWidth = Math.floor(width / 8);
    const colors = [
      [255, 255, 255], [255, 255, 0], [0, 255, 255], [0, 255, 0],
      [255, 0, 255], [255, 0, 0], [0, 0, 255], [0, 0, 0],
    ];

    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const barIndex = Math.min(Math.floor(x / barWidth), 7);
        const offset = (y * width + x) * 4;
        const pulse = Math.sin(frameCount * 0.05) * 20;
        buffer[offset] = Math.max(0, Math.min(255, colors[barIndex][0] + pulse));
        buffer[offset + 1] = Math.max(0, Math.min(255, colors[barIndex][1] + pulse));
        buffer[offset + 2] = Math.max(0, Math.min(255, colors[barIndex][2] + pulse));
        buffer[offset + 3] = 255;
      }
    }
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
      testPatternActive: this.testPatternActive,
      sourceCount: this.sources.size,
      sources,
    };
  }

  destroyAll() {
    this.stopTestPattern();
    for (const [userId] of this.sources) {
      this.destroySource(userId);
    }
  }

  destroy() {
    this.destroyAll();
    this.removeAllListeners();
  }
}

module.exports = { NDIManager };
