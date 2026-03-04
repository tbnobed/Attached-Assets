const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const projectRoot = path.join(__dirname, '..');
const grandioseDir = path.join(projectRoot, 'node_modules', 'grandiose');

console.log('=== Grandiose NDI Fix Tool ===\n');

if (!fs.existsSync(grandioseDir)) {
  console.log('Step 1: Downloading grandiose (skipping compilation)...\n');
  try {
    execSync('npm install grandiose --ignore-scripts', {
      cwd: projectRoot,
      stdio: 'inherit',
    });
    console.log('');
  } catch (err) {
    console.error('Failed to download grandiose. Check your internet connection.');
    process.exit(1);
  }
}

if (!fs.existsSync(grandioseDir)) {
  console.error('grandiose directory still not found after install. Something went wrong.');
  process.exit(1);
}

const srcDir = path.join(grandioseDir, 'src');

if (!fs.existsSync(srcDir)) {
  console.error('grandiose src directory not found. The package may be corrupt.');
  console.error('Try: rm -rf node_modules/grandiose && node scripts/fix-grandiose.js');
  process.exit(1);
}

console.log('Step 2: Patching source files for MSVC const-correctness...\n');

let patched = 0;

const allFiles = fs.readdirSync(srcDir).filter(f =>
  f.endsWith('.cc') || f.endsWith('.cpp') || f.endsWith('.h')
);

for (const file of allFiles) {
  const filepath = path.join(srcDir, file);
  let content = fs.readFileSync(filepath, 'utf8');
  const original = content;

  content = content.replace(/char\s*\*\s*msg,\s*int32_t\s*status/g, 'const char* msg, int32_t status');

  content = content.replace(
    /(int32_t\s+rejectStatus\s*\([^)]*?)char\s*\*(\s*msg)/g,
    '$1const char*$2'
  );

  content = content.replace(
    /(rejectStatus\s*\([^)]*?)char\s*\*(\s*msg)/g,
    '$1const char*$2'
  );

  if (content !== original) {
    fs.writeFileSync(filepath, content, 'utf8');
    console.log('  Patched: ' + file);
    patched++;
  } else {
    console.log('  OK (no changes needed): ' + file);
  }
}

if (patched === 0) {
  console.log('\nNo files needed patching (may already be fixed).\n');
} else {
  console.log('\nPatched ' + patched + ' file(s).\n');
}

console.log('Step 3: Building native module...\n');

try {
  execSync('npx node-gyp rebuild', {
    cwd: grandioseDir,
    stdio: 'inherit',
  });
  console.log('\n=== SUCCESS: grandiose built successfully! ===');
  console.log('NDI output is now available in Zoom ISO Capture.');
} catch (err) {
  console.error('\n=== BUILD FAILED ===');
  console.error('');
  console.error('Make sure you have ALL of the following installed:');
  console.error('');
  console.error('  1. NDI Tools          -> https://ndi.video/tools/');
  console.error('  2. Visual Studio Build Tools with "Desktop development with C++"');
  console.error('     -> https://visualstudio.microsoft.com/visual-cpp-build-tools/');
  console.error('  3. Python 3.x         -> https://www.python.org/downloads/');
  console.error('     (check "Add to PATH" during install)');
  console.error('');
  console.error('After installing those, run this script again:');
  console.error('  node scripts/fix-grandiose.js');
  process.exit(1);
}
