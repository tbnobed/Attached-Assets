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
    console.log('  node scripts/install-zoom-sdk.js /Users/debo/Documents/zoom-sdk-macos-6.7.6.75900');
  } else {
    console.log('Usage: node scripts/install-zoom-sdk.js <path-to-sdk-x64-folder>');
    console.log('');
    console.log('Example:');
    console.log('  node scripts/install-zoom-sdk.js "C:\\Users\\Debo\\Downloads\\zoom-sdk-windows\\x64"');
  }
  process.exit(1);
}

if (!fs.existsSync(sdkPath)) {
  console.error('ERROR: Path does not exist:', sdkPath);
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
    } else if (entry.isSymbolicLink()) {
      const linkTarget = fs.readlinkSync(srcPath);
      try { fs.unlinkSync(destPath); } catch (e) {}
      fs.symlinkSync(linkTarget, destPath);
      count++;
    } else {
      fs.copyFileSync(srcPath, destPath);
      count++;
    }
  }
  return count;
}

function findDirRecursive(dir, name) {
  try {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
      const fullPath = path.join(dir, entry.name);
      if (entry.isDirectory() && entry.name === name) {
        return fullPath;
      }
      if (entry.isDirectory() && !entry.name.endsWith('.framework') && !entry.name.endsWith('.app') && !entry.name.endsWith('.bundle')) {
        const found = findDirRecursive(fullPath, name);
        if (found) return found;
      }
    }
  } catch (e) {}
  return null;
}

console.log(`=== Installing Zoom Meeting SDK (${platform}) ===\n`);
console.log(`Source: ${sdkPath}\n`);

if (platform === 'darwin') {
  let zoomSdkDir = sdkPath;
  const innerZoomSDK = findDirRecursive(sdkPath, 'ZoomSDK');
  if (innerZoomSDK) {
    const frameworkPath = path.join(innerZoomSDK, 'ZoomSDK.framework');
    if (fs.existsSync(frameworkPath)) {
      zoomSdkDir = innerZoomSDK;
    }
  }

  const frameworkPath = path.join(zoomSdkDir, 'ZoomSDK.framework');
  if (!fs.existsSync(frameworkPath)) {
    console.error('ERROR: ZoomSDK.framework not found in', zoomSdkDir);
    console.error('');
    console.error('Expected structure:');
    console.error('  <sdk-path>/ZoomSDK/ZoomSDK.framework');
    process.exit(1);
  }

  console.log(`Found SDK at: ${zoomSdkDir}`);
  console.log(`Found framework: ${frameworkPath}\n`);

  fs.mkdirSync(destLib, { recursive: true });

  const entries = fs.readdirSync(zoomSdkDir, { withFileTypes: true });

  let frameworkCount = 0;
  let dylibCount = 0;
  let bundleCount = 0;
  let appCount = 0;

  for (const entry of entries) {
    const srcPath = path.join(zoomSdkDir, entry.name);
    const destPath = path.join(destLib, entry.name);

    if (entry.name.startsWith('.')) continue;

    if (entry.name.endsWith('.framework')) {
      console.log(`  Copying framework: ${entry.name}`);
      copyDirRecursive(srcPath, destPath);
      frameworkCount++;
    } else if (entry.name.endsWith('.dylib')) {
      console.log(`  Copying dylib: ${entry.name}`);
      fs.copyFileSync(srcPath, destPath);
      dylibCount++;
    } else if (entry.name.endsWith('.bundle')) {
      console.log(`  Copying bundle: ${entry.name}`);
      copyDirRecursive(srcPath, destPath);
      bundleCount++;
    } else if (entry.name.endsWith('.app')) {
      console.log(`  Copying app: ${entry.name}`);
      copyDirRecursive(srcPath, destPath);
      appCount++;
    }
  }

  console.log('');
  console.log(`Copied: ${frameworkCount} frameworks, ${dylibCount} dylibs, ${bundleCount} bundles, ${appCount} apps`);
  console.log('');

  const headersDir = path.join(destLib, 'ZoomSDK.framework', 'Versions', 'A', 'Headers');
  if (fs.existsSync(headersDir)) {
    const headers = fs.readdirSync(headersDir).filter(f => f.endsWith('.h'));
    console.log(`SDK headers: ${headers.length} .h files in ZoomSDK.framework/Headers`);
  } else {
    const altHeaders = path.join(destLib, 'ZoomSDK.framework', 'Headers');
    if (fs.existsSync(altHeaders)) {
      const headers = fs.readdirSync(altHeaders).filter(f => f.endsWith('.h'));
      console.log(`SDK headers: ${headers.length} .h files in ZoomSDK.framework/Headers`);
    } else {
      console.warn('WARNING: Could not find Headers directory in ZoomSDK.framework');
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

  if (fs.existsSync(srcBin)) {
    console.log('Copying binaries...');
    const binCount = copyDirRecursive(srcBin, destBin);
    console.log(`  ${binCount} binary files copied\n`);
  }
}

if (platform === 'darwin') {
  console.log('Setting up Electron framework symlinks...');
  try {
    const electronFrameworksDir = path.join(__dirname, '..', 'node_modules', 'electron', 'dist', 'Electron.app', 'Contents', 'Frameworks');
    if (fs.existsSync(path.join(__dirname, '..', 'node_modules', 'electron', 'dist', 'Electron.app'))) {
      fs.mkdirSync(electronFrameworksDir, { recursive: true });
      const sdkEntries = fs.readdirSync(destLib);
      let linkCount = 0;
      for (const entry of sdkEntries) {
        const srcPath = path.join(destLib, entry);
        const linkPath = path.join(electronFrameworksDir, entry);
        try { fs.unlinkSync(linkPath); } catch (e) {}
        try { fs.rmSync(linkPath, { recursive: true }); } catch (e) {}
        fs.symlinkSync(srcPath, linkPath);
        linkCount++;
      }
      console.log(`  Created ${linkCount} symlinks in Electron.app/Contents/Frameworks/`);
      console.log('  (Required: ZoomSDK uses @executable_path/../Frameworks to find companions)');
    } else {
      console.warn('  Electron.app not found — run "npm install" first, then re-run this script');
    }
  } catch (err) {
    console.warn('  Warning: Could not create Electron framework symlinks:', err.message);
    console.warn('  You may need to manually symlink sdk/lib/* into Electron.app/Contents/Frameworks/');
  }
  console.log('');
}

console.log('=== SDK installed successfully ===');
console.log('Location:', destBase);
console.log('');
console.log('Next step: rebuild the addon:');
console.log('  npm run build-addon');
