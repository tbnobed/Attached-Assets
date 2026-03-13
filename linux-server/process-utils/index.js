const path = require('path');

let addon = null;
let available = false;

try {
  addon = require('./build/Release/process_utils.node');
  available = true;
} catch (err) {
  try {
    addon = require(path.join(__dirname, 'build', 'Release', 'process_utils.node'));
    available = true;
  } catch (err2) {
  }
}

module.exports = {
  get available() { return available; },

  redirectStdoutToDevNull() {
    if (!addon || typeof addon.redirectStdoutToDevNull !== 'function') return -1;
    return addon.redirectStdoutToDevNull();
  },

  redirectStderrToDevNull() {
    if (!addon || typeof addon.redirectStderrToDevNull !== 'function') return -1;
    return addon.redirectStderrToDevNull();
  },

  restoreStdout() {
    if (!addon || typeof addon.restoreStdout !== 'function') return false;
    return addon.restoreStdout();
  },

  restoreStderr() {
    if (!addon || typeof addon.restoreStderr !== 'function') return false;
    return addon.restoreStderr();
  },

  installSigintHandler() {
    if (!addon || typeof addon.installSigintHandler !== 'function') return false;
    return addon.installSigintHandler();
  },
};
