'use strict';

const { nativeImage } = require('electron');

const JPEG_QUALITY = 92;

const _encodeBusy = new Map();
const _latestFrame = new Map();
let _onCompressed = null;

function setCompressedCallback(fn) {
  _onCompressed = fn;
}

function compressFrame(userId, bgraBuffer, width, height) {
  _latestFrame.set(userId, { bgraBuffer, width, height });

  if (_encodeBusy.get(userId)) return;

  _processNext(userId);
}

function _processNext(userId) {
  const frame = _latestFrame.get(userId);
  if (!frame) {
    _encodeBusy.delete(userId);
    return;
  }
  _latestFrame.delete(userId);
  _encodeBusy.set(userId, true);

  try {
    const { bgraBuffer, width, height } = frame;
    const img = nativeImage.createFromBitmap(bgraBuffer, { width, height });
    const jpegBuf = img.toJPEG(JPEG_QUALITY);

    if (_onCompressed) {
      _onCompressed(userId, jpegBuf, width, height);
    }
  } catch (err) {
    console.error(`[FrameCompressor] Error encoding userId=${userId}:`, err.message);
  }

  _encodeBusy.delete(userId);

  if (_latestFrame.has(userId)) {
    setImmediate(() => _processNext(userId));
  }
}

module.exports = { compressFrame, setCompressedCallback, JPEG_QUALITY };
