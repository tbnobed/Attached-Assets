const EventEmitter = require('events');

class StreamHandler extends EventEmitter {
  constructor(settings) {
    super();
    this.settings = settings;
    this.activeVideoCaptures = new Map();
    this.activeAudioCaptures = new Map();
    this.captureIntervals = new Map();
    this.frameRate = settings.video.frameRate;
  }

  startVideoCapture(userId, displayName) {
    if (this.activeVideoCaptures.has(userId)) return;

    this.activeVideoCaptures.set(userId, {
      userId,
      displayName,
      startedAt: Date.now(),
      frameCount: 0,
    });

    this.emit('video-capture-started', { userId, displayName });
  }

  stopVideoCapture(userId) {
    const capture = this.activeVideoCaptures.get(userId);
    if (!capture) return;

    const interval = this.captureIntervals.get(`video_${userId}`);
    if (interval) {
      clearInterval(interval);
      this.captureIntervals.delete(`video_${userId}`);
    }

    this.activeVideoCaptures.delete(userId);
    this.emit('video-capture-stopped', { userId, displayName: capture.displayName });
  }

  startAudioCapture(userId, displayName) {
    if (this.activeAudioCaptures.has(userId)) return;

    this.activeAudioCaptures.set(userId, {
      userId,
      displayName,
      startedAt: Date.now(),
      sampleCount: 0,
    });

    this.emit('audio-capture-started', { userId, displayName });
  }

  stopAudioCapture(userId) {
    const capture = this.activeAudioCaptures.get(userId);
    if (!capture) return;

    const interval = this.captureIntervals.get(`audio_${userId}`);
    if (interval) {
      clearInterval(interval);
      this.captureIntervals.delete(`audio_${userId}`);
    }

    this.activeAudioCaptures.delete(userId);
    this.emit('audio-capture-stopped', { userId, displayName: capture.displayName });
  }

  handleVideoFrame(userId, frameData) {
    const capture = this.activeVideoCaptures.get(userId);
    if (capture) {
      capture.frameCount++;
      this.emit('video-frame', { userId, frameData, frameNumber: capture.frameCount });
    }
  }

  handleAudioData(userId, audioData) {
    const capture = this.activeAudioCaptures.get(userId);
    if (capture) {
      capture.sampleCount++;
      this.emit('audio-data', { userId, audioData, sampleNumber: capture.sampleCount });
    }
  }

  stopAll() {
    for (const [userId] of this.activeVideoCaptures) {
      this.stopVideoCapture(userId);
    }
    for (const [userId] of this.activeAudioCaptures) {
      this.stopAudioCapture(userId);
    }
  }

  getStatus() {
    const videoCaptures = {};
    for (const [userId, capture] of this.activeVideoCaptures) {
      videoCaptures[userId] = { ...capture };
    }
    const audioCaptures = {};
    for (const [userId, capture] of this.activeAudioCaptures) {
      audioCaptures[userId] = { ...capture };
    }
    return { videoCaptures, audioCaptures };
  }

  destroy() {
    this.stopAll();
    this.removeAllListeners();
  }
}

module.exports = { StreamHandler };
