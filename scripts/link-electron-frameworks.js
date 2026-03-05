const fs = require('fs');
const path = require('path');

if (process.platform !== 'darwin') {
  console.log('[link-frameworks] Not macOS, skipping');
  process.exit(0);
}

const sdkLib = path.join(__dirname, '..', 'zoom-meeting-sdk-addon', 'sdk', 'lib');
const electronApp = path.join(__dirname, '..', 'node_modules', 'electron', 'dist', 'Electron.app');
const frameworksDir = path.join(electronApp, 'Contents', 'Frameworks');

if (!fs.existsSync(sdkLib)) {
  console.log('[link-frameworks] SDK not installed yet (sdk/lib missing). Run install-sdk first.');
  process.exit(0);
}

if (!fs.existsSync(electronApp)) {
  console.log('[link-frameworks] Electron.app not found. Run npm install first.');
  process.exit(0);
}

fs.mkdirSync(frameworksDir, { recursive: true });

const entries = fs.readdirSync(sdkLib);
let count = 0;
for (const entry of entries) {
  const src = path.join(sdkLib, entry);
  const dest = path.join(frameworksDir, entry);
  try { fs.unlinkSync(dest); } catch (e) {}
  try { fs.rmSync(dest, { recursive: true }); } catch (e) {}
  fs.symlinkSync(src, dest);
  count++;
}

console.log(`[link-frameworks] Created ${count} symlinks in Electron.app/Contents/Frameworks/`);
console.log('[link-frameworks] ZoomSDK companion frameworks are now accessible at @executable_path/../Frameworks');
