const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('zoomISO', {
  getStatus: () => ipcRenderer.invoke('get-status'),
  getSettings: () => ipcRenderer.invoke('get-settings'),

  joinMeeting: (meetingId, password, displayName) => ipcRenderer.invoke('join-meeting', meetingId, password, displayName),
  leaveMeeting: () => ipcRenderer.invoke('leave-meeting'),
  joinSession: (sessionName, password) => ipcRenderer.invoke('join-session', sessionName, password),
  leaveSession: () => ipcRenderer.invoke('leave-session'),
  generateToken: (sessionName) => ipcRenderer.invoke('generate-token', sessionName),

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

  getAppConfig: () => ipcRenderer.invoke('get-app-config'),
  saveAppConfig: (config) => ipcRenderer.invoke('save-app-config', config),

  getMeetings: () => ipcRenderer.invoke('get-meetings'),
  addRecentMeeting: (meetingId, passcode, label) => ipcRenderer.invoke('add-recent-meeting', meetingId, passcode, label),
  toggleFavoriteMeeting: (meetingId, passcode, label) => ipcRenderer.invoke('toggle-favorite-meeting', meetingId, passcode, label),
  removeFavoriteMeeting: (meetingId) => ipcRenderer.invoke('remove-favorite-meeting', meetingId),

  remoteSDIConnect: (host, port) => ipcRenderer.invoke('remote-sdi-connect', host, port),
  remoteSDIDisconnect: () => ipcRenderer.invoke('remote-sdi-disconnect'),
  remoteSDIStatus: () => ipcRenderer.invoke('remote-sdi-status'),
  remoteSDIQueryDevices: (host, apiPort) => ipcRenderer.invoke('remote-sdi-query-devices', host, apiPort),
  remoteSDIAssign: (userId, deviceIndex) => ipcRenderer.invoke('remote-sdi-assign', userId, deviceIndex),
  remoteSDIUnassign: (userId) => ipcRenderer.invoke('remote-sdi-unassign', userId),
  onRemoteSDIStatus: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('remote-sdi-status', handler);
    return () => ipcRenderer.removeListener('remote-sdi-status', handler);
  },
  onRemoteSDIError: (callback) => {
    const handler = (_event, data) => callback(data);
    ipcRenderer.on('remote-sdi-error', handler);
    return () => ipcRenderer.removeListener('remote-sdi-error', handler);
  },
});
