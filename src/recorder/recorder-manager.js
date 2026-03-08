const EventEmitter = require('events');
const { spawn, execSync } = require('child_process');
const path = require('path');
const fs = require('fs');

class RecorderManager extends EventEmitter {
  constructor(settings) {
    super();
    this.settings = settings;
    this.recorders = new Map();
    this.outputDir = '';
    this.ffmpegAvailable = this._checkFfmpeg();
  }

  _checkFfmpeg() {
    try {
      execSync('ffmpeg -version', { stdio: 'pipe' });
      console.log('[Recorder] FFmpeg found');
      return true;
    } catch (err) {
      console.error('[Recorder] FFmpeg not found - recording disabled');
      return false;
    }
  }

  setOutputDir(dir) {
    this.outputDir = dir;
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
  }

  _generateFilename(displayName) {
    const safeName = displayName.replace(/[^a-zA-Z0-9_-]/g, '_');
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    return `${safeName}_${timestamp}.mp4`;
  }

  startRecording(userId, displayName) {
    if (!this.ffmpegAvailable) {
      this.emit('recording-error', { userId, displayName, error: 'FFmpeg not found' });
      return null;
    }

    if (this.recorders.has(userId)) {
      console.warn(`[Recorder] Already recording for ${displayName}`);
      return null;
    }

    const filename = this._generateFilename(displayName);
    const filepath = path.join(this.outputDir, filename);

    const recorder = {
      userId,
      displayName,
      filename,
      filepath,
      ffmpeg: null,
      startedAt: Date.now(),
      frameCount: 0,
      audioChunks: 0,
      fileSize: 0,
      active: true,
      width: 0,
      height: 0,
      pendingAudio: [],
    };

    this.recorders.set(userId, recorder);
    this.emit('recording-started', { userId, displayName, filename, filepath });
    console.log(`[Recorder] Recording queued for ${displayName}: ${filename} (waiting for first frame)`);
    return recorder;
  }

  _spawnFfmpeg(recorder, width, height) {
    const { recording, audio } = this.settings;
    const ffmpegArgs = [
      '-y',
      '-f', 'rawvideo',
      '-pix_fmt', 'bgra',
      '-s', `${width}x${height}`,
      '-r', String(this.settings.video.frameRate),
      '-i', 'pipe:0',
      '-f', 's16le',
      '-ar', String(audio.sampleRate),
      '-ac', String(audio.channels),
      '-i', 'pipe:3',
      '-c:v', recording.videoCodec,
      '-preset', recording.preset,
      '-b:v', recording.videoBitrate,
      '-pix_fmt', 'yuv420p',
      '-c:a', recording.audioCodec,
      '-b:a', recording.audioBitrate,
      '-movflags', '+faststart',
      recorder.filepath,
    ];

    try {
      const ffmpeg = spawn('ffmpeg', ffmpegArgs, {
        stdio: ['pipe', 'pipe', 'pipe', 'pipe'],
      });

      recorder.ffmpeg = ffmpeg;
      recorder.width = width;
      recorder.height = height;

      ffmpeg.stderr.on('data', (data) => {
        const msg = data.toString();
        if (msg.includes('Error') || msg.includes('error')) {
          console.error(`[Recorder] FFmpeg error for ${recorder.displayName}:`, msg);
        }
      });

      ffmpeg.on('close', (code) => {
        console.log(`[Recorder] FFmpeg closed for ${recorder.displayName} with code ${code}`);
        recorder.active = false;
        this.recorders.delete(recorder.userId);

        try {
          const stats = fs.statSync(recorder.filepath);
          recorder.fileSize = stats.size;
        } catch (e) {}

        this.emit('recording-stopped', {
          userId: recorder.userId,
          displayName: recorder.displayName,
          filename: recorder.filename,
          filepath: recorder.filepath,
          duration: Date.now() - recorder.startedAt,
          fileSize: recorder.fileSize,
        });
      });

      ffmpeg.on('error', (err) => {
        console.error(`[Recorder] FFmpeg process error for ${recorder.displayName}:`, err);
        recorder.active = false;
        this.recorders.delete(recorder.userId);
        this.emit('recording-error', { userId: recorder.userId, displayName: recorder.displayName, error: err.message });
      });

      if (recorder.pendingAudio.length > 0) {
        const audioStream = ffmpeg.stdio[3];
        if (audioStream && audioStream.writable) {
          for (const buf of recorder.pendingAudio) {
            audioStream.write(buf);
          }
        }
        recorder.pendingAudio = [];
      }

      console.log(`[Recorder] FFmpeg started for ${recorder.displayName}: ${width}x${height} bgra`);
    } catch (err) {
      console.error(`[Recorder] Failed to spawn FFmpeg for ${recorder.displayName}:`, err);
      recorder.active = false;
      this.recorders.delete(recorder.userId);
      this.emit('recording-error', { userId: recorder.userId, displayName: recorder.displayName, error: err.message });
    }
  }

  stopRecording(userId) {
    const recorder = this.recorders.get(userId);
    if (!recorder) return null;

    recorder.active = false;
    recorder.pendingAudio = [];

    if (!recorder.ffmpeg) {
      this.recorders.delete(userId);
      this.emit('recording-stopped', {
        userId, displayName: recorder.displayName,
        filename: recorder.filename, filepath: recorder.filepath,
        duration: Date.now() - recorder.startedAt, fileSize: 0,
      });
      return recorder;
    }

    try {
      if (recorder.ffmpeg.stdin.writable) {
        recorder.ffmpeg.stdin.end();
      }
      if (recorder.ffmpeg.stdio && recorder.ffmpeg.stdio[3] && recorder.ffmpeg.stdio[3].writable) {
        recorder.ffmpeg.stdio[3].end();
      }
    } catch (err) {
      console.error(`[Recorder] Error stopping FFmpeg for ${recorder.displayName}:`, err);
      try {
        recorder.ffmpeg.kill('SIGTERM');
      } catch (e) {}
    }

    return recorder;
  }

  writeVideoFrame(userId, frameBuffer, width, height) {
    const recorder = this.recorders.get(userId);
    if (!recorder || !recorder.active) return false;

    if (!recorder.ffmpeg) {
      if (width && height) {
        this._spawnFfmpeg(recorder, width, height);
      }
      if (!recorder.ffmpeg) return false;
    }

    try {
      if (recorder.ffmpeg.stdin.writable) {
        recorder.ffmpeg.stdin.write(frameBuffer);
        recorder.frameCount++;
        return true;
      }
    } catch (err) {
      console.error(`[Recorder] Error writing video frame for ${recorder.displayName}:`, err);
    }
    return false;
  }

  writeAudioData(userId, audioBuffer) {
    const recorder = this.recorders.get(userId);
    if (!recorder || !recorder.active) return false;

    if (!recorder.ffmpeg) {
      if (recorder.pendingAudio.length < 500) {
        recorder.pendingAudio.push(Buffer.from(audioBuffer));
      }
      return true;
    }

    try {
      const audioStream = recorder.ffmpeg.stdio[3];
      if (audioStream && audioStream.writable) {
        audioStream.write(audioBuffer);
        recorder.audioChunks++;
        return true;
      }
    } catch (err) {
      console.error(`[Recorder] Error writing audio for ${recorder.displayName}:`, err);
    }
    return false;
  }

  stopAll() {
    for (const [userId] of this.recorders) {
      this.stopRecording(userId);
    }
  }

  isRecording(userId) {
    const recorder = this.recorders.get(userId);
    return recorder ? recorder.active : false;
  }

  getRecordingStatus(userId) {
    const recorder = this.recorders.get(userId);
    if (!recorder) return null;

    let fileSize = 0;
    try {
      if (fs.existsSync(recorder.filepath)) {
        const stats = fs.statSync(recorder.filepath);
        fileSize = stats.size;
      }
    } catch (e) {}

    return {
      userId,
      displayName: recorder.displayName,
      filename: recorder.filename,
      filepath: recorder.filepath,
      active: recorder.active,
      duration: Date.now() - recorder.startedAt,
      frameCount: recorder.frameCount,
      audioChunks: recorder.audioChunks,
      fileSize,
    };
  }

  getStatus() {
    const recorders = {};
    for (const [userId] of this.recorders) {
      recorders[userId] = this.getRecordingStatus(userId);
    }
    return {
      outputDir: this.outputDir,
      activeCount: this.recorders.size,
      recorders,
    };
  }

  destroy() {
    this.stopAll();
    this.removeAllListeners();
  }
}

module.exports = { RecorderManager };
