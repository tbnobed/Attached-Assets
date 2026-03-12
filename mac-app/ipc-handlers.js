const { dialog } = require('electron');
const { settings, ensureOutputDir, getOutputDir } = require('../src/config/settings');
const { getCurrentConfig, saveUserConfig, getMeetings, addRecentMeeting, toggleFavoriteMeeting, removeFavoriteMeeting } = require('../src/config/user-config');

let simulatedUserCounter = 0;

function setupIpcHandlers(ipcMain, context) {
  const {
    sessionManager,
    streamHandler,
    remoteSDIClient,
    sendStatusUpdate,
    getMainWindow,
  } = context;

  ipcMain.handle('get-status', () => {
    return {
      session: sessionManager.getStatus(),
      streams: streamHandler.getStatus(),
      remoteSDI: remoteSDIClient.getStatus(),
    };
  });

  ipcMain.handle('get-settings', () => {
    return {
      sessionName: settings.sessionName,
      hasCredentials: !!(settings.zoomSdkKey && settings.zoomSdkSecret),
      maxParticipants: settings.maxParticipants,
      video: settings.video,
      audio: settings.audio,
      zoomAvailable: sessionManager.isZoomAvailable(),
    };
  });

  ipcMain.handle('generate-token', (_event, sessionName) => {
    try {
      const token = sessionManager.generateToken(1);
      return { success: true, token, sessionName: sessionName || settings.sessionName };
    } catch (err) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('join-meeting', async (_event, meetingId, password, displayName) => {
    try {
      const ok = await sessionManager.joinMeeting(meetingId, password, displayName);
      if (ok) {
        const JOIN_TIMEOUT_MS = 45000;
        const joinTimer = setTimeout(() => {
          if (!sessionManager.connected) {
            const sdkState = sessionManager.zoomBridge ? sessionManager.zoomBridge.getState() : 'unknown';
            const msg = `Meeting join timed out after ${JOIN_TIMEOUT_MS / 1000}s (SDK state: ${sdkState}). The meeting may have a waiting room, wrong passcode, or the host hasn't started it yet.`;
            console.warn('[MacApp]', msg);
            sendStatusUpdate();
            const win = getMainWindow();
            if (win && !win.isDestroyed()) {
              win.webContents.send('status-message', { message: msg });
              win.webContents.send('connection-error', { message: msg });
            }
          }
        }, JOIN_TIMEOUT_MS);

        const clearTimer = () => clearTimeout(joinTimer);
        sessionManager.once('connection-status', clearTimer);
        sessionManager.once('connection-error', clearTimer);
        if (sessionManager.zoomBridge) {
          sessionManager.zoomBridge.once('meeting-joined', clearTimer);
          sessionManager.zoomBridge.once('meeting-ended', clearTimer);
        }
      }
      return {
        success: ok,
        meetingId,
        message: ok ? 'Joining meeting...' : 'Failed to initiate join',
      };
    } catch (err) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('leave-meeting', () => {
    streamHandler.stopAll();
    sessionManager.leaveMeeting();
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('join-session', (_event, sessionName, password) => {
    try {
      if (sessionName) {
        sessionManager.sessionName = sessionName;
      }
      const token = sessionManager.generateToken(1);
      sessionManager.setConnected(true);
      return {
        success: true,
        token,
        sessionName: sessionManager.sessionName,
        message: 'Session token generated.',
      };
    } catch (err) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('leave-session', () => {
    streamHandler.stopAll();
    sessionManager.leaveMeeting();
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('add-simulated-participant', (_event, name) => {
    simulatedUserCounter++;
    const userId = `sim_${simulatedUserCounter}_${Date.now()}`;
    const displayName = name || `Participant_${simulatedUserCounter}`;
    const participant = sessionManager.addParticipant({ userId, displayName });
    return { success: true, participant };
  });

  ipcMain.handle('remove-simulated-participant', (_event, userId) => {
    const participant = sessionManager.removeParticipant(userId);
    return { success: !!participant };
  });

  ipcMain.handle('get-app-config', () => {
    return getCurrentConfig(settings);
  });

  ipcMain.handle('save-app-config', (_event, config) => {
    try {
      const saved = saveUserConfig(config);
      if (config.zoomSdkKey) settings.zoomSdkKey = config.zoomSdkKey;
      if (config.zoomSdkSecret) settings.zoomSdkSecret = config.zoomSdkSecret;
      if (config.zoomAccountId !== undefined) settings.zoomAccountId = config.zoomAccountId;
      if (config.zoomClientId !== undefined) settings.zoomClientId = config.zoomClientId;
      if (config.zoomClientSecret !== undefined) settings.zoomClientSecret = config.zoomClientSecret;
      if (config.botName) settings.botName = config.botName;
      if (config.defaultOutputDir) settings.defaultOutputDir = config.defaultOutputDir;
      return { success: true, needsRestart: true };
    } catch (err) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('get-meetings', () => {
    return getMeetings();
  });

  ipcMain.handle('add-recent-meeting', (_event, meetingId, passcode, label) => {
    return addRecentMeeting(meetingId, passcode, label);
  });

  ipcMain.handle('toggle-favorite-meeting', (_event, meetingId, passcode, label) => {
    return toggleFavoriteMeeting(meetingId, passcode, label);
  });

  ipcMain.handle('remove-favorite-meeting', (_event, meetingId) => {
    return removeFavoriteMeeting(meetingId);
  });

  ipcMain.handle('remote-sdi-connect', (_event, host, port) => {
    try {
      remoteSDIClient.connect(host, port || 9300);
      return { success: true };
    } catch (err) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('remote-sdi-query-devices', async (_event, host, apiPort) => {
    const http = require('http');
    const p = apiPort || 9301;
    return new Promise((resolve) => {
      const req = http.get(`http://${host}:${p}/api/devices`, { timeout: 5000 }, (res) => {
        let body = '';
        res.on('data', (chunk) => { body += chunk; });
        res.on('end', () => {
          if (res.statusCode < 200 || res.statusCode >= 300) {
            resolve({ success: false, error: `Server returned HTTP ${res.statusCode}`, devices: [] });
            return;
          }
          try {
            const data = JSON.parse(body);
            resolve({ success: true, devices: data.devices || [], displayModes: data.displayModes || {} });
          } catch (e) {
            resolve({ success: false, error: 'Invalid response from server', devices: [] });
          }
        });
      });
      req.on('error', (err) => {
        resolve({ success: false, error: err.message, devices: [] });
      });
      req.on('timeout', () => {
        req.destroy();
        resolve({ success: false, error: 'Connection timed out', devices: [] });
      });
    });
  });

  ipcMain.handle('remote-sdi-disconnect', () => {
    remoteSDIClient.disconnect();
    return { success: true };
  });

  ipcMain.handle('remote-sdi-status', () => {
    return remoteSDIClient.getStatus();
  });

  ipcMain.handle('remote-sdi-assign', (_event, userId, deviceIndex) => {
    remoteSDIClient.sendAssignment(userId, deviceIndex);
    return { success: true };
  });

  ipcMain.handle('remote-sdi-unassign', (_event, userId) => {
    remoteSDIClient.removeAssignment(userId);
    return { success: true };
  });
}

module.exports = { setupIpcHandlers };
