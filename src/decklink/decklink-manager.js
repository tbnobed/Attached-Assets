const EventEmitter = require('events');
const path = require('path');

let decklink = null;
let decklinkAvailable = false;

try {
  decklink = require(path.join(__dirname, '..', '..', 'decklink-output-addon'));
  if (decklink && decklink.available) {
    decklinkAvailable = true;
    console.log('[DeckLink] Loaded decklink-output-addon');
  } else {
    console.warn('[DeckLink] Addon loaded but not available:', decklink ? decklink.loadError : 'null');
  }
} catch (err) {
  console.warn('[DeckLink] Failed to load decklink-output-addon:', err.message);
}

const DISPLAY_MODES = {
  '1080i50':  { label: '1080i 50',    mode: 'bmdModeHD1080i50',    width: 1920, height: 1080, fps: 25 },
  '1080i5994': { label: '1080i 59.94', mode: 'bmdModeHD1080i5994',  width: 1920, height: 1080, fps: 29.97 },
  '1080p24':  { label: '1080p 24',    mode: 'bmdModeHD1080p24',    width: 1920, height: 1080, fps: 24 },
  '1080p25':  { label: '1080p 25',    mode: 'bmdModeHD1080p25',    width: 1920, height: 1080, fps: 25 },
  '1080p2997': { label: '1080p 29.97', mode: 'bmdModeHD1080p2997',  width: 1920, height: 1080, fps: 29.97 },
  '1080p30':  { label: '1080p 30',    mode: 'bmdModeHD1080p30',    width: 1920, height: 1080, fps: 30 },
  '720p50':   { label: '720p 50',     mode: 'bmdModeHD720p50',     width: 1280, height: 720,  fps: 50 },
  '720p5994': { label: '720p 59.94',  mode: 'bmdModeHD720p5994',   width: 1280, height: 720,  fps: 59.94 },
  '720p60':   { label: '720p 60',     mode: 'bmdModeHD720p60',     width: 1280, height: 720,  fps: 60 },
};

const DEFAULT_MODE = '1080p2997';

class DeckLinkManager extends EventEmitter {
  constructor(settings) {
    super();
    this.settings = settings;
    this.devices = [];
    this.outputs = new Map();
    this.assignments = new Map();
    this.available = false;
    this._init();
  }

  _init() {
    if (!decklinkAvailable) {
      console.warn('[DeckLink] decklink-output-addon not available — DeckLink output disabled');
      return;
    }

    try {
      if (!decklink.isAvailable()) {
        console.warn('[DeckLink] DeckLink drivers not installed or no devices found');
        console.warn('[DeckLink] Install Blackmagic Desktop Video from https://www.blackmagicdesign.com/support');
        return;
      }

      const devices = decklink.getDevices();
      this.devices = (devices || []).map((d, i) => ({
        index: d.index !== undefined ? d.index : i,
        name: d.name || `Output ${i + 1}`,
        modelName: d.modelName || d.name || `DeckLink ${i}`,
        displayName: d.name || `Output ${i + 1}`,
        hasOutput: d.hasOutput !== false,
      }));
      this.available = true;
      console.log(`[DeckLink] Initialized — ${this.devices.length} device(s) found`);
    } catch (err) {
      console.warn('[DeckLink] Error initializing:', err.message);
      this.available = false;
    }
  }

  isAvailable() {
    return this.available;
  }

  _refreshDevices() {
    if (!decklinkAvailable) return;

    try {
      const devices = decklink.getDevices();
      this.devices = (devices || []).map((d, i) => ({
        index: d.index !== undefined ? d.index : i,
        name: d.name || `Output ${i + 1}`,
        modelName: d.modelName || d.name || `DeckLink ${i}`,
        displayName: d.name || `Output ${i + 1}`,
        hasOutput: d.hasOutput !== false,
      }));
    } catch (err) {
      console.warn('[DeckLink] Error enumerating devices:', err.message);
      this.devices = [];
    }
  }

  getDevices() {
    if (this.devices.length === 0) {
      this._refreshDevices();
    }
    return this.devices;
  }

  getDisplayModes() {
    const modes = {};
    for (const [key, val] of Object.entries(DISPLAY_MODES)) {
      modes[key] = val.label;
    }
    return modes;
  }

  async startOutput(deviceIndex, modeKey) {
    if (!this.available) {
      return { success: false, error: 'DeckLink not available' };
    }

    if (this.outputs.has(deviceIndex)) {
      return { success: false, error: `Device ${deviceIndex} already has an active output` };
    }

    const modeInfo = DISPLAY_MODES[modeKey] || DISPLAY_MODES[DEFAULT_MODE];
    const modeConst = decklink[modeInfo.mode];

    if (modeConst === undefined || modeConst === 0) {
      return { success: false, error: `Display mode ${modeInfo.mode} not available` };
    }

    try {
      const result = await decklink.openOutput({
        deviceIndex: deviceIndex,
        displayMode: modeConst,
        pixelFormat: decklink.bmdFormat8BitYUV,
        audioSampleRate: 48000,
        audioSampleType: 16,
        audioChannels: 2,
      });

      const exactFps = modeInfo.fps;

      const output = {
        deviceIndex,
        modeKey,
        modeInfo,
        handle: result.handle,
        assignedUserId: null,
        framesSent: 0,
        active: true,
        startedAt: Date.now(),
        _uyvyBuf: null,
        _audioPendingBufs: [],
        _audioRemainder: null,
        _audioTotalIn: 0,
        _audioTotalOut: 0,
        _exactFps: exactFps,
        _audioLogCount: 0,
        _pumpRunning: false,
        _hasFrame: false,
      };

      this.outputs.set(deviceIndex, output);
      this.emit('output-started', {
        deviceIndex,
        mode: modeInfo.label,
        width: modeInfo.width,
        height: modeInfo.height,
      });

      console.log(`[DeckLink] Output started on device ${deviceIndex}: ${modeInfo.label} (fps=${exactFps}, handle=${result.handle})`);
      return { success: true };
    } catch (err) {
      console.error(`[DeckLink] Failed to start output on device ${deviceIndex}:`, err.message);
      return { success: false, error: err.message };
    }
  }

  stopOutput(deviceIndex) {
    deviceIndex = typeof deviceIndex === 'string' ? parseInt(deviceIndex, 10) : deviceIndex;
    const output = this.outputs.get(deviceIndex);
    if (!output) return;

    this._stopFramePump(output);

    try {
      if (output.handle !== undefined && output.handle !== null) {
        decklink.closeOutput(output.handle);
      }
    } catch (err) {
      console.warn(`[DeckLink] Error stopping output on device ${deviceIndex}:`, err.message);
    }

    const userId = output.assignedUserId;
    if (userId !== null) {
      this.assignments.delete(userId);
    }

    this.outputs.delete(deviceIndex);
    this.emit('output-stopped', { deviceIndex });
    console.log(`[DeckLink] Output stopped on device ${deviceIndex}`);
  }

  assignParticipant(userId, deviceIndex) {
    deviceIndex = typeof deviceIndex === 'string' ? parseInt(deviceIndex, 10) : deviceIndex;
    const output = this.outputs.get(deviceIndex);
    if (!output) {
      return { success: false, error: `No active output on device ${deviceIndex}` };
    }

    const prevDevice = this.assignments.get(userId);
    if (prevDevice !== undefined) {
      const prevOutput = this.outputs.get(prevDevice);
      if (prevOutput) {
        prevOutput.assignedUserId = null;
      }
    }

    if (output.assignedUserId !== null && output.assignedUserId !== userId) {
      this.assignments.delete(output.assignedUserId);
    }

    output.assignedUserId = userId;
    this.assignments.set(userId, deviceIndex);

    output._audioPendingBufs = [];
    output._audioRemainder = null;
    output._audioTotalIn = 0;
    output._audioTotalOut = 0;
    output._audioLogCount = 0;
    output._hasFrame = false;
    console.log(`[DeckLink] Audio buffer flushed for device ${deviceIndex} on participant assign`);

    this.emit('assignment-changed', { userId, deviceIndex });
    console.log(`[DeckLink] Assigned participant ${userId} to device ${deviceIndex}`);
    return { success: true };
  }

  unassignParticipant(userId) {
    const deviceIndex = this.assignments.get(userId);
    if (deviceIndex === undefined) return;

    const output = this.outputs.get(deviceIndex);
    if (output) {
      output.assignedUserId = null;
    }

    this.assignments.delete(userId);
    this.emit('assignment-changed', { userId, deviceIndex: null });
    console.log(`[DeckLink] Unassigned participant ${userId}`);
  }

  sendVideoFrame(userId, frameBuffer, width, height) {
    const deviceIndex = this.assignments.get(userId);
    if (deviceIndex === undefined) {
      if (!this._missLoggedUsers) this._missLoggedUsers = new Set();
      if (!this._missLoggedUsers.has(userId)) {
        this._missLoggedUsers.add(userId);
        console.log(`[DeckLink] sendVideoFrame: no assignment for userId=${userId} (type=${typeof userId}), assignments:`, [...this.assignments.entries()]);
      }
      return;
    }

    const output = this.outputs.get(deviceIndex);
    if (!output || !output.active || output.handle === undefined) return;

    const outW = output.modeInfo.width;
    const outH = output.modeInfo.height;
    const uyvySize = outW * outH * 2;

    if (!output._uyvyBuf || output._uyvyBuf.length !== uyvySize) {
      output._uyvyBuf = Buffer.alloc(uyvySize);
    }

    this._bgraToUyvy(frameBuffer, width, height, output._uyvyBuf, outW, outH);
    output._hasFrame = true;

    if (!output._pumpRunning) {
      this._startFramePump(deviceIndex, output);
    }
  }

  _startFramePump(deviceIndex, output) {
    if (output._pumpRunning) return;
    output._pumpRunning = true;

    console.log(`[DeckLink] Starting sync frame pump dev=${deviceIndex}`);
    this._pumpFrameLoop(deviceIndex, output);
  }

  _stopFramePump(output) {
    output._pumpRunning = false;
  }

  async _pumpFrameLoop(deviceIndex, output) {
    while (output._pumpRunning && output.active && output.handle !== undefined) {
      if (!output._uyvyBuf || !output._hasFrame) {
        await new Promise(r => setTimeout(r, 5));
        continue;
      }

      try {
        const samplesPerFrame = Math.round(48000 / output._exactFps);

        const parts = [];
        if (output._audioRemainder) {
          parts.push(output._audioRemainder);
          output._audioRemainder = null;
        }
        for (const buf of output._audioPendingBufs) {
          parts.push(buf);
        }
        output._audioPendingBufs = [];

        const allAudio = parts.length > 0 ? Buffer.concat(parts) : Buffer.alloc(0);
        const bytesNeeded = samplesPerFrame * 4;

        let audioBuf;
        if (allAudio.length >= bytesNeeded) {
          audioBuf = Buffer.alloc(bytesNeeded);
          allAudio.copy(audioBuf, 0, 0, bytesNeeded);
          if (allAudio.length > bytesNeeded) {
            const rem = Buffer.alloc(allAudio.length - bytesNeeded);
            allAudio.copy(rem, 0, bytesNeeded);
            output._audioRemainder = rem;
          }
        } else if (allAudio.length > 0) {
          audioBuf = Buffer.alloc(bytesNeeded);
          allAudio.copy(audioBuf, 0, 0, allAudio.length);
        } else {
          audioBuf = Buffer.alloc(bytesNeeded);
        }
        output._audioTotalOut += samplesPerFrame;

        const result = await decklink.displayFrame(output.handle, output._uyvyBuf, audioBuf);

        output.framesSent++;
        const shouldLog = output.framesSent <= 5 || output.framesSent % 300 === 0 || output.framesSent === 100 || output.framesSent === 200;
        if (shouldLog) {
          const remLen = output._audioRemainder ? output._audioRemainder.length / 4 : 0;
          const written = result ? result.samplesWritten : '?';
          console.log(`[DeckLink] Frame #${output.framesSent} dev=${deviceIndex} audio=${audioBuf.length}B samples=${samplesPerFrame} written=${written} rem=${remLen} totalOut=${output._audioTotalOut}`);
        }
      } catch (err) {
        if (!output._videoErrorLogged) {
          console.error(`[DeckLink] Error displaying frame on device ${deviceIndex}:`, err.message);
          output._videoErrorLogged = true;
        }
        await new Promise(r => setTimeout(r, 100));
      }
    }
    console.log(`[DeckLink] Frame pump stopped dev=${deviceIndex}`);
  }

  sendAudioData(userId, audioBuffer, sampleRate, channels) {
    const deviceIndex = this.assignments.get(userId);
    if (deviceIndex === undefined) return;

    const output = this.outputs.get(deviceIndex);
    if (!output || !output.active || output.handle === undefined) return;

    try {
      const srcCh = channels || 1;
      const bytesPerSample = 2;
      const noSamples = Math.floor(audioBuffer.length / (bytesPerSample * srcCh));
      if (noSamples <= 0) return;

      const stereoBuf = Buffer.alloc(noSamples * 4);
      for (let s = 0; s < noSamples; s++) {
        const sample = audioBuffer.readInt16LE(s * srcCh * bytesPerSample);
        stereoBuf.writeInt16LE(sample, s * 4);
        stereoBuf.writeInt16LE(sample, s * 4 + 2);
      }

      output._audioPendingBufs.push(stereoBuf);
      output._audioTotalIn += noSamples;

      const maxBufs = Math.ceil(output._exactFps / 10) * 3;
      if (output._audioPendingBufs.length > maxBufs) {
        output._audioPendingBufs = output._audioPendingBufs.slice(-maxBufs);
      }

      output._audioLogCount++;
      if (output._audioLogCount <= 3 || output._audioLogCount % 2000 === 0) {
        const pending = output._audioPendingBufs.reduce((s, b) => s + b.length, 0) / 4;
        console.log(`[DeckLink] Audio in #${output._audioLogCount} dev=${deviceIndex}: ${noSamples} samples sr=${sampleRate} ch=${channels}, pending=${pending} bufs=${output._audioPendingBufs.length}, totalIn=${output._audioTotalIn} totalOut=${output._audioTotalOut}`);
      }
    } catch (err) {
      if (!output._audioErrorLogged) {
        console.error(`[DeckLink] Error buffering audio for device ${deviceIndex}:`, err.message);
        output._audioErrorLogged = true;
      }
    }
  }

  _bgraToUyvy(srcBuf, srcW, srcH, dstBuf, dstW, dstH) {
    for (let y = 0; y < dstH; y++) {
      const srcY = Math.min(Math.floor(y * srcH / dstH), srcH - 1);

      for (let x = 0; x < dstW; x += 2) {
        const srcX0 = Math.min(Math.floor(x * srcW / dstW), srcW - 1);
        const srcX1 = Math.min(Math.floor((x + 1) * srcW / dstW), srcW - 1);

        const srcOff0 = (srcY * srcW + srcX0) * 4;
        const srcOff1 = (srcY * srcW + srcX1) * 4;

        const b0 = srcBuf[srcOff0];
        const g0 = srcBuf[srcOff0 + 1];
        const r0 = srcBuf[srcOff0 + 2];

        const b1 = srcBuf[srcOff1];
        const g1 = srcBuf[srcOff1 + 1];
        const r1 = srcBuf[srcOff1 + 2];

        const y0 = Math.max(16, Math.min(235, ((66 * r0 + 129 * g0 + 25 * b0 + 128) >> 8) + 16));
        const y1 = Math.max(16, Math.min(235, ((66 * r1 + 129 * g1 + 25 * b1 + 128) >> 8) + 16));

        const avgR = (r0 + r1) >> 1;
        const avgG = (g0 + g1) >> 1;
        const avgB = (b0 + b1) >> 1;

        const u = Math.max(16, Math.min(240, ((-38 * avgR - 74 * avgG + 112 * avgB + 128) >> 8) + 128));
        const v = Math.max(16, Math.min(240, ((112 * avgR - 94 * avgG - 18 * avgB + 128) >> 8) + 128));

        const dstOff = (y * dstW + x) * 2;
        dstBuf[dstOff]     = u;
        dstBuf[dstOff + 1] = y0;
        dstBuf[dstOff + 2] = v;
        dstBuf[dstOff + 3] = y1;
      }
    }
  }

  getStatus() {
    const outputList = {};
    for (const [deviceIndex, output] of this.outputs) {
      outputList[deviceIndex] = {
        deviceIndex,
        mode: output.modeInfo.label,
        assignedUserId: output.assignedUserId,
        framesSent: output.framesSent,
        active: output.active,
      };
    }

    const assignmentList = {};
    for (const [userId, deviceIndex] of this.assignments) {
      assignmentList[userId] = deviceIndex;
    }

    return {
      available: this.available,
      deviceCount: this.devices.length,
      devices: this.devices,
      outputs: outputList,
      assignments: assignmentList,
    };
  }

  stopAll() {
    for (const [deviceIndex] of this.outputs) {
      this.stopOutput(deviceIndex);
    }
    this.assignments.clear();
  }

  destroy() {
    this.stopAll();
    this.removeAllListeners();
  }
}

module.exports = { DeckLinkManager };
