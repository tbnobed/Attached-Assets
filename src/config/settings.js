const path = require('path');
const fs = require('fs');

function loadEnv() {
  const envPath = path.join(__dirname, '..', '..', '.env');
  if (fs.existsSync(envPath)) {
    const content = fs.readFileSync(envPath, 'utf8');
    content.split('\n').forEach(line => {
      const trimmed = line.trim();
      if (trimmed && !trimmed.startsWith('#')) {
        const [key, ...valueParts] = trimmed.split('=');
        if (key && valueParts.length > 0) {
          process.env[key.trim()] = valueParts.join('=').trim();
        }
      }
    });
  }
}

loadEnv();

const settings = {
  zoomSdkKey: process.env.ZOOM_SDK_KEY || '',
  zoomSdkSecret: process.env.ZOOM_SDK_SECRET || '',
  defaultOutputDir: process.env.DEFAULT_OUTPUT_DIR || path.join(__dirname, '..', '..', 'recordings'),
  sessionName: process.env.SESSION_NAME || 'PlexStudioSession',
  sessionPassword: process.env.SESSION_PASSWORD || '',

  video: {
    targetWidth: 1920,
    targetHeight: 1080,
    fallbackWidth: 1280,
    fallbackHeight: 720,
    frameRate: 30,
  },

  audio: {
    sampleRate: 48000,
    channels: 1,
    bitDepth: 16,
  },

  ndi: {
    enabled: true,
    prefix: 'ZoomISO',
  },

  recording: {
    format: 'mp4',
    videoCodec: 'libx264',
    audioCodec: 'aac',
    videoBitrate: '8000k',
    audioBitrate: '192k',
    preset: 'ultrafast',
  },

  maxParticipants: 25,
};

function getOutputDir() {
  const dir = settings.defaultOutputDir;
  if (!path.isAbsolute(dir)) {
    return path.resolve(__dirname, '..', '..', dir);
  }
  return dir;
}

function ensureOutputDir(dir) {
  const outputDir = dir || getOutputDir();
  if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
  }
  return outputDir;
}

module.exports = { settings, getOutputDir, ensureOutputDir };
