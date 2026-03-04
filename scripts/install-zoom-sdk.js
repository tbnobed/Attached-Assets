const fs = require('fs');
const path = require('path');

const sdkPath = process.argv[2];
if (!sdkPath) {
  console.log('Usage: node scripts/install-zoom-sdk.js <path-to-sdk-x64-folder>');
  console.log('');
  console.log('Example:');
  console.log('  node scripts/install-zoom-sdk.js "C:\\Users\\Debo\\Downloads\\zoom-sdk-windows\\zoom-sdk-windows-6.7.5.30939\\x64"');
  process.exit(1);
}

const srcH = path.join(sdkPath, 'h');
const srcLib = path.join(sdkPath, 'lib');
const srcBin = path.join(sdkPath, 'bin');

if (!fs.existsSync(path.join(srcH, 'zoom_sdk.h'))) {
  console.error('ERROR: zoom_sdk.h not found at', srcH);
  console.error('Make sure you point to the x64 folder of the SDK.');
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

console.log('=== Installing Zoom Meeting SDK ===\n');

console.log('Copying headers...');
const hCount = copyDirRecursive(srcH, destH);
console.log(`  ${hCount} header files copied\n`);

console.log('Copying libraries...');
const libCount = copyDirRecursive(srcLib, destLib);
console.log(`  ${libCount} library files copied\n`);

console.log('Copying binaries...');
const binCount = copyDirRecursive(srcBin, destBin);
console.log(`  ${binCount} binary files copied\n`);

console.log('=== SDK installed successfully ===');
console.log('Location:', destBase);
console.log('');
console.log('Next step: rebuild the addon:');
console.log('  cd zoom-meeting-sdk-addon');
console.log('  npx node-gyp rebuild');
