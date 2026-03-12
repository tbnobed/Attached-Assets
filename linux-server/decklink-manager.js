'use strict';

const EventEmitter = require('events');
const path = require('path');

let decklink = null;
let decklinkAvailable = false;

try {
  decklink = require(path.join(__dirname, 'decklink-addon'));
  if (decklink && decklink.available) {
    decklinkAvailable = true;
    console.log('[DeckLink] Loaded decklink-addon');
  } else {
    console.warn('[DeckLink] Addon loaded but not available:', decklink ? decklink.loadError : 'null');
  }
} catch (err) {
  console.warn('[DeckLink] Failed to load decklink-addon:', err.message);
}

const DISPLAY_MODES = {
  '1080i50':   { label: '1080i 50',     mode: 'bmdModeHD1080i50',   width: 1920, height: 1080, fps: 25 },
  '1080i5994': { label: '1080i 59.94',  mode: 'bmdModeHD1080i5994', width: 1920, height: 1080, fps: 29.97 },
  '1080p24':   { label: '1080p 24',     mode: 'bmdModeHD1080p24',   width: 1920, height: 1080, fps: 24 },
  '1080p25':   { label: '1080p 25',     mode: 'bmdModeHD1080p25',   width: 1920, height: 1080, fps: 25 },
  '1080p2997': { label: '1080p 29.97',  mode: 'bmdModeHD1080p2997', width: 1920, height: 1080, fps: 29.97 },
  '1080p30':   { label: '1080p 30',     mode: 'bmdModeHD1080p30',   width: 1920, height: 1080, fps: 30 },
  '720p50':    { label: '720p 50',      mode: 'bmdModeHD720p50',    width: 1280, height: 720,  fps: 50 },
  '720p5994':  { label: '720p 59.94',   mode: 'bmdModeHD720p5994',  width: 1280, height: 720,  fps: 59.94 },
  '720p60':    { label: '720p 60',      mode: 'bmdModeHD720p60',    width: 1280, height: 720,  fps: 60 },
};

const DEFAULT_MODE = '1080p2997';

class DeckLinkManager extends EventEmitter {
  constructor() {
    super();
    this.devices = [];
    this.outputs = new Map();
    this.assignments = new Map();
    this.available = false;
    this._init();
  }

  _init() {
    if (!decklinkAvailable) {
      console.warn('[DeckLink] decklink-addon not available — DeckLink output disabled');
      return;
    }

    try {
      if (!decklink.isAvailable()) {
        console.warn('[DeckLink] DeckLink drivers not installed or no devices found');
        return;
      }

      const devices = decklink.enumerateDevices();
      this.devices = (devices || []).map((d, i) => ({
        index: d.index !== undefined ? d.index : i,
        name: d.name || `Output ${i + 1}`,
        displayName: d.name || `Output ${i + 1}`,
        supportsOutput: d.supportsOutput !== false,
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

  refreshDevices() {
    if (!decklinkAvailable) return;
    try {
      const devices = decklink.enumerateDevices();
      this.devices = (devices || []).map((d, i) => ({
        index: d.index !== undefined ? d.index : i,
        name: d.name || `Output ${i + 1}`,
        displayName: d.name || `Output ${i + 1}`,
        supportsOutput: d.supportsOutput !== false,
      }));
    } catch (err) {
      console.warn('[DeckLink] Error enumerating devices:', err.message);
      this.devices = [];
    }
  }

  getDevices() {
    if (this.devices.length === 0) {
      this.refreshDevices();
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
      const result = await decklink.openDevice({
        deviceIndex: deviceIndex,
        displayMode: modeConst,
        pixelFormat: decklink.bmdFormat8BitYUV,
        audioSampleRate: 48000,
        audioSampleType: 16,
        audioChannels: 2,
      });

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
        _exactFps: modeInfo.fps,
        _pumpRunning: false,
        _hasFrame: false,
      };

      this.outputs.set(deviceIndex, output);
      this.emit('output-started', { deviceIndex, mode: modeInfo.label, width: modeInfo.width, height: modeInfo.height });
      console.log(`[DeckLink] Output started on device ${deviceIndex}: ${modeInfo.label} (handle=${result.handle})`);
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

    output._pumpRunning = false;
    output.active = false;

    try {
      if (output.handle !== undefined && output.handle !== null) {
        decklink.closeDevice(output.handle);
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

  async assignParticipant(userId, deviceIndex) {
    deviceIndex = typeof deviceIndex === 'string' ? parseInt(deviceIndex, 10) : deviceIndex;
    let output = this.outputs.get(deviceIndex);
    if (!output) {
      const device = this.devices.find(d => d.index === deviceIndex);
      if (!device || !device.supportsOutput) {
        return { success: false, error: `Device ${deviceIndex} not found or does not support output` };
      }
      console.log(`[DeckLink] Auto-starting output on device ${deviceIndex} for assignment`);
      const startResult = await this.startOutput(deviceIndex);
      if (!startResult.success) {
        return { success: false, error: `Failed to auto-start output: ${startResult.error}` };
      }
      output = this.outputs.get(deviceIndex);
      if (!output) {
        return { success: false, error: `Output state missing after start on device ${deviceIndex}` };
      }
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
    output._hasFrame = false;

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
    if (deviceIndex === undefined) return;

    const output = this.outputs.get(deviceIndex);
    if (!output || !output.active || output.handle === undefined) return;

    const outW = output.modeInfo.width;
    const outH = output.modeInfo.height;
    const uyvySize = outW * outH * 2;

    if (!output._uyvyBuf || output._uyvyBuf.length !== uyvySize) {
      output._uyvyBuf = Buffer.alloc(uyvySize);
    }

    const expectedBgraSize = width * height * 4;
    const expectedUyvySize = width * height * 2;

    if (frameBuffer.length === expectedUyvySize && width === outW && height === outH) {
      frameBuffer.copy(output._uyvyBuf);
    } else if (frameBuffer.length === expectedBgraSize) {
      this._bgraToUyvy(frameBuffer, width, height, output._uyvyBuf, outW, outH);
    } else if (frameBuffer.length === expectedUyvySize) {
      this._scaleUyvy(frameBuffer, width, height, output._uyvyBuf, outW, outH);
    } else {
      const copySize = Math.min(frameBuffer.length, uyvySize);
      frameBuffer.copy(output._uyvyBuf, 0, 0, copySize);
    }

    output._hasFrame = true;

    if (!output._pumpRunning) {
      this._startFramePump(deviceIndex, output);
    }
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
    } catch (err) {
      console.error(`[DeckLink] Error buffering audio for device ${deviceIndex}:`, err.message);
    }
  }

  _startFramePump(deviceIndex, output) {
    if (output._pumpRunning) return;
    output._pumpRunning = true;
    console.log(`[DeckLink] Starting frame pump dev=${deviceIndex}`);
    this._pumpFrameLoop(deviceIndex, output);
  }

  async _pumpFrameLoop(deviceIndex, output) {
    const maxSamplesPerWrite = Math.round(48000 / output._exactFps) * 2;
    const frameIntervalMs = 1000 / output._exactFps;
    let lastFrameTime = Date.now();

    while (output._pumpRunning && output.active && output.handle !== undefined) {
      const now = Date.now();
      const elapsed = now - lastFrameTime;

      if (elapsed < frameIntervalMs - 1) {
        await new Promise(r => setTimeout(r, Math.max(1, frameIntervalMs - elapsed - 1)));
        continue;
      }

      lastFrameTime = now;

      if (!output._uyvyBuf || !output._hasFrame) {
        continue;
      }

      try {
        await decklink.displayFrame(output.handle, output._uyvyBuf);

        const parts = [];
        if (output._audioRemainder) {
          parts.push(output._audioRemainder);
          output._audioRemainder = null;
        }
        for (const buf of output._audioPendingBufs) {
          parts.push(buf);
        }
        output._audioPendingBufs = [];

        let allAudio = parts.length > 0 ? Buffer.concat(parts) : null;

        if (allAudio && allAudio.length > 0) {
          const maxBytes = maxSamplesPerWrite * 4;
          let audioBuf;
          if (allAudio.length > maxBytes) {
            audioBuf = allAudio.subarray(0, maxBytes);
            output._audioRemainder = Buffer.from(allAudio.subarray(maxBytes));
          } else {
            audioBuf = allAudio;
          }

          const samplesSent = Math.floor(audioBuf.length / 4);
          output._audioTotalOut += samplesSent;

          await decklink.writeAudio(output.handle, audioBuf);
        }

        output.framesSent++;
        if (output.framesSent <= 3 || output.framesSent % 900 === 0) {
          console.log(`[DeckLink] Frame #${output.framesSent} dev=${deviceIndex} (${Math.round(1000 / (elapsed || 1))}fps)`);
        }
      } catch (err) {
        if (!output._errorLogged) {
          console.error(`[DeckLink] Error on device ${deviceIndex}:`, err.message);
          output._errorLogged = true;
        }
        await new Promise(r => setTimeout(r, 100));
      }
    }
    console.log(`[DeckLink] Frame pump stopped dev=${deviceIndex}`);
  }

  _bgraToUyvy(srcBuf, srcW, srcH, dstBuf, dstW, dstH) {
    for (let y = 0; y < dstH; y++) {
      const srcY = Math.min(Math.floor(y * srcH / dstH), srcH - 1);
      for (let x = 0; x < dstW; x += 2) {
        const srcX0 = Math.min(Math.floor(x * srcW / dstW), srcW - 1);
        const srcX1 = Math.min(Math.floor((x + 1) * srcW / dstW), srcW - 1);
        const srcOff0 = (srcY * srcW + srcX0) * 4;
        const srcOff1 = (srcY * srcW + srcX1) * 4;
        const b0 = srcBuf[srcOff0], g0 = srcBuf[srcOff0 + 1], r0 = srcBuf[srcOff0 + 2];
        const b1 = srcBuf[srcOff1], g1 = srcBuf[srcOff1 + 1], r1 = srcBuf[srcOff1 + 2];
        const y0 = Math.max(16, Math.min(235, ((66 * r0 + 129 * g0 + 25 * b0 + 128) >> 8) + 16));
        const y1 = Math.max(16, Math.min(235, ((66 * r1 + 129 * g1 + 25 * b1 + 128) >> 8) + 16));
        const avgR = (r0 + r1) >> 1, avgG = (g0 + g1) >> 1, avgB = (b0 + b1) >> 1;
        const u = Math.max(16, Math.min(240, ((-38 * avgR - 74 * avgG + 112 * avgB + 128) >> 8) + 128));
        const v = Math.max(16, Math.min(240, ((112 * avgR - 94 * avgG - 18 * avgB + 128) >> 8) + 128));
        const dstOff = (y * dstW + x) * 2;
        dstBuf[dstOff] = u;
        dstBuf[dstOff + 1] = y0;
        dstBuf[dstOff + 2] = v;
        dstBuf[dstOff + 3] = y1;
      }
    }
  }

  _scaleUyvy(srcBuf, srcW, srcH, dstBuf, dstW, dstH) {
    for (let y = 0; y < dstH; y++) {
      const srcY = Math.min(Math.floor(y * srcH / dstH), srcH - 1);
      const srcRowOff = srcY * srcW * 2;
      const dstRowOff = y * dstW * 2;
      for (let x = 0; x < dstW; x += 2) {
        const srcX = Math.min(Math.floor(x * srcW / dstW) & ~1, srcW - 2);
        const sOff = srcRowOff + srcX * 2;
        const dOff = dstRowOff + x * 2;
        dstBuf[dOff] = srcBuf[sOff];
        dstBuf[dOff + 1] = srcBuf[sOff + 1];
        dstBuf[dOff + 2] = srcBuf[sOff + 2];
        dstBuf[dOff + 3] = srcBuf[sOff + 3];
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
        startedAt: output.startedAt,
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

module.exports = { DeckLinkManager, DISPLAY_MODES, DEFAULT_MODE };
