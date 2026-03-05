const fs = require('fs');
const path = require('path');
const os = require('os');

const sdkPath = process.argv[2];
const platform = os.platform();

if (!sdkPath) {
  if (platform === 'darwin') {
    console.log('Usage: node scripts/install-zoom-sdk.js <path-to-mac-sdk-folder>');
    console.log('');
    console.log('Example:');
    console.log('  node scripts/install-zoom-sdk.js ~/Downloads/zoom-meeting-sdk-mac-6.4.6.46498');
    console.log('');
    console.log('Expected structure inside the SDK folder:');
    console.log('  h/           - header files (zoom_sdk.h, etc.)');
    console.log('  lib/         - libmeetingsdk.dylib');
  } else {
    console.log('Usage: node scripts/install-zoom-sdk.js <path-to-sdk-x64-folder>');
    console.log('');
    console.log('Example:');
    console.log('  node scripts/install-zoom-sdk.js "C:\\Users\\Debo\\Downloads\\zoom-sdk-windows\\zoom-sdk-windows-6.7.5.30939\\x64"');
  }
  process.exit(1);
}

const destBase = path.join(__dirname, '..', 'zoom-meeting-sdk-addon', 'sdk');
const destH = path.join(destBase, 'h');
const destLib = path.join(destBase, 'lib');
const destBin = path.join(destBase, 'bin');

function copyDirRecursive(src, dest) {
  fs.mkdirSync(dest, { recursive: true });
  const entries = fs.readdirSync(src, { withFileTypes: true });
  let count = 0;
  for (const entry of entries) {
    const srcPath = path.join(src, entry.name);
    const destPath = path.join(dest, entry.name);
    if (entry.isDirectory()) {
      count += copyDirRecursive(srcPath, destPath);
    } else {
      fs.copyFileSync(srcPath, destPath);
      count++;
    }
  }
  return count;
}

console.log(`=== Installing Zoom Meeting SDK (${platform}) ===\n`);

if (platform === 'darwin') {
  const srcH = path.join(sdkPath, 'h');
  const srcLib = path.join(sdkPath, 'lib');

  if (!fs.existsSync(path.join(srcH, 'zoom_sdk.h'))) {
    console.error('ERROR: zoom_sdk.h not found at', srcH);
    console.error('Make sure you point to the correct macOS SDK folder.');
    console.error('');
    console.error('If headers are in a different location, check the SDK structure:');
    console.error('  find', sdkPath, '-name "zoom_sdk.h"');
    process.exit(1);
  }

  const dylib = path.join(srcLib, 'libmeetingsdk.dylib');
  if (!fs.existsSync(dylib)) {
    console.error('ERROR: libmeetingsdk.dylib not found at', srcLib);
    console.error('');
    console.error('Check for .dylib files:');
    console.error('  find', sdkPath, '-name "*.dylib"');
    process.exit(1);
  }

  console.log('Copying headers...');
  const hCount = copyDirRecursive(srcH, destH);
  console.log(`  ${hCount} header files copied\n`);

  console.log('Copying libraries...');
  const libCount = copyDirRecursive(srcLib, destLib);
  console.log(`  ${libCount} library files copied\n`);

  const destDylib = path.join(destLib, 'libmeetingsdk.dylib');
  if (fs.existsSync(destDylib)) {
    console.log('Fixing dylib install_name for @rpath...');
    const { execSync } = require('child_process');
    try {
      execSync(`install_name_tool -id @rpath/libmeetingsdk.dylib "${destDylib}"`, { stdio: 'pipe' });
      console.log('  install_name set to @rpath/libmeetingsdk.dylib\n');
    } catch (err) {
      console.warn('  Warning: could not fix install_name:', err.message);
      console.warn('  You may need to run: install_name_tool -id @rpath/libmeetingsdk.dylib', destDylib, '\n');
    }
  }

} else {
  const srcH = path.join(sdkPath, 'h');
  const srcLib = path.join(sdkPath, 'lib');
  const srcBin = path.join(sdkPath, 'bin');

  if (!fs.existsSync(path.join(srcH, 'zoom_sdk.h'))) {
    console.error('ERROR: zoom_sdk.h not found at', srcH);
    console.error('Make sure you point to the x64 folder of the SDK.');
    process.exit(1);
  }

  console.log('Copying headers...');
  const hCount = copyDirRecursive(srcH, destH);
  console.log(`  ${hCount} header files copied\n`);

  console.log('Copying libraries...');
  const libCount = copyDirRecursive(srcLib, destLib);
  console.log(`  ${libCount} library files copied\n`);

  console.log('Copying binaries...');
  const binCount = copyDirRecursive(srcBin, destBin);
  console.log(`  ${binCount} binary files copied\n`);
}

console.log('=== SDK installed successfully ===');
console.log('Location:', destBase);
console.log('');
console.log('Next step: rebuild the addon:');
console.log('  cd zoom-meeting-sdk-addon');
console.log('  npx node-gyp rebuild');
