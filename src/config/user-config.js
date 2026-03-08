const path = require('path');
const fs = require('fs');
const { app } = require('electron');

const CONFIG_KEYS = [
  'zoomSdkKey',
  'zoomSdkSecret',
  'zoomAccountId',
  'zoomClientId',
  'zoomClientSecret',
  'botName',
  'defaultOutputDir',
];

function getConfigDir() {
  const userDataPath = app.getPath('userData');
  return userDataPath;
}

function getConfigPath() {
  return path.join(getConfigDir(), 'config.json');
}

function loadUserConfig() {
  const configPath = getConfigPath();
  try {
    if (fs.existsSync(configPath)) {
      const raw = fs.readFileSync(configPath, 'utf8');
      const parsed = JSON.parse(raw);
      console.log(`[Config] Loaded user config from ${configPath}`);
      return parsed;
    }
  } catch (err) {
    console.error(`[Config] Error loading config from ${configPath}:`, err.message);
  }
  return {};
}

function saveUserConfig(config) {
  const configDir = getConfigDir();
  if (!fs.existsSync(configDir)) {
    fs.mkdirSync(configDir, { recursive: true });
  }
  const configPath = getConfigPath();
  const filtered = {};
  for (const key of CONFIG_KEYS) {
    if (config[key] !== undefined && config[key] !== null) {
      filtered[key] = config[key];
    }
  }
  fs.writeFileSync(configPath, JSON.stringify(filtered, null, 2), 'utf8');
  console.log(`[Config] Saved user config to ${configPath}`);
  return filtered;
}

function applyUserConfigToSettings(settings) {
  const userConfig = loadUserConfig();
  if (userConfig.zoomSdkKey && !settings.zoomSdkKey) {
    settings.zoomSdkKey = userConfig.zoomSdkKey;
  }
  if (userConfig.zoomSdkSecret && !settings.zoomSdkSecret) {
    settings.zoomSdkSecret = userConfig.zoomSdkSecret;
  }
  if (userConfig.zoomAccountId && !settings.zoomAccountId) {
    settings.zoomAccountId = userConfig.zoomAccountId;
  }
  if (userConfig.zoomClientId && !settings.zoomClientId) {
    settings.zoomClientId = userConfig.zoomClientId;
  }
  if (userConfig.zoomClientSecret && !settings.zoomClientSecret) {
    settings.zoomClientSecret = userConfig.zoomClientSecret;
  }
  if (userConfig.botName) {
    settings.botName = userConfig.botName;
  }
  if (userConfig.defaultOutputDir) {
    settings.defaultOutputDir = userConfig.defaultOutputDir;
  }
  return settings;
}

function getCurrentConfig(settings) {
  return {
    zoomSdkKey: settings.zoomSdkKey || '',
    zoomSdkSecret: settings.zoomSdkSecret || '',
    zoomAccountId: settings.zoomAccountId || '',
    zoomClientId: settings.zoomClientId || '',
    zoomClientSecret: settings.zoomClientSecret || '',
    botName: settings.botName || 'ZoomLink',
    defaultOutputDir: settings.defaultOutputDir || '',
  };
}

function needsConfiguration(settings) {
  return !settings.zoomSdkKey || !settings.zoomSdkSecret;
}

module.exports = {
  loadUserConfig,
  saveUserConfig,
  applyUserConfigToSettings,
  getCurrentConfig,
  needsConfiguration,
  getConfigPath,
};
