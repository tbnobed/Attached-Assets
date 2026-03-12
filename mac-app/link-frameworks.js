const fs = require('fs');
const path = require('path');

if (process.platform !== 'darwin') process.exit(0);

const electronFrameworks = path.join(__dirname, 'node_modules', 'electron', 'dist', 'Electron.app', 'Contents', 'Frameworks');
const sdkLib = path.join(__dirname, '..', 'zoom-meeting-sdk-addon', 'sdk', 'lib');

if (!fs.existsSync(sdkLib)) {
  console.log('[link-frameworks] SDK lib not found at', sdkLib, '— skipping');
  process.exit(0);
}

if (!fs.existsSync(electronFrameworks)) {
  console.log('[link-frameworks] Electron Frameworks dir not found — skipping');
  process.exit(0);
}

const items = fs.readdirSync(sdkLib);
let linked = 0;

for (const item of items) {
  const src = path.join(sdkLib, item);
  const dest = path.join(electronFrameworks, item);
  const srcReal = fs.realpathSync(src);

  try {
    if (fs.existsSync(dest)) {
      const stat = fs.lstatSync(dest);
      if (stat.isSymbolicLink()) {
        const existing = fs.readlinkSync(dest);
        if (existing === srcReal) continue;
        fs.unlinkSync(dest);
      } else {
        continue;
      }
    }
    fs.symlinkSync(srcReal, dest);
    linked++;
  } catch (e) {
    console.log(`[link-frameworks] Warning: could not link ${item}: ${e.message}`);
  }
}

if (linked > 0) {
  console.log(`[link-frameworks] Linked ${linked} Zoom SDK frameworks into Electron.app`);
} else {
  console.log('[link-frameworks] All Zoom SDK frameworks already linked');
}
