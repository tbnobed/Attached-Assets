const { dialog } = require('electron');
const { settings, ensureOutputDir, getOutputDir } = require('../config/settings');

let simulatedUserCounter = 0;

function setupIpcHandlers(ipcMain, context) {
  const {
    sessionManager,
    streamHandler,
    ndiManager,
    deckLinkManager,
    recorderManager,
    sendStatusUpdate,
    getMainWindow,
  } = context;

  ipcMain.handle('get-status', () => {
    return {
      session: sessionManager.getStatus(),
      streams: streamHandler.getStatus(),
      ndi: ndiManager.getStatus(),
      decklink: deckLinkManager.getStatus(),
      recording: recorderManager.getStatus(),
    };
  });

  ipcMain.handle('get-settings', () => {
    return {
      sessionName: settings.sessionName,
      outputDir: getOutputDir(),
      hasCredentials: !!(settings.zoomSdkKey && settings.zoomSdkSecret),
      ndiAvailable: ndiManager.isAvailable(),
      decklinkAvailable: deckLinkManager.isAvailable(),
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
    recorderManager.stopAll();
    deckLinkManager.stopAll();
    ndiManager.destroyAll();
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
    recorderManager.stopAll();
    ndiManager.destroyAll();
    sessionManager.leaveMeeting();
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('start-recording', (_event, userId) => {
    const participant = sessionManager.getParticipant(userId);
    if (!participant) {
      return { success: false, error: 'Participant not found' };
    }
    const recorder = recorderManager.startRecording(
      userId,
      participant.displayName
    );
    return { success: !!recorder };
  });

  ipcMain.handle('stop-recording', (_event, userId) => {
    const recorder = recorderManager.stopRecording(userId);
    return { success: !!recorder };
  });

  ipcMain.handle('start-all-recording', () => {
    const participants = sessionManager.getParticipants();
    let started = 0;
    for (const p of participants) {
      if (!recorderManager.isRecording(p.userId)) {
        const recorder = recorderManager.startRecording(
          p.userId,
          p.displayName
        );
        if (recorder) started++;
      }
    }
    return { success: true, started };
  });

  ipcMain.handle('stop-all-recording', () => {
    recorderManager.stopAll();
    return { success: true };
  });

  ipcMain.handle('toggle-ndi', (_event, userId, active) => {
    ndiManager.toggleSource(userId, active);
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('toggle-all-ndi', (_event, active) => {
    for (const [userId] of ndiManager.sources) {
      ndiManager.toggleSource(userId, active);
    }
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('start-test-pattern', async () => {
    await ndiManager.startTestPattern();
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('stop-test-pattern', () => {
    ndiManager.stopTestPattern();
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('set-output-dir', (_event, dir) => {
    try {
      const outputDir = ensureOutputDir(dir);
      recorderManager.setOutputDir(outputDir);
      sendStatusUpdate();
      return { success: true, outputDir };
    } catch (err) {
      return { success: false, error: err.message };
    }
  });

  ipcMain.handle('select-output-dir', async () => {
    const win = getMainWindow();
    if (!win) return { success: false, error: 'No window' };

    const result = await dialog.showOpenDialog(win, {
      properties: ['openDirectory', 'createDirectory'],
      title: 'Select ISO Recording Output Directory',
    });

    if (result.canceled || !result.filePaths.length) {
      return { success: false, canceled: true };
    }

    const dir = result.filePaths[0];
    recorderManager.setOutputDir(dir);
    sendStatusUpdate();
    return { success: true, outputDir: dir };
  });

  ipcMain.handle('decklink-get-devices', () => {
    return {
      success: true,
      available: deckLinkManager.isAvailable(),
      devices: deckLinkManager.getDevices(),
      displayModes: deckLinkManager.getDisplayModes(),
    };
  });

  ipcMain.handle('decklink-start-output', async (_event, deviceIndex, modeKey) => {
    const result = await deckLinkManager.startOutput(deviceIndex, modeKey);
    sendStatusUpdate();
    return result;
  });

  ipcMain.handle('decklink-stop-output', (_event, deviceIndex) => {
    deckLinkManager.stopOutput(deviceIndex);
    sendStatusUpdate();
    return { success: true };
  });

  ipcMain.handle('decklink-assign-participant', (_event, userId, deviceIndex) => {
    const result = deckLinkManager.assignParticipant(userId, deviceIndex);
    sendStatusUpdate();
    return result;
  });

  ipcMain.handle('decklink-unassign-participant', (_event, userId) => {
    deckLinkManager.unassignParticipant(userId);
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
}

module.exports = { setupIpcHandlers };
