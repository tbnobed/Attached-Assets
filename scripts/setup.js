const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const readline = require('readline');

const ROOT = path.join(__dirname, '..');
const ADDON_DIR = path.join(ROOT, 'zoom-meeting-sdk-addon');

function run(cmd, cwd, label) {
  console.log(`  Running: ${cmd}`);
  try {
    execSync(cmd, { cwd: cwd || ROOT, stdio: 'inherit' });
    return true;
  } catch (err) {
    console.error(`  [FAIL] ${label || cmd} failed`);
    return false;
  }
}

function step(num, msg) {
  console.log('');
  console.log('========================================');
  console.log(`  STEP ${num}: ${msg}`);
  console.log('========================================');
  console.log('');
}

function ok(msg) { console.log(`  [OK] ${msg}`); }
function warn(msg) { console.log(`  [!!] ${msg}`); }
function fail(msg) { console.log(`  [FAIL] ${msg}`); }

function prompt(question) {
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
  return new Promise(resolve => {
    rl.question(question, answer => {
      rl.close();
      resolve(answer.trim());
    });
  });
}

function commandExists(cmd) {
  try {
    execSync(process.platform === 'win32' ? `where ${cmd}` : `which ${cmd}`, { stdio: 'pipe' });
    return true;
  } catch { return false; }
}

async function main() {
  console.log('');
  console.log('=============================================');
  console.log('   Zoom ISO Capture - Full Setup');
  console.log('=============================================');
  console.log('');

  let sdkPath = process.argv[2] || '';

  step(0, 'Checking prerequisites');

  if (commandExists('node')) {
    const v = execSync('node --version', { encoding: 'utf8' }).trim();
    ok(`Node.js ${v}`);
  } else {
    fail('Node.js not found');
    process.exit(1);
  }

  if (commandExists('python') || commandExists('python3')) {
    ok('Python found');
  } else {
    warn('Python not found — node-gyp may fail');
  }

  if (commandExists('ffmpeg')) {
    ok('FFmpeg found');
  } else {
    warn('FFmpeg not on PATH — recording will not work');
  }

  if (!sdkPath) {
    const home = process.env.USERPROFILE || process.env.HOME || '';
    const searchDirs = [
      path.join(home, 'Downloads'),
      path.join(home, 'Desktop'),
    ];

    for (const dir of searchDirs) {
      if (!fs.existsSync(dir)) continue;
      const entries = fs.readdirSync(dir);
      for (const entry of entries) {
        if (!entry.toLowerCase().includes('zoom-sdk') && !entry.toLowerCase().includes('zoom-meeting-sdk')) continue;
        const candidate = path.join(dir, entry);
        if (!fs.statSync(candidate).isDirectory()) continue;

        const tryPaths = [
          path.join(candidate, 'x64'),
          candidate,
        ];
        const subDirs = fs.readdirSync(candidate).filter(e => {
          try { return fs.statSync(path.join(candidate, e)).isDirectory(); } catch { return false; }
        });
        for (const sub of subDirs) {
          tryPaths.push(path.join(candidate, sub, 'x64'));
          tryPaths.push(path.join(candidate, sub));
        }

        for (const tp of tryPaths) {
          if (fs.existsSync(path.join(tp, 'h', 'zoom_sdk.h'))) {
            sdkPath = tp;
            ok(`Auto-detected Zoom SDK at: ${sdkPath}`);
            break;
          }
        }
        if (sdkPath) break;
      }
      if (sdkPath) break;
    }

    if (!sdkPath) {
      console.log('');
      sdkPath = await prompt('  Enter path to Zoom SDK x64 folder (or press Enter to skip): ');
    }
  }

  if (sdkPath && !fs.existsSync(path.join(sdkPath, 'h', 'zoom_sdk.h'))) {
    fail(`zoom_sdk.h not found at ${path.join(sdkPath, 'h')}`);
    fail('Make sure you point to the x64 folder inside the extracted SDK.');
    process.exit(1);
  }

  step(1, 'Installing root npm dependencies');
  if (!run('npm install', ROOT, 'npm install')) process.exit(1);
  ok('Root dependencies installed');

  step(2, 'Installing native addon dependencies');
  if (!run('npm install', ADDON_DIR, 'addon npm install')) process.exit(1);
  ok('Addon dependencies installed');

  if (sdkPath) {
    step(3, 'Copying Zoom Meeting SDK files');
    if (!run(`node scripts/install-zoom-sdk.js "${sdkPath}"`, ROOT, 'SDK copy')) process.exit(1);
    ok('Zoom SDK files installed');

    step(4, 'Building Zoom Meeting SDK native addon');
    if (!run('npx node-gyp rebuild', ADDON_DIR, 'addon build')) {
      console.log('');
      warn('Common fixes:');
      warn('  - Install Visual Studio Build Tools with "Desktop development with C++"');
      warn('  - Make sure Python 3 is on your PATH');
      process.exit(1);
    }
    ok('Native addon built successfully');

    const sdkBinDir = path.join(ADDON_DIR, 'sdk', 'bin');
    const buildDir = path.join(ADDON_DIR, 'build', 'Release');
    if (fs.existsSync(sdkBinDir) && fs.existsSync(buildDir)) {
      console.log('');
      console.log('  Copying SDK DLLs to build output...');
      let dllCount = 0;
      for (const file of fs.readdirSync(sdkBinDir)) {
        if (file.toLowerCase().endsWith('.dll')) {
          fs.copyFileSync(path.join(sdkBinDir, file), path.join(buildDir, file));
          dllCount++;
        }
      }
      ok(`${dllCount} SDK DLLs copied to build/Release/`);
    }
  } else {
    step(3, 'Skipping Zoom SDK (no path provided)');
    step(4, 'Skipping addon build');
    warn('App will run in stub mode without Zoom SDK');
  }

  step(5, 'Installing NDI support (grandiose)');
  if (!run('node scripts/fix-grandiose.js', ROOT, 'grandiose')) {
    warn('Grandiose build failed — NDI output will be unavailable');
    warn('This is optional. To retry later: node scripts/fix-grandiose.js');
  } else {
    ok('Grandiose (NDI) installed');
  }

  step(6, 'Checking environment configuration');
  const envFile = path.join(ROOT, '.env');
  if (!fs.existsSync(envFile)) {
    fs.writeFileSync(envFile, [
      '# Zoom Meeting SDK Credentials',
      '# Get these from https://marketplace.zoom.us -> Build App -> Meeting SDK',
      'ZOOM_SDK_KEY=',
      'ZOOM_SDK_SECRET=',
      '',
      '# Bot display name when joining meetings',
      'ZOOM_MEETING_BOT_NAME=PlexISO',
      '',
      '# Recording output directory',
      'DEFAULT_OUTPUT_DIR=./recordings',
      '',
    ].join('\n'), 'utf8');
    ok('Created .env template');
    warn('IMPORTANT: Edit .env and add your ZOOM_SDK_KEY and ZOOM_SDK_SECRET');
  } else {
    ok('.env file exists');
    const content = fs.readFileSync(envFile, 'utf8');
    if (/ZOOM_SDK_KEY=\s*$/m.test(content) || !content.includes('ZOOM_SDK_KEY=')) {
      warn('ZOOM_SDK_KEY is empty — add your key before joining meetings');
    }
  }

  const recordingsDir = path.join(ROOT, 'recordings');
  if (!fs.existsSync(recordingsDir)) {
    fs.mkdirSync(recordingsDir, { recursive: true });
    ok('Created recordings directory');
  }

  console.log('');
  console.log('=============================================');
  console.log('   Setup Complete!');
  console.log('=============================================');
  console.log('');
  console.log('  To launch the app:');
  console.log(`    cd ${ROOT}`);
  console.log('    npm start');
  console.log('');

  if (!sdkPath) {
    warn('Zoom SDK was not installed. Run again with the SDK path:');
    console.log('    node scripts/setup.js "C:\\path\\to\\zoom-sdk\\x64"');
    console.log('');
  }
}

main().catch(err => {
  console.error('Setup failed:', err.message);
  process.exit(1);
});
