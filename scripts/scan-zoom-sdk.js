const fs = require('fs');
const path = require('path');

const sdkDownloadDir = path.join(
  process.env.USERPROFILE || process.env.HOME || '',
  'Downloads', 'zoom-sdk-windows'
);

const addonSdkDir = path.join(__dirname, '..', 'zoom-meeting-sdk-addon', 'sdk');

function findSdkRoot() {
  const candidates = [
    path.join(addonSdkDir, 'h'),
    path.join(sdkDownloadDir, 'zoom-sdk-windows-6.7.5.30939', 'x64', 'h'),
  ];

  if (process.argv[2]) {
    candidates.unshift(path.join(process.argv[2], 'h'));
    candidates.unshift(process.argv[2]);
  }

  for (const dir of candidates) {
    if (fs.existsSync(dir) && fs.existsSync(path.join(dir, 'zoom_sdk.h'))) {
      return path.dirname(dir);
    }
  }
  return null;
}

function listDir(dir, prefix = '') {
  const results = [];
  try {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
      const relPath = prefix ? `${prefix}/${entry.name}` : entry.name;
      if (entry.isDirectory()) {
        results.push(`[DIR] ${relPath}/`);
        results.push(...listDir(path.join(dir, entry.name), relPath));
      } else {
        results.push(`      ${relPath}`);
      }
    }
  } catch (e) {}
  return results;
}

const sdkRoot = findSdkRoot();

if (!sdkRoot) {
  console.log('Could not find Zoom SDK. Usage:');
  console.log('  node scripts/scan-zoom-sdk.js [path-to-sdk-x64-folder]');
  console.log('');
  console.log('Example:');
  console.log('  node scripts/scan-zoom-sdk.js "C:\\Users\\Debo\\Downloads\\zoom-sdk-windows\\zoom-sdk-windows-6.7.5.30939\\x64"');
  process.exit(1);
}

console.log('=== Zoom SDK Scanner ===');
console.log('SDK root:', sdkRoot);
console.log('');

const headersDir = path.join(sdkRoot, 'h');
const libDir = path.join(sdkRoot, 'lib');
const binDir = path.join(sdkRoot, 'bin');

console.log('--- HEADERS (h/) ---');
listDir(headersDir).forEach(l => console.log(l));

console.log('');
console.log('--- LIBRARIES (lib/) ---');
if (fs.existsSync(libDir)) {
  listDir(libDir).forEach(l => console.log(l));
} else {
  console.log('(not found at', libDir, ')');
}

console.log('');
console.log('--- BINARIES (bin/) ---');
if (fs.existsSync(binDir)) {
  const entries = fs.readdirSync(binDir).filter(f => f.endsWith('.dll') || f.endsWith('.exe'));
  entries.forEach(f => console.log('      ' + f));
  console.log(`  (${entries.length} files)`);
} else {
  console.log('(not found at', binDir, ')');
}

console.log('');
console.log('--- KEY RAW DATA HEADERS ---');
const rawdataDir = path.join(headersDir, 'rawdata');
if (fs.existsSync(rawdataDir)) {
  listDir(rawdataDir).forEach(l => console.log(l));
} else {
  console.log('No rawdata/ subdirectory found in headers');
  const rawHeaders = fs.readdirSync(headersDir).filter(f => f.includes('raw'));
  if (rawHeaders.length > 0) {
    console.log('Raw-related headers in root h/:');
    rawHeaders.forEach(f => console.log('  ' + f));
  }
}

console.log('');
console.log('--- MEETING SERVICE COMPONENTS ---');
const mscDir = path.join(headersDir, 'meeting_service_components');
if (fs.existsSync(mscDir)) {
  listDir(mscDir).forEach(l => console.log(l));
} else {
  console.log('No meeting_service_components/ subdirectory found');
}

console.log('');

const addonHDir = path.join(addonSdkDir, 'h');
if (fs.existsSync(addonHDir) && fs.existsSync(path.join(addonHDir, 'zoom_sdk.h'))) {
  console.log('SDK headers are already installed in zoom-meeting-sdk-addon/sdk/h/');
} else {
  console.log('To install: copy the SDK x64 folder contents into zoom-meeting-sdk-addon/sdk/');
  console.log('  Headers -> zoom-meeting-sdk-addon/sdk/h/');
  console.log('  Lib     -> zoom-meeting-sdk-addon/sdk/lib/');
  console.log('  Bin     -> zoom-meeting-sdk-addon/sdk/bin/');
}
