const fs = require('fs');
const path = require('path');
const os = require('os');
const { execSync } = require('child_process');

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
    } else {
      fs.copyFileSync(srcPath, destPath);
      count++;
    }
  }
  return count;
}

function findFileRecursive(dir, filename) {
  try {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
      const fullPath = path.join(dir, entry.name);
      if (entry.isFile() && entry.name === filename) {
        return fullPath;
      }
      if (entry.isDirectory()) {
        const found = findFileRecursive(fullPath, filename);
        if (found) return found;
      }
    }
  } catch (e) {}
  return null;
}

function findFilesByExtRecursive(dir, ext) {
  const results = [];
  try {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
      const fullPath = path.join(dir, entry.name);
      if (entry.isFile() && entry.name.endsWith(ext)) {
        results.push(fullPath);
      }
      if (entry.isDirectory()) {
        results.push(...findFilesByExtRecursive(fullPath, ext));
      }
    }
  } catch (e) {}
  return results;
}

console.log(`=== Installing Zoom Meeting SDK (${platform}) ===\n`);
console.log(`Source: ${sdkPath}\n`);

if (platform === 'darwin') {
  console.log('Scanning SDK folder structure...\n');

  const zoomSdkH = findFileRecursive(sdkPath, 'zoom_sdk.h');
  if (!zoomSdkH) {
    console.error('ERROR: Could not find zoom_sdk.h anywhere under', sdkPath);
    process.exit(1);
  }
  const headerDir = path.dirname(zoomSdkH);
  console.log(`  Found headers: ${headerDir}`);

  const dylibs = findFilesByExtRecursive(sdkPath, '.dylib');
  let meetingSdkDylib = dylibs.find(d => path.basename(d).includes('meetingsdk') || path.basename(d).includes('zoomsdk'));

  if (!meetingSdkDylib && dylibs.length > 0) {
    meetingSdkDylib = dylibs[0];
    console.log(`  Note: No 'meetingsdk' dylib found, using: ${path.basename(meetingSdkDylib)}`);
  }

  const frameworks = findFilesByExtRecursive(sdkPath, '.framework');

  if (!meetingSdkDylib && frameworks.length === 0) {
    console.error('ERROR: No .dylib or .framework found under', sdkPath);
    console.error('');
    console.error('Found these files:');
    const allFiles = findFilesByExtRecursive(sdkPath, '');
    for (const f of allFiles.slice(0, 30)) {
      console.error('  ', f);
    }
    process.exit(1);
  }

  if (!meetingSdkDylib && frameworks.length > 0) {
    console.log(`  Found framework(s): ${frameworks.map(f => path.basename(f)).join(', ')}`);
    const zoomFramework = frameworks.find(f =>
      path.basename(f).toLowerCase().includes('zoom') ||
      path.basename(f).toLowerCase().includes('meetingsdk')
    ) || frameworks[0];

    const frameworkName = path.basename(zoomFramework, '.framework');
    const frameworkBinary = path.join(zoomFramework, frameworkName);
    const frameworkVersionsBinary = path.join(zoomFramework, 'Versions', 'A', frameworkName);

    if (fs.existsSync(frameworkBinary)) {
      meetingSdkDylib = frameworkBinary;
    } else if (fs.existsSync(frameworkVersionsBinary)) {
      meetingSdkDylib = frameworkVersionsBinary;
    }

    if (meetingSdkDylib) {
      console.log(`  Using framework binary: ${meetingSdkDylib}`);
    } else {
      console.error('ERROR: Could not locate binary inside framework:', zoomFramework);
      process.exit(1);
    }
  }

  console.log(`  Found library: ${meetingSdkDylib}\n`);

  console.log('Copying headers...');
  const hCount = copyDirRecursive(headerDir, destH);
  console.log(`  ${hCount} header files copied\n`);

  console.log('Copying library...');
  fs.mkdirSync(destLib, { recursive: true });
  const destDylibPath = path.join(destLib, 'libmeetingsdk.dylib');
  fs.copyFileSync(meetingSdkDylib, destDylibPath);
  console.log(`  Copied to ${destDylibPath}\n`);

  const otherDylibs = dylibs.filter(d => d !== meetingSdkDylib);
  if (otherDylibs.length > 0) {
    console.log('Copying additional libraries...');
    for (const d of otherDylibs) {
      const destPath = path.join(destLib, path.basename(d));
      fs.copyFileSync(d, destPath);
      console.log(`  ${path.basename(d)}`);
    }
    console.log('');
  }

  console.log('Fixing dylib install_name for @rpath...');
  try {
    execSync(`install_name_tool -id @rpath/libmeetingsdk.dylib "${destDylibPath}"`, { stdio: 'pipe' });
    console.log('  install_name set to @rpath/libmeetingsdk.dylib\n');
  } catch (err) {
    console.warn('  Warning: could not fix install_name:', err.message);
    console.warn('  You may need to run manually:');
    console.warn(`  install_name_tool -id @rpath/libmeetingsdk.dylib "${destDylibPath}"\n`);
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

console.log('=== SDK installed successfully ===');
console.log('Location:', destBase);
console.log('');
console.log('Next step: rebuild the addon:');
console.log('  npm run build-addon');
