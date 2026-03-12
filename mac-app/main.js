process.env.UV_THREADPOOL_SIZE = '64';
const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const fs = require('fs');

if (process.platform === 'darwin') {
  const electronFrameworks = path.join(__dirname, 'node_modules', 'electron', 'dist', 'Electron.app', 'Contents', 'Frameworks');
  const sdkLib = path.join(__dirname, '..', 'zoom-meeting-sdk-addon', 'sdk', 'lib');
  if (fs.existsSync(sdkLib) && fs.existsSync(electronFrameworks)) {
    for (const item of fs.readdirSync(sdkLib)) {
      const src = path.join(sdkLib, item);
      const dest = path.join(electronFrameworks, item);
      try {
        const srcReal = fs.realpathSync(src);
        if (fs.existsSync(dest)) {
          const stat = fs.lstatSync(dest);
          if (stat.isSymbolicLink() && fs.readlinkSync(dest) === srcReal) continue;
          if (!stat.isSymbolicLink()) continue;
          fs.unlinkSync(dest);
        }
        fs.symlinkSync(srcReal, dest);
        console.log(`[MacApp] Linked SDK framework: ${item}`);
      } catch (e) {}
    }
  }
}

const localEnvPath = path.join(__dirname, '.env');
if (fs.existsSync(localEnvPath)) {
  const content = fs.readFileSync(localEnvPath, 'utf8');
  content.split('\n').forEach(line => {
    const trimmed = line.trim();
    if (trimmed && !trimmed.startsWith('#')) {
      const [key, ...valueParts] = trimmed.split('=');
      if (key && valueParts.length > 0) {
        process.env[key.trim()] = valueParts.join('=').trim();
      }
    }
  });
  console.log(`[MacApp] Loaded local .env from ${localEnvPath}`);
}

const { settings, ensureOutputDir, getOutputDir } = require('../src/config/settings');
const { applyUserConfigToSettings, needsConfiguration } = require('../src/config/user-config');

if (fs.existsSync(localEnvPath)) {
  if (process.env.ZOOM_SDK_KEY) settings.zoomSdkKey = process.env.ZOOM_SDK_KEY;
  if (process.env.ZOOM_SDK_SECRET) settings.zoomSdkSecret = process.env.ZOOM_SDK_SECRET;
  if (process.env.ZOOM_ACCOUNT_ID) settings.zoomAccountId = process.env.ZOOM_ACCOUNT_ID;
  if (process.env.ZOOM_CLIENT_ID) settings.zoomClientId = process.env.ZOOM_CLIENT_ID;
  if (process.env.ZOOM_CLIENT_SECRET) settings.zoomClientSecret = process.env.ZOOM_CLIENT_SECRET;
  if (process.env.ZOOM_MEETING_BOT_NAME) settings.botName = process.env.ZOOM_MEETING_BOT_NAME;
}
const { SessionManager } = require('../src/zoom/session-manager');
const { StreamHandler } = require('../src/zoom/stream-handler');
const { RemoteSDIClient } = require('./network/stream-client');
const { setupIpcHandlers } = require('./ipc-handlers');

let mainWindow = null;
let sessionManager = null;
let streamHandler = null;
let remoteSDIClient = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1000,
    minHeight: 700,
    title: 'ZoomLink Remote SDI',
    backgroundColor: '#0f172a',
    icon: path.join(__dirname, '..', 'build', 'icon.png'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
    },
  });

  mainWindow.loadFile(path.join(__dirname, 'renderer', 'index.html'));

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

function initManagers() {
  sessionManager = new SessionManager(settings);
  streamHandler = new StreamHandler(settings);
  remoteSDIClient = new RemoteSDIClient();

  remoteSDIClient.on('status', (info) => {
    sendToRenderer('remote-sdi-status', info);

    if (info.state === 'connected') {
      const participants = sessionManager.getParticipants();
      for (const p of participants) {
        remoteSDIClient.sendParticipantJoin(p.userId, p.displayName);
      }
    }
  });

  remoteSDIClient.on('error', (info) => {
    sendToRenderer('remote-sdi-error', info);
  });

  sessionManager.setStreamHandler(streamHandler);

  sessionManager.on('participant-joined', async (participant) => {
    remoteSDIClient.sendParticipantJoin(participant.userId, participant.displayName);
    sendToRenderer('participant-joined', participant);
    sendStatusUpdate();
  });

  sessionManager.on('participant-left', (participant) => {
    streamHandler.stopVideoCapture(participant.userId);
    streamHandler.stopAudioCapture(participant.userId);
    remoteSDIClient.sendParticipantLeave(participant.userId);
    sendToRenderer('participant-left', participant);
    sendStatusUpdate();
  });

  sessionManager.on('participant-name-updated', async (participant) => {
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
    remoteSDIClient.sendVideoFrame(userId, frameData.buffer, frameData.width, frameData.height);

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

    remoteSDIClient.sendAudioData(userId, audioData.buffer, audioData.sampleRate, audioData.channels);
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
    remoteSDI: remoteSDIClient.getStatus(),
  };
  sendToRenderer('status-update', status);
}

app.whenReady().then(() => {
  applyUserConfigToSettings(settings);
  initManagers();
  setupIpcHandlers(ipcMain, {
    sessionManager,
    streamHandler,
    remoteSDIClient,
    sendStatusUpdate,
    getMainWindow: () => mainWindow,
  });
  createWindow();

  setInterval(sendStatusUpdate, 2000);
});

app.on('window-all-closed', () => {
  streamHandler.destroy();
  remoteSDIClient.destroy();
  sessionManager.destroy();
  app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
