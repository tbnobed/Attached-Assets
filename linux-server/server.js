'use strict';

const { DeckLinkManager } = require('./decklink-manager');
const { FrameReceiver } = require('./receiver');
const { createApi } = require('./api');

const TCP_PORT = parseInt(process.env.TCP_PORT || '9300', 10);
const API_PORT = parseInt(process.env.API_PORT || '9301', 10);
const HOST = process.env.HOST || '0.0.0.0';

console.log('');
console.log('============================================');
console.log('  ZoomLink SDI Server (Linux)');
console.log('============================================');
console.log('');

const deckLinkManager = new DeckLinkManager();
const receiver = new FrameReceiver({ port: TCP_PORT, host: HOST });
const api = createApi({ port: API_PORT, deckLinkManager, receiver });

receiver.on('video-frame', ({ userId, width, height, buffer }) => {
  deckLinkManager.sendVideoFrame(userId, buffer, width, height);
});

receiver.on('audio-data', ({ userId, buffer, sampleRate, channels }) => {
  deckLinkManager.sendAudioData(userId, buffer, sampleRate, channels);
});

receiver.on('participant-join', (participant) => {
  console.log(`[Server] Participant joined: ${participant.name} (id=${participant.userId})`);
});

receiver.on('participant-leave', ({ userId }) => {
  deckLinkManager.unassignParticipant(userId);
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
  deckLinkManager.destroy();
  receiver.stop();
  api.stop();
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log('\n[Server] Shutting down...');
  deckLinkManager.destroy();
  receiver.stop();
  api.stop();
  process.exit(0);
});

main();
