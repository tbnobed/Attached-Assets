'use strict';

let sharp = null;
let sharpAvailable = false;

try {
  sharp = require('sharp');
  sharpAvailable = true;
  console.log('[FrameDecoder] sharp loaded — JPEG decoding available');
} catch (err) {
  console.warn('[FrameDecoder] sharp not available — only raw BGRA frames will work');
  console.warn('[FrameDecoder] Install with: npm install sharp');
}

function isJpeg(buffer) {
  return buffer.length >= 2 && buffer[0] === 0xFF && buffer[1] === 0xD8;
}

async function decodeFrame(buffer, width, height) {
  if (!isJpeg(buffer)) {
    return { buffer, width, height, format: 'bgra' };
  }

  if (!sharpAvailable) {
    console.warn('[FrameDecoder] Received JPEG frame but sharp is not installed');
    return null;
  }

  try {
    const { data, info } = await sharp(buffer)
      .ensureAlpha()
      .raw()
      .toBuffer({ resolveWithObject: true });

    const bgra = Buffer.allocUnsafe(data.length);
    for (let i = 0; i < data.length; i += 4) {
      bgra[i] = data[i + 2];
      bgra[i + 1] = data[i + 1];
      bgra[i + 2] = data[i];
      bgra[i + 3] = data[i + 3];
    }

    return { buffer: bgra, width: info.width, height: info.height, format: 'bgra' };
  } catch (err) {
    console.error('[FrameDecoder] JPEG decode error:', err.message);
    return null;
  }
}

function isAvailable() {
  return sharpAvailable;
}

module.exports = { decodeFrame, isJpeg, isAvailable };
