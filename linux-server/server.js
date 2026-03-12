'use strict';

const { DeckLinkManager } = require('./decklink-manager');
const { NDIManager } = require('./ndi-manager');
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
const ndiManager = new NDIManager({ prefix: NDI_PREFIX });
const recorderManager = new RecorderManager({ outputDir: RECORDING_DIR });
const receiver = new FrameReceiver({ port: TCP_PORT, host: HOST });
const api = createApi({ port: API_PORT, deckLinkManager, ndiManager, recorderManager, receiver });

let _ndiAutoCreate = true;

let _videoDecodeCount = 0;
const _decoding = new Map();
const _pendingDecode = new Map();

function _routeFrame(userId, buf, w, h) {
  deckLinkManager.sendVideoFrame(userId, buf, w, h);
  ndiManager.sendVideoFrame(userId, buf, w, h);
  recorderManager.writeVideoFrame(userId, buf, w, h);
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
  deckLinkManager.sendAudioData(userId, buffer, sampleRate, channels);
  ndiManager.sendAudioData(userId, buffer, sampleRate, channels);
  recorderManager.writeAudioData(userId, buffer);
});

receiver.on('participant-join', async (participant) => {
  console.log(`[Server] Participant joined: ${participant.name} (id=${participant.userId})`);
  if (_ndiAutoCreate) {
    const isoIndex = ndiManager.sources.size + 1;
    await ndiManager.createSource(participant.userId, participant.name, isoIndex);
  }
});

receiver.on('participant-leave', ({ userId }) => {
  deckLinkManager.unassignParticipant(userId);
  ndiManager.destroySource(userId);
  recorderManager.stopRecording(userId);
  console.log(`[Server] Participant left: id=${userId}`);
});

receiver.on('assignment-update', (assignment) => {
  if (assignment.userId !== undefined && assignment.deviceIndex !== undefined) {
    if (assignment.deviceIndex === null || assignment.deviceIndex === -1) {
      deckLinkManager.unassignParticipant(assignment.userId);
    } else {
      deckLinkManager.assignParticipant(assignment.userId, assignment.deviceIndex);
    }
  }
});

receiver.on('client-connected', ({ clientId, remoteAddr }) => {
  console.log(`[Server] Mac client connected: ${remoteAddr} (id=${clientId})`);
});

receiver.on('client-disconnected', ({ clientId, remoteAddr }) => {
  console.log(`[Server] Mac client disconnected: ${remoteAddr} (id=${clientId})`);
});

async function main() {
  try {
    await receiver.start();
    await api.start();

    console.log('');
    console.log('  Configuration:');
    console.log(`    TCP stream port: ${TCP_PORT}`);
    console.log(`    HTTP API port:   ${API_PORT}`);
    console.log(`    DeckLink:        ${deckLinkManager.isAvailable() ? `${deckLinkManager.devices.length} device(s)` : 'not available'}`);
    console.log(`    NDI:             ${ndiManager.isAvailable() ? 'available' : 'not available'}`);
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

process.on('SIGINT', () => {
  console.log('\n[Server] Shutting down...');
  recorderManager.destroy();
  ndiManager.destroy();
  deckLinkManager.destroy();
  receiver.stop();
  api.stop();
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log('\n[Server] Shutting down...');
  recorderManager.destroy();
  ndiManager.destroy();
  deckLinkManager.destroy();
  receiver.stop();
  api.stop();
  process.exit(0);
});

main();
