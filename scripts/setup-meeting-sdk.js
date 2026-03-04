const fs = require('fs');
const path = require('path');

const addonDir = path.join(__dirname, '..', 'zoom-meeting-sdk-addon');
const sdkDir = path.join(addonDir, 'sdk');

console.log('=== Zoom Meeting SDK Setup ===\n');

if (!fs.existsSync(sdkDir)) {
  fs.mkdirSync(sdkDir, { recursive: true });
  fs.mkdirSync(path.join(sdkDir, 'h'), { recursive: true });
  fs.mkdirSync(path.join(sdkDir, 'lib', 'x64'), { recursive: true });
  fs.mkdirSync(path.join(sdkDir, 'bin', 'x64'), { recursive: true });
}

console.log('Directory structure created at: zoom-meeting-sdk-addon/sdk/\n');
console.log('Next steps:\n');
console.log('1. Download the Zoom Meeting SDK for Windows from:');
console.log('   https://marketplace.zoom.us/ -> Build App -> Meeting SDK\n');
console.log('2. Extract the SDK and copy files into the following locations:\n');
console.log('   SDK Headers (.h files):');
console.log('     zoom-meeting-sdk-addon/sdk/h/');
console.log('     - Copy all .h files from the SDK\'s "h" folder\n');
console.log('   SDK Libraries (.lib files):');
console.log('     zoom-meeting-sdk-addon/sdk/lib/x64/');
console.log('     - libSDKBase.lib\n');
console.log('   SDK Binaries (.dll files):');
console.log('     zoom-meeting-sdk-addon/sdk/bin/x64/');
console.log('     - libSDKBase.dll');
console.log('     - sdk.dll');
console.log('     - aomhost.exe');
console.log('     - zVideoApp.dll');
console.log('     - zAudioApp.dll');
console.log('     - (and any other .dll files from the SDK bin folder)\n');

console.log('3. Install node-addon-api:');
console.log('   cd zoom-meeting-sdk-addon && npm install\n');

console.log('4. Build the addon (targeting your Electron version):');
console.log('   npx node-gyp rebuild --target=YOUR_ELECTRON_VERSION --arch=x64 --dist-url=https://electronjs.org/headers\n');
console.log('   To find your Electron version, run: npx electron --version\n');

console.log('5. Set environment variables in .env:');
console.log('   ZOOM_SDK_KEY=your_meeting_sdk_key');
console.log('   ZOOM_SDK_SECRET=your_meeting_sdk_secret');
console.log('   ZOOM_MEETING_BOT_NAME=PlexISO\n');

const existingEnv = path.join(__dirname, '..', '.env');
if (fs.existsSync(existingEnv)) {
  const content = fs.readFileSync(existingEnv, 'utf8');
  if (!content.includes('ZOOM_MEETING_BOT_NAME')) {
    fs.appendFileSync(existingEnv, '\n# Meeting SDK Bot Display Name\nZOOM_MEETING_BOT_NAME=PlexISO\n');
    console.log('Added ZOOM_MEETING_BOT_NAME to .env\n');
  }
} else {
  console.log('No .env file found. Create one with the variables listed above.\n');
}

console.log('=== Setup Complete ===');
console.log('After completing the steps above, restart the Electron app.');
console.log('The dashboard will show "Meeting SDK: Available" when the addon is built correctly.');
