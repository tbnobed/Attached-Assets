'use strict';

if (!process.env.UV_THREADPOOL_SIZE) {
  process.env.UV_THREADPOOL_SIZE = '16';
}
const UV_POOL = process.env.UV_THREADPOOL_SIZE;
const fs = require('fs');
const path = require('path');

const { Console } = require('console');
let _logStream = process.stderr;

try {
  const procUtils = require(path.join(__dirname, 'process-utils'));
  if (procUtils.available) {
    procUtils.redirectStdoutToDevNull();
    const savedStderrFd = procUtils.redirectStderrToDevNull();
    if (savedStderrFd >= 0) {
      _logStream = fs.createWriteStream(null, { fd: savedStderrFd });
      _logStream.on('error', () => {});
    }
  }
} catch (e) {
}

const _console = new Console({ stdout: _logStream, stderr: _logStream });
console.log = _console.log.bind(_console);
console.warn = _console.warn.bind(_console);
console.error = _console.error.bind(_console);
console.info = _console.info.bind(_console);
console.debug = _console.debug.bind(_console);

console.log('[Server] Native stdout/stderr redirected to /dev/null (suppresses SDK spam)');

const { DeckLinkManager } = require('./decklink-manager');

const NDI_DISABLED = false;
const NDIManager = NDI_DISABLED ? null : require('./ndi-manager').NDIManager;
const { RecorderManager } = require('./recorder-manager');
const { FrameReceiver } = require('./receiver');
const { createApi } = require('./api');
const { decodeFrame, isJpeg, isAvailable: jpegAvailable } = require('./frame-decoder');

const TCP_PORT = parseInt(process.env.TCP_PORT || '9300', 10);
const API_PORT = parseInt(process.env.API_PORT || '9301', 10);
const HOST = process.env.HOST || '0.0.0.0';
const NDI_PREFIX = process.env.NDI_PREFIX || 'ZoomLink';
const RECORDING_DIR = process.env.RECORDING_DIR || './recordings';

console.log('');
console.log('============================================');
console.log('  ZoomLink SDI Server (Linux)');
console.log('============================================');
console.log('');

const deckLinkManager = new DeckLinkManager();
const ndiManager = NDI_DISABLED ? { isAvailable: () => false, sendVideoFrame() {}, sendAudioData() {}, createSource() {}, destroySource() {}, destroyAll() {}, destroy() {}, getStatus: () => ({ available: false, disabled: true, sourceCount: 0, sources: {} }) } : new NDIManager({ prefix: NDI_PREFIX });
const recorderManager = new RecorderManager({ outputDir: RECORDING_DIR });
const receiver = new FrameReceiver({ port: TCP_PORT, host: HOST });
const api = createApi({ port: API_PORT, deckLinkManager, ndiManager, recorderManager, receiver });

let _ndiAutoCreate = !NDI_DISABLED;

let _videoDecodeCount = 0;
const _decoding = new Map();
const _pendingDecode = new Map();

const _participantIsoIndex = new Map();
let _nextIsoIndex = 1;

const _clientParticipants = new Map();

function _getIsoIndex(userId) {
  if (_participantIsoIndex.has(userId)) {
    return _participantIsoIndex.get(userId);
  }
  const idx = _nextIsoIndex++;
  _participantIsoIndex.set(userId, idx);
  return idx;
}

function _routeFrame(userId, buf, w, h) {
  try { deckLinkManager.sendVideoFrame(userId, buf, w, h); } catch (e) {}
  try { if (!NDI_DISABLED) ndiManager.sendVideoFrame(userId, buf, w, h); } catch (e) {}
  try { recorderManager.writeVideoFrame(userId, buf, w, h); } catch (e) {}
}

async function _processDecodeQueue(userId) {
  while (_pendingDecode.has(userId)) {
    const frame = _pendingDecode.get(userId);
    _pendingDecode.delete(userId);

    const decoded = await decodeFrame(frame.buffer, frame.width, frame.height);
    if (!decoded) continue;
    _videoDecodeCount++;
    if (_videoDecodeCount <= 5 || _videoDecodeCount % 300 === 0) {
      console.log(`[Server] Decoded JPEG #${_videoDecodeCount}: ${decoded.width}x${decoded.height} (${Math.round(frame.buffer.length / 1024)}KB → ${Math.round(decoded.buffer.length / 1024)}KB)`);
    }
    _routeFrame(userId, decoded.buffer, decoded.width, decoded.height);
  }
  _decoding.delete(userId);
}

receiver.on('video-frame', ({ userId, width, height, buffer }) => {
  if (isJpeg(buffer)) {
    _pendingDecode.set(userId, { buffer, width, height });

    if (!_decoding.get(userId)) {
      _decoding.set(userId, true);
      _processDecodeQueue(userId);
    }
  } else {
    _routeFrame(userId, buffer, width, height);
  }
});

receiver.on('audio-data', ({ userId, buffer, sampleRate, channels }) => {
  try { deckLinkManager.sendAudioData(userId, buffer, sampleRate, channels); } catch (e) {}
  try { if (!NDI_DISABLED) ndiManager.sendAudioData(userId, buffer, sampleRate, channels); } catch (e) {}
  try { recorderManager.writeAudioData(userId, buffer); } catch (e) {}
});

receiver.on('participant-join', (participant) => {
  const { userId, name, clientId } = participant;
  console.log(`[Server] Participant joined: ${name} (id=${userId}, client=${clientId || '?'})`);

  if (clientId !== undefined) {
    if (!_clientParticipants.has(clientId)) {
      _clientParticipants.set(clientId, new Set());
    }
    _clientParticipants.get(clientId).add(userId);
  }

  if (_ndiAutoCreate) {
    if (ndiManager.sources && ndiManager.sources.has(userId)) {
      console.log(`[Server] NDI source already exists for ${name} — skipping creation`);
      return;
    }
    const isoIndex = _getIsoIndex(userId);
    const t0 = Date.now();
    Promise.resolve(ndiManager.createSource(userId, name, isoIndex)).then(() => {
      const dt = Date.now() - t0;
      if (dt > 100) {
        console.warn(`[Server] NDI createSource took ${dt}ms (may have blocked event loop)`);
      }
    }).catch((err) => {
      console.error(`[Server] NDI createSource failed: ${err.message}`);
    });
  }
});

receiver.on('participant-leave', ({ userId }) => {
  deckLinkManager.unassignParticipant(userId);
  if (!NDI_DISABLED) ndiManager.destroySource(userId);
  recorderManager.stopRecording(userId);
  console.log(`[Server] Participant left: id=${userId}`);
});

receiver.on('assignment-update', async (assignment) => {
  if (assignment.userId !== undefined && assignment.deviceIndex !== undefined) {
    if (assignment.deviceIndex === null || assignment.deviceIndex === -1) {
      deckLinkManager.unassignParticipant(assignment.userId);
    } else {
      const result = await deckLinkManager.assignParticipant(assignment.userId, assignment.deviceIndex);
      if (result.success) {
        console.log(`[Server] Assigned user ${assignment.userId} → device ${assignment.deviceIndex}`);
      } else {
        console.error(`[Server] Assignment failed: ${result.error}`);
      }
    }
  }
});

receiver.on('client-connected', ({ clientId, remoteAddr }) => {
  console.log(`[Server] Mac client connected: ${remoteAddr} (id=${clientId})`);
});

receiver.on('client-disconnected', ({ clientId, remoteAddr }) => {
  console.log(`[Server] Mac client disconnected: ${remoteAddr} (id=${clientId})`);

  const participantIds = _clientParticipants.get(clientId);
  if (participantIds) {
    for (const userId of participantIds) {
      receiver.participants.delete(userId);
    }
    _clientParticipants.delete(clientId);
    console.log(`[Server] Cleaned up ${participantIds.size} participant(s) from client ${clientId}`);
  }
});

async function main() {
  try {
    await receiver.start();
    await api.start();

    console.log('');
    console.log('  Configuration:');
    console.log(`    TCP stream port: ${TCP_PORT}`);
    console.log(`    HTTP API port:   ${API_PORT}`);
    console.log(`    UV thread pool:  ${UV_POOL}`);
    console.log(`    DeckLink:        ${deckLinkManager.isAvailable() ? `${deckLinkManager.devices.length} device(s)` : 'not available'}`);
    console.log(`    NDI:             ${NDI_DISABLED ? 'DISABLED' : (ndiManager.isAvailable() ? 'available' : 'not available')}`);
    console.log(`    Recording:       ${recorderManager.ffmpegAvailable ? 'available' : 'FFmpeg not found'}`);
    console.log(`    JPEG decode:     ${jpegAvailable() ? 'available (sharp)' : 'NOT available — install sharp for compressed frames'}`);
    console.log(`    NDI Prefix:      ${NDI_PREFIX}`);
    console.log(`    Recording Dir:   ${RECORDING_DIR}`);
    console.log('');
    console.log('  Waiting for connections from ZoomLink Mac app...');
    console.log('');

    if (deckLinkManager.isAvailable()) {
      const devices = deckLinkManager.getDevices();
      console.log('  DeckLink devices:');
      for (const dev of devices) {
        console.log(`    [${dev.index}] ${dev.name} (output: ${dev.supportsOutput ? 'yes' : 'no'})`);
      }
      console.log('');
    }
  } catch (err) {
    console.error('[Server] Failed to start:', err.message);
    process.exit(1);
  }
}

let _shuttingDown = false;
function shutdown() {
  if (_shuttingDown) return;
  _shuttingDown = true;

  console.log('\n[Server] Shutting down...');

  const forceTimer = setTimeout(() => {
    console.log('[Server] Force exit (cleanup exceeded 3s)');
    process.exit(1);
  }, 3000);
  forceTimer.unref();

  setImmediate(() => {
    try { deckLinkManager.destroy(); } catch (e) {}
    try { recorderManager.destroy(); } catch (e) {}
    try { ndiManager.destroy(); } catch (e) {}
    try { receiver.stop(); } catch (e) {}
    try { api.stop(); } catch (e) {}
    console.log('[Server] Cleanup done, exiting.');
    process.exit(0);
  });
}

process.on('SIGTERM', shutdown);

try {
  const procUtils = require(path.join(__dirname, 'process-utils'));
  if (procUtils.available) {
    procUtils.installSigintHandler();
  } else {
    process.on('SIGINT', shutdown);
  }
} catch (e) {
  process.on('SIGINT', shutdown);
}

process.on('uncaughtException', (err) => {
  console.error('[Server] UNCAUGHT EXCEPTION:', err.message);
  console.error(err.stack);
});

process.on('unhandledRejection', (reason) => {
  console.error('[Server] UNHANDLED REJECTION:', reason);
});

main();
