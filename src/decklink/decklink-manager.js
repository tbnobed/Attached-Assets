const EventEmitter = require('events');
const path = require('path');

let macadam = null;
let macadamAvailable = false;

function bmCodeToInt(s) {
  return ((s.charCodeAt(0) << 24) | (s.charCodeAt(1) << 16) | (s.charCodeAt(2) << 8) | s.charCodeAt(3)) >>> 0;
}

const MACADAM_CONSTANTS = {
  bmdModeNTSC:           bmCodeToInt('ntsc'),
  bmdModeNTSC2398:       bmCodeToInt('nt23'),
  bmdModePAL:            bmCodeToInt('pal '),
  bmdModeNTSCp:          bmCodeToInt('ntsp'),
  bmdModePALp:           bmCodeToInt('palp'),
  bmdModeHD1080p2398:    bmCodeToInt('23ps'),
  bmdModeHD1080p24:      bmCodeToInt('24ps'),
  bmdModeHD1080p25:      bmCodeToInt('Hp25'),
  bmdModeHD1080p2997:    bmCodeToInt('Hp29'),
  bmdModeHD1080p30:      bmCodeToInt('Hp30'),
  bmdModeHD1080i50:      bmCodeToInt('Hi50'),
  bmdModeHD1080i5994:    bmCodeToInt('Hi59'),
  bmdModeHD1080i6000:    bmCodeToInt('Hi60'),
  bmdModeHD1080p50:      bmCodeToInt('Hp50'),
  bmdModeHD1080p5994:    bmCodeToInt('Hp59'),
  bmdModeHD1080p6000:    bmCodeToInt('Hp60'),
  bmdModeHD720p50:       bmCodeToInt('hp50'),
  bmdModeHD720p5994:     bmCodeToInt('hp59'),
  bmdModeHD720p60:       bmCodeToInt('hp60'),
  bmdFormat8BitYUV:      bmCodeToInt('2vuy'),
  bmdFormat8BitBGRA:     bmCodeToInt('BGRA'),
  bmdFormat10BitYUV:     bmCodeToInt('v210'),
  bmdAudioSampleRate48kHz: 48000,
  bmdAudioSampleType16bitInteger: 16,
  bmdAudioSampleType32bitInteger: 32,
};

try {
  macadam = require('macadam');
  macadamAvailable = true;
  console.log('[DeckLink] Loaded macadam via npm package');
} catch (err) {
  console.warn('[DeckLink] require("macadam") failed:', err.message);
  try {
    const fs = require('fs');
    const nativePath = path.join(
      __dirname, '..', '..', 'node_modules', 'macadam', 'build', 'Release', 'macadam.node'
    );
    if (fs.existsSync(nativePath)) {
      console.log('[DeckLink] Found native binary at:', nativePath);
      const nativeBinding = require(nativePath);
      macadam = nativeBinding;
      for (const [k, v] of Object.entries(MACADAM_CONSTANTS)) {
        if (macadam[k] === undefined) {
          macadam[k] = v;
        }
      }
      macadamAvailable = true;
      console.log('[DeckLink] Loaded macadam native binding with constants applied');
    } else {
      console.warn('[DeckLink] Native binary not found at:', nativePath);
    }
  } catch (err2) {
    console.warn('[DeckLink] Direct native load failed:', err2.message);
    if (err2.message.includes('DeckLinkAPI')) {
      console.warn('[DeckLink] DeckLink drivers may not be installed. Install Blackmagic Desktop Video from https://www.blackmagicdesign.com/support');
    }
  }
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
    if (!macadamAvailable) {
      console.warn('[DeckLink] macadam not available — DeckLink output disabled');
      console.warn('[DeckLink] Install with: npm install macadam');
      return;
    }

    const nativePath = path.join(
      __dirname, '..', '..', 'node_modules', 'macadam', 'build', 'Release', 'macadam.node'
    );

    const fs = require('fs');
    const { spawnSync } = require('child_process');

    const shimPath = path.join(__dirname, 'libdecklink-shim.dylib');
    const shimExists = fs.existsSync(shimPath);

    const testScript = `
      try {
        const m = require(${JSON.stringify(nativePath)});
        const devices = m.getDeviceInfo ? m.getDeviceInfo() : [];
        process.stdout.write(JSON.stringify({ ok: true, count: devices ? devices.length : 0, devices: devices || [] }));
      } catch (e) {
        process.stdout.write(JSON.stringify({ ok: false, error: e.message }));
      }
    `;

    const probeEnv = { ...process.env, ELECTRON_RUN_AS_NODE: '1' };
    if (shimExists) {
      probeEnv.DYLD_INSERT_LIBRARIES = shimPath;
      console.log('[DeckLink] Using API compatibility shim:', shimPath);
    }

    const result = spawnSync(process.execPath, ['-e', testScript], {
      timeout: 5000,
      encoding: 'utf8',
      env: probeEnv,
    });

    if (result.status !== 0 || result.signal) {
      console.warn(`[DeckLink] DeckLink API probe crashed (signal=${result.signal || 'none'})`);
      console.warn('[DeckLink] Blackmagic Desktop Video may not be installed or is incompatible');
      console.warn('[DeckLink] Install from https://www.blackmagicdesign.com/support');
      this.available = false;
      return;
    }

    try {
      const probe = JSON.parse(result.stdout);
      if (!probe.ok) {
        console.warn('[DeckLink] DeckLink API probe failed:', probe.error);
        this.available = false;
        return;
      }
      this.devices = (probe.devices || []).map((d, i) => ({
        index: i,
        name: d.displayName || d.modelName || `Output ${i + 1}`,
        modelName: d.modelName || `DeckLink ${i}`,
        displayName: d.displayName || d.modelName || `Output ${i + 1}`,
        hasOutput: true,
      }));
      this.available = true;
      console.log(`[DeckLink] Initialized — ${this.devices.length} device(s) found`);
    } catch (err) {
      console.warn('[DeckLink] Failed to parse DeckLink probe result:', err.message);
      this.available = false;
    }
  }

  isAvailable() {
    return this.available;
  }

  _refreshDevices() {
    if (!macadamAvailable) return;

    try {
      const info = macadam.getDeviceInfo();
      this.devices = (info || []).map((d, i) => ({
        index: i,
        name: d.displayName || d.modelName || `Output ${i + 1}`,
        modelName: d.modelName || `DeckLink ${i}`,
        displayName: d.displayName || d.modelName || `Output ${i + 1}`,
        hasOutput: true,
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
    const modeConst = macadam[modeInfo.mode];

    if (modeConst === undefined) {
      return { success: false, error: `Display mode ${modeInfo.mode} not supported by this macadam version` };
    }

    try {
      const playback = await macadam.playback({
        deviceIndex: deviceIndex,
        displayMode: modeConst,
        pixelFormat: macadam.bmdFormat8BitYUV,
        channels: 2,
        sampleRate: macadam.bmdAudioSampleRate48kHz,
        sampleType: macadam.bmdAudioSampleType16bitInteger,
      });

      const output = {
        deviceIndex,
        modeKey,
        modeInfo,
        playback,
        assignedUserId: null,
        framesSent: 0,
        active: true,
        startedAt: Date.now(),
        _lastFrameTime: 0,
        _frameInterval: 1000 / modeInfo.fps,
        _scaledFrameBuf: null,
        _uyvyBuf: null,
      };

      this.outputs.set(deviceIndex, output);
      this.emit('output-started', {
        deviceIndex,
        mode: modeInfo.label,
        width: modeInfo.width,
        height: modeInfo.height,
      });

      console.log(`[DeckLink] Output started on device ${deviceIndex}: ${modeInfo.label}`);
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

    try {
      if (output.playback && typeof output.playback.stop === 'function') {
        output.playback.stop();
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
    if (!output || !output.active || !output.playback) return;

    const now = Date.now();
    if (now - output._lastFrameTime < output._frameInterval * 0.8) return;
    output._lastFrameTime = now;

    try {
      const outW = output.modeInfo.width;
      const outH = output.modeInfo.height;
      const uyvySize = outW * outH * 2;

      if (!output._uyvyBuf || output._uyvyBuf.length !== uyvySize) {
        output._uyvyBuf = Buffer.alloc(uyvySize);
      }

      this._bgraToUyvy(frameBuffer, width, height, output._uyvyBuf, outW, outH);

      let audioBuf = null;
      if (output._audioPendingBufs && output._audioPendingBufs.length > 0) {
        const totalLen = output._audioPendingBufs.reduce((sum, b) => sum + b.length, 0);
        audioBuf = Buffer.concat(output._audioPendingBufs, totalLen);
        output._audioPendingBufs = [];
      }

      if (audioBuf) {
        if (!output._audioSampleTime) output._audioSampleTime = 0;
        output.playback.schedule({
          video: output._uyvyBuf,
          audio: audioBuf,
          time: output._audioSampleTime,
        });
        output._audioSampleTime += Math.floor(audioBuf.length / 4);
      } else {
        output.playback.displayFrame(output._uyvyBuf).catch(err => {
          if (!output._videoErrorLogged) {
            console.error(`[DeckLink] displayFrame error on device ${deviceIndex}:`, err.message);
            output._videoErrorLogged = true;
          }
        });
      }

      output.framesSent++;
      if (output.framesSent <= 5 || output.framesSent % 300 === 0) {
        console.log(`[DeckLink] Sent frame #${output.framesSent} to device ${deviceIndex} (${outW}x${outH} UYVY) audio=${audioBuf ? audioBuf.length : 0}B`);
      }
    } catch (err) {
      if (!output._videoErrorLogged) {
        console.error(`[DeckLink] Error sending frame to device ${deviceIndex}:`, err.message);
        output._videoErrorLogged = true;
      }
    }
  }

  sendAudioData(userId, audioBuffer, sampleRate, channels) {
    const deviceIndex = this.assignments.get(userId);
    if (deviceIndex === undefined) return;

    const output = this.outputs.get(deviceIndex);
    if (!output || !output.active || !output.playback) return;

    try {
      const srcCh = channels || 1;
      const bytesPerSample = 2;
      const noSamples = Math.floor(audioBuffer.length / (bytesPerSample * srcCh));
      if (noSamples <= 0) return;

      let stereoBuf;
      if (srcCh === 1) {
        stereoBuf = Buffer.alloc(noSamples * 2 * bytesPerSample);
        for (let s = 0; s < noSamples; s++) {
          const sample = audioBuffer.readInt16LE(s * 2);
          stereoBuf.writeInt16LE(sample, s * 4);
          stereoBuf.writeInt16LE(sample, s * 4 + 2);
        }
      } else {
        stereoBuf = audioBuffer;
      }

      if (!output._audioPendingBufs) output._audioPendingBufs = [];
      output._audioPendingBufs.push(stereoBuf);

      if (!output._audioBufferedCount) output._audioBufferedCount = 0;
      output._audioBufferedCount++;
      if (output._audioBufferedCount <= 3 || output._audioBufferedCount % 1000 === 0) {
        console.log(`[DeckLink] Audio buffered #${output._audioBufferedCount}: device=${deviceIndex} samples=${noSamples} pending=${output._audioPendingBufs.length}`);
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
