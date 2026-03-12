process.env.UV_THREADPOOL_SIZE = '64';
const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const { settings, ensureOutputDir, getOutputDir } = require('../config/settings');
const { applyUserConfigToSettings, needsConfiguration } = require('../config/user-config');
const { SessionManager } = require('../zoom/session-manager');
const { StreamHandler } = require('../zoom/stream-handler');
const { NDIManager } = require('../ndi/ndi-manager');
const { DeckLinkManager } = require('../decklink/decklink-manager');
const { RecorderManager } = require('../recorder/recorder-manager');
const { setupIpcHandlers } = require('./ipc-handlers');

let mainWindow = null;
let sessionManager = null;
let streamHandler = null;
let ndiManager = null;
let deckLinkManager = null;
let recorderManager = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1000,
    minHeight: 700,
    title: 'ZoomLink',
    backgroundColor: '#0f172a',
    icon: path.join(__dirname, '..', '..', 'build', 'icon.png'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
    },
  });

  mainWindow.loadFile(path.join(__dirname, '..', 'renderer', 'index.html'));

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

function initManagers() {
  sessionManager = new SessionManager(settings);
  streamHandler = new StreamHandler(settings);
  ndiManager = new NDIManager(settings);
  deckLinkManager = new DeckLinkManager(settings);
  recorderManager = new RecorderManager(settings);

  sessionManager.setStreamHandler(streamHandler);

  const outputDir = ensureOutputDir();
  recorderManager.setOutputDir(outputDir);

  sessionManager.on('participant-joined', async (participant) => {
    await ndiManager.createSource(participant.userId, participant.displayName, participant.isoIndex);
    ndiManager.toggleSource(participant.userId, true);
    sendToRenderer('participant-joined', participant);
    sendStatusUpdate();
  });

  sessionManager.on('participant-left', (participant) => {
    streamHandler.stopVideoCapture(participant.userId);
    streamHandler.stopAudioCapture(participant.userId);
    recorderManager.stopRecording(participant.userId);
    deckLinkManager.unassignParticipant(participant.userId);
    ndiManager.destroySource(participant.userId);
    sendToRenderer('participant-left', participant);
    sendStatusUpdate();
  });

  sessionManager.on('participant-name-updated', async (participant) => {
    await ndiManager.renameSource(participant.userId, participant.displayName, participant.isoIndex);
    sendToRenderer('participant-updated', participant);
    sendStatusUpdate();
  });

  sessionManager.on('participant-updated', (participant) => {
    sendToRenderer('participant-updated', participant);
  });

  sessionManager.on('connection-status', (connected) => {
    sendToRenderer('connection-status', connected);
    sendStatusUpdate();
  });

  sessionManager.on('auth-success', () => {
    sendToRenderer('auth-success', {});
  });

  sessionManager.on('raw-recording-started', () => {
    sendToRenderer('raw-recording-started', {});
  });

  sessionManager.on('connection-error', (error) => {
    sendToRenderer('connection-error', { message: error.message || String(error) });
  });

  sessionManager.on('status-message', (message) => {
    sendToRenderer('status-message', { message });
  });

  sessionManager.on('reconnecting', (info) => {
    sendToRenderer('reconnecting', info);
  });

  sessionManager.on('reconnect-failed', () => {
    sendToRenderer('reconnect-failed', {});
  });

  sessionManager.on('reconnect-attempt', (info) => {
    console.log(`[Main] Reconnect attempt ${info.attempt}`);
    sendToRenderer('reconnecting', { attempt: info.attempt, maxAttempts: sessionManager.maxReconnectAttempts });
  });

  if (sessionManager.zoomBridge) {
    sessionManager.zoomBridge.on('meeting-status-detail', (status) => {
      const statusMessages = {
        'MEETING_STATUS_CONNECTING': 'Zoom SDK: Connecting to meeting servers...',
        'MEETING_STATUS_WAITINGFORHOST': 'Zoom SDK: Waiting for host to start the meeting...',
        'MEETING_STATUS_INMEETING': 'Zoom SDK: Successfully joined meeting',
        'MEETING_STATUS_DISCONNECTING': 'Zoom SDK: Disconnecting from meeting...',
        'MEETING_STATUS_RECONNECTING': 'Zoom SDK: Reconnecting to meeting...',
        'MEETING_STATUS_ENDED': 'Zoom SDK: Meeting has ended',
        'MEETING_STATUS_FAILED': 'Zoom SDK: Failed to join meeting — check meeting ID and password',
        'MEETING_STATUS_IDLE': 'Zoom SDK: Idle',
      };
      let msg;
      if (status.startsWith('MEETING_STATUS_FAILED:')) {
        const detail = status.substring('MEETING_STATUS_FAILED:'.length);
        msg = `Zoom SDK: Failed to join meeting — ${detail}`;
      } else {
        msg = statusMessages[status] || `Zoom SDK status: ${status}`;
      }
      console.log(`[Main] ${msg}`);
      sendToRenderer('status-message', { message: msg });
    });
  }

  const previewThrottle = new Map();
  const PREVIEW_INTERVAL_MS = 200;

  streamHandler.on('video-frame', ({ userId, frameData }) => {
    ndiManager.sendVideoFrame(userId, frameData.buffer, frameData.width, frameData.height);
    deckLinkManager.sendVideoFrame(userId, frameData.buffer, frameData.width, frameData.height);
    recorderManager.writeVideoFrame(userId, frameData.buffer, frameData.width, frameData.height);

    const now = Date.now();
    const lastSent = previewThrottle.get(userId) || 0;
    if (now - lastSent >= PREVIEW_INTERVAL_MS) {
      previewThrottle.set(userId, now);
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('video-preview-frame', {
          userId,
          buffer: Buffer.from(frameData.buffer),
          width: frameData.width,
          height: frameData.height,
        });
      }
    }
  });

  let _audioLogCount = 0;
  let _audioStartTime = 0;
  streamHandler.on('audio-data', ({ userId, audioData }) => {
    _audioLogCount++;
    const now = Date.now();
    if (_audioLogCount === 1) _audioStartTime = now;

    if (_audioLogCount <= 5 || _audioLogCount % 2000 === 0) {
      const elapsed = ((now - _audioStartTime) / 1000).toFixed(1);
      const samples = Math.floor(audioData.buffer.length / 2);
      const rate = _audioLogCount / ((now - _audioStartTime) / 1000 || 1);
      console.log(`[AudioDiag] #${_audioLogCount} t=${elapsed}s userId=${userId} bufLen=${audioData.buffer.length} samples=${samples} rate=${Math.round(rate)}/sec sr=${audioData.sampleRate} ch=${audioData.channels}`);
    }

    ndiManager.sendAudioData(userId, audioData.buffer, audioData.sampleRate, audioData.channels);
    deckLinkManager.sendAudioData(userId, audioData.buffer, audioData.sampleRate, audioData.channels);
    recorderManager.writeAudioData(userId, audioData.buffer);
  });

  recorderManager.on('recording-started', (info) => {
    sendToRenderer('recording-started', info);
    sendStatusUpdate();
  });

  recorderManager.on('recording-stopped', (info) => {
    sendToRenderer('recording-stopped', info);
    sendStatusUpdate();
  });

  recorderManager.on('recording-error', (info) => {
    sendToRenderer('recording-error', info);
  });

  ndiManager.on('source-created', (info) => {
    sendToRenderer('ndi-source-created', info);
    sendStatusUpdate();
  });

  ndiManager.on('source-destroyed', (info) => {
    sendToRenderer('ndi-source-destroyed', info);
    sendStatusUpdate();
  });
}

function sendToRenderer(channel, data) {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.send(channel, data);
  }
}

function sendStatusUpdate() {
  const status = {
    session: sessionManager.getStatus(),
    streams: streamHandler.getStatus(),
    ndi: ndiManager.getStatus(),
    decklink: deckLinkManager.getStatus(),
    recording: recorderManager.getStatus(),
  };
  sendToRenderer('status-update', status);
}

app.whenReady().then(() => {
  applyUserConfigToSettings(settings);
  initManagers();
  setupIpcHandlers(ipcMain, {
    sessionManager,
    streamHandler,
    ndiManager,
    deckLinkManager,
    recorderManager,
    sendStatusUpdate,
    getMainWindow: () => mainWindow,
  });
  createWindow();

  setInterval(sendStatusUpdate, 2000);
});

app.on('window-all-closed', () => {
  streamHandler.destroy();
  recorderManager.destroy();
  deckLinkManager.destroy();
  ndiManager.destroy();
  sessionManager.destroy();
  app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
