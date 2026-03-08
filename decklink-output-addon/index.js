const path = require('path');

let addon = null;
let available = false;
let loadError = null;

try {
  addon = require('./build/Release/decklink_output.node');
  available = true;
} catch (err) {
  loadError = err.message;
  try {
    const nativePath = path.join(__dirname, 'build', 'Release', 'decklink_output.node');
    addon = require(nativePath);
    available = true;
    loadError = null;
  } catch (err2) {
    loadError = err2.message;
  }
}

module.exports = {
  get available() {
    return available;
  },

  get loadError() {
    return loadError;
  },

  isAvailable() {
    if (!addon) return false;
    try {
      return addon.isAvailable();
    } catch (e) {
      return false;
    }
  },

  getDevices() {
    if (!addon) return [];
    return addon.getDevices();
  },

  async openOutput(options) {
    if (!addon) throw new Error('DeckLink addon not loaded');
    return addon.openOutput(options);
  },

  async displayFrame(handle, videoBuf, audioBuf) {
    if (!addon) throw new Error('DeckLink addon not loaded');
    return addon.displayFrame(handle, videoBuf, audioBuf);
  },

  closeOutput(handle) {
    if (!addon) return false;
    return addon.closeOutput(handle);
  },

  bmdModeNTSC:           addon ? addon.bmdModeNTSC : 0,
  bmdModeNTSC2398:       addon ? addon.bmdModeNTSC2398 : 0,
  bmdModePAL:            addon ? addon.bmdModePAL : 0,
  bmdModeNTSCp:          addon ? addon.bmdModeNTSCp : 0,
  bmdModePALp:           addon ? addon.bmdModePALp : 0,
  bmdModeHD1080p2398:    addon ? addon.bmdModeHD1080p2398 : 0,
  bmdModeHD1080p24:      addon ? addon.bmdModeHD1080p24 : 0,
  bmdModeHD1080p25:      addon ? addon.bmdModeHD1080p25 : 0,
  bmdModeHD1080p2997:    addon ? addon.bmdModeHD1080p2997 : 0,
  bmdModeHD1080p30:      addon ? addon.bmdModeHD1080p30 : 0,
  bmdModeHD1080i50:      addon ? addon.bmdModeHD1080i50 : 0,
  bmdModeHD1080i5994:    addon ? addon.bmdModeHD1080i5994 : 0,
  bmdModeHD1080i6000:    addon ? addon.bmdModeHD1080i6000 : 0,
  bmdModeHD1080p50:      addon ? addon.bmdModeHD1080p50 : 0,
  bmdModeHD1080p5994:    addon ? addon.bmdModeHD1080p5994 : 0,
  bmdModeHD1080p6000:    addon ? addon.bmdModeHD1080p6000 : 0,
  bmdModeHD720p50:       addon ? addon.bmdModeHD720p50 : 0,
  bmdModeHD720p5994:     addon ? addon.bmdModeHD720p5994 : 0,
  bmdModeHD720p60:       addon ? addon.bmdModeHD720p60 : 0,
  bmdFormat8BitYUV:      addon ? addon.bmdFormat8BitYUV : 0,
  bmdFormat8BitBGRA:     addon ? addon.bmdFormat8BitBGRA : 0,
  bmdFormat10BitYUV:     addon ? addon.bmdFormat10BitYUV : 0,
  bmdAudioSampleRate48kHz:       addon ? addon.bmdAudioSampleRate48kHz : 48000,
  bmdAudioSampleType16bitInteger: addon ? addon.bmdAudioSampleType16bitInteger : 16,
  bmdAudioSampleType32bitInteger: addon ? addon.bmdAudioSampleType32bitInteger : 32,
};
