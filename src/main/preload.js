const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('zoomISO', {
  getStatus: () => ipcRenderer.invoke('get-status'),
  getSettings: () => ipcRenderer.invoke('get-settings'),

  joinMeeting: (meetingId, password, displayName) => ipcRenderer.invoke('join-meeting', meetingId, password, displayName),
  leaveMeeting: () => ipcRenderer.invoke('leave-meeting'),
  joinSession: (sessionName, password) => ipcRenderer.invoke('join-session', sessionName, password),
  leaveSession: () => ipcRenderer.invoke('leave-session'),
  generateToken: (sessionName) => ipcRenderer.invoke('generate-token', sessionName),

  startRecording: (userId) => ipcRenderer.invoke('start-recording', userId),
  stopRecording: (userId) => ipcRenderer.invoke('stop-recording', userId),
  startAllRecording: () => ipcRenderer.invoke('start-all-recording'),
  stopAllRecording: () => ipcRenderer.invoke('stop-all-recording'),

  toggleNdi: (userId, active) => ipcRenderer.invoke('toggle-ndi', userId, active),
  toggleAllNdi: (active) => ipcRenderer.invoke('toggle-all-ndi', active),
  startTestPattern: () => ipcRenderer.invoke('start-test-pattern'),
  stopTestPattern: () => ipcRenderer.invoke('stop-test-pattern'),

  setOutputDir: (dir) => ipcRenderer.invoke('set-output-dir', dir),
  selectOutputDir: () => ipcRenderer.invoke('select-output-dir'),

  decklinkGetDevices: () => ipcRenderer.invoke('decklink-get-devices'),
  decklinkStartOutput: (deviceIndex, modeKey) => ipcRenderer.invoke('decklink-start-output', deviceIndex, modeKey),
  decklinkStopOutput: (deviceIndex) => ipcRenderer.invoke('decklink-stop-output', deviceIndex),
  decklinkAssignParticipant: (userId, deviceIndex) => ipcRenderer.invoke('decklink-assign-participant', userId, deviceIndex),
  decklinkUnassignParticipant: (userId) => ipcRenderer.invoke('decklink-unassign-participant', userId),

  addSimulatedParticipant: (name) => ipcRenderer.invoke('add-simulated-participant', name),
  removeSimulatedParticipant: (userId) => ipcRenderer.invoke('remove-simulated-participant', userId),

  onStatusUpdate: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('status-update', handler);
    return () => ipcRenderer.removeListener('status-update', handler);
  },
  onParticipantJoined: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('participant-joined', handler);
    return () => ipcRenderer.removeListener('participant-joined', handler);
  },
  onParticipantLeft: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('participant-left', handler);
    return () => ipcRenderer.removeListener('participant-left', handler);
  },
  onParticipantUpdated: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('participant-updated', handler);
    return () => ipcRenderer.removeListener('participant-updated', handler);
  },
  onConnectionStatus: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('connection-status', handler);
    return () => ipcRenderer.removeListener('connection-status', handler);
  },
  onConnectionError: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('connection-error', handler);
    return () => ipcRenderer.removeListener('connection-error', handler);
  },
  onReconnecting: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('reconnecting', handler);
    return () => ipcRenderer.removeListener('reconnecting', handler);
  },
  onRecordingStarted: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('recording-started', handler);
    return () => ipcRenderer.removeListener('recording-started', handler);
  },
  onRecordingStopped: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('recording-stopped', handler);
    return () => ipcRenderer.removeListener('recording-stopped', handler);
  },
  onNdiSourceCreated: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('ndi-source-created', handler);
    return () => ipcRenderer.removeListener('ndi-source-created', handler);
  },
  onNdiSourceDestroyed: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('ndi-source-destroyed', handler);
    return () => ipcRenderer.removeListener('ndi-source-destroyed', handler);
  },
  onRecordingError: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('recording-error', handler);
    return () => ipcRenderer.removeListener('recording-error', handler);
  },
  onReconnectFailed: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('reconnect-failed', handler);
    return () => ipcRenderer.removeListener('reconnect-failed', handler);
  },
  onAuthSuccess: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('auth-success', handler);
    return () => ipcRenderer.removeListener('auth-success', handler);
  },
  onRawRecordingStarted: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('raw-recording-started', handler);
    return () => ipcRenderer.removeListener('raw-recording-started', handler);
  },
  onVideoPreviewFrame: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('video-preview-frame', handler);
    return () => ipcRenderer.removeListener('video-preview-frame', handler);
  },
  onStatusMessage: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('status-message', handler);
    return () => ipcRenderer.removeListener('status-message', handler);
  },
});
