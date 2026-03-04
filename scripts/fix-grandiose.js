const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const projectRoot = path.join(__dirname, '..');
const grandioseDir = path.join(projectRoot, 'node_modules', 'grandiose');

console.log('=== Grandiose NDI Fix Tool ===\n');
console.log('This installs the rse/grandiose fork which supports NDI sending.\n');

if (fs.existsSync(grandioseDir)) {
  console.log('Step 1: Removing existing grandiose installation...\n');
  try {
    execSync('npm uninstall grandiose', {
      cwd: projectRoot,
      stdio: 'inherit',
    });
    console.log('');
  } catch (err) {
    console.warn('Warning: could not uninstall cleanly, continuing...');
  }
}

console.log('Step 2: Installing rse/grandiose fork (skipping build)...\n');
try {
  execSync('npm install github:rse/grandiose --ignore-scripts', {
    cwd: projectRoot,
    stdio: 'inherit',
  });
  console.log('');
} catch (err) {
  console.error('Failed to install rse/grandiose. Check your internet connection.');
  process.exit(1);
}

if (!fs.existsSync(grandioseDir)) {
  console.error('grandiose directory not found after install.');
  process.exit(1);
}

console.log('Step 3: Downloading NDI SDK (headers, libs, DLLs)...\n');

const ndiDir = path.join(grandioseDir, 'ndi');
const ndiInclude = path.join(ndiDir, 'include');
const ndiScript = path.join(grandioseDir, 'ndi.js');

if (fs.existsSync(ndiInclude) && fs.existsSync(path.join(ndiInclude, 'Processing.NDI.Lib.h'))) {
  console.log('  NDI SDK already present, skipping download.\n');
} else if (fs.existsSync(ndiScript)) {
  try {
    execSync('node ndi.js', {
      cwd: grandioseDir,
      stdio: 'inherit',
      timeout: 120000,
    });
    console.log('');
  } catch (err) {
    console.warn('  Auto-download failed. Trying to find NDI SDK on system...\n');

    let found = false;
    const programFiles = [
      process.env['ProgramFiles'] || 'C:\\Program Files',
      process.env['ProgramFiles(x86)'] || 'C:\\Program Files (x86)',
    ];

    for (const pf of programFiles) {
      const ndiBase = path.join(pf, 'NDI');
      if (!fs.existsSync(ndiBase)) continue;

      const sdkDirs = fs.readdirSync(ndiBase)
        .filter(d => d.toLowerCase().includes('sdk'))
        .map(d => path.join(ndiBase, d))
        .filter(d => { try { return fs.statSync(d).isDirectory(); } catch { return false; } });

      for (const sdkDir of sdkDirs) {
        const includeDir = [
          path.join(sdkDir, 'Include'),
          path.join(sdkDir, 'include'),
        ].find(d => fs.existsSync(path.join(d, 'Processing.NDI.Lib.h')));

        if (!includeDir) continue;

        const libDir = [
          path.join(sdkDir, 'Lib', 'x64'),
          path.join(sdkDir, 'lib', 'x64'),
        ].find(d => fs.existsSync(d));

        const binDir = [
          path.join(sdkDir, 'Bin', 'x64'),
          path.join(sdkDir, 'bin', 'x64'),
        ].find(d => fs.existsSync(d));

        if (includeDir) {
          console.log('  Found NDI SDK at: ' + sdkDir);
          fs.mkdirSync(path.join(ndiDir, 'include'), { recursive: true });
          fs.mkdirSync(path.join(ndiDir, 'lib', 'win-x64'), { recursive: true });

          for (const f of fs.readdirSync(includeDir)) {
            fs.copyFileSync(path.join(includeDir, f), path.join(ndiDir, 'include', f));
          }
          console.log('  Copied NDI headers');

          if (libDir) {
            for (const f of fs.readdirSync(libDir)) {
              const src = path.join(libDir, f);
              const destDir = f.toLowerCase().endsWith('.dll')
                ? path.join(ndiDir, 'lib', 'win-x64')
                : path.join(ndiDir, 'lib', 'win-x64');
              fs.copyFileSync(src, path.join(destDir, f));
            }
            console.log('  Copied NDI libraries');
          }

          if (binDir) {
            for (const f of fs.readdirSync(binDir)) {
              if (f.toLowerCase().endsWith('.dll')) {
                fs.copyFileSync(
                  path.join(binDir, f),
                  path.join(ndiDir, 'lib', 'win-x64', f)
                );
              }
            }
            console.log('  Copied NDI DLLs');
          }

          found = true;
          break;
        }
      }
      if (found) break;
    }

    if (!found) {
      console.error('');
      console.error('  Could not find NDI SDK on this system.');
      console.error('  Please install the NDI SDK from: https://ndi.video/tools/');
      console.error('  (Download "NDI SDK" — not just "NDI Tools")');
      console.error('');
      console.error('  After installing, run this script again:');
      console.error('    node scripts/fix-grandiose.js');
      process.exit(1);
    }
    console.log('');
  }
} else {
  console.error('  ndi.js script not found in grandiose package.');
  process.exit(1);
}

if (!fs.existsSync(path.join(ndiDir, 'include', 'Processing.NDI.Lib.h'))) {
  console.error('  NDI SDK header (Processing.NDI.Lib.h) still not found after download.');
  console.error('  Install the NDI SDK from https://ndi.video/tools/ and try again.');
  process.exit(1);
}

console.log('Step 4: Applying MSVC const-correctness patches...\n');

const srcDir = path.join(grandioseDir, 'src');
let totalFixes = 0;

function patchFile(filename, replacements) {
  const filepath = path.join(srcDir, filename);
  if (!fs.existsSync(filepath)) {
    console.log('  SKIP: ' + filename + ' not found');
    return;
  }

  let content = fs.readFileSync(filepath, 'utf8');
  const original = content;
  let fixes = 0;

  for (const [search, replace] of replacements) {
    if (content.includes(search)) {
      content = content.split(search).join(replace);
      fixes++;
      console.log('  [' + filename + '] Fixed: ' + search.trim().substring(0, 70) + '...');
    }
  }

  if (content !== original) {
    fs.writeFileSync(filepath, content, 'utf8');
    console.log('  Patched ' + filename + ' (' + fixes + ' fixes)\n');
    totalFixes += fixes;
  } else {
    console.log('  ' + filename + ' - no changes needed\n');
  }
}

if (fs.existsSync(srcDir)) {
  patchFile('grandiose_util.h', [
    [
      'napi_status checkArgs(napi_env env, napi_callback_info info, char* methodName,',
      'napi_status checkArgs(napi_env env, napi_callback_info info, const char* methodName,'
    ],
    [
      'int32_t rejectStatus(napi_env env, carrier* c, char* file, int32_t line);',
      'int32_t rejectStatus(napi_env env, carrier* c, const char* file, int32_t line);'
    ],
  ]);

  patchFile('grandiose_util.cc', [
    [
      'napi_status checkArgs(napi_env env, napi_callback_info info, char* methodName,',
      'napi_status checkArgs(napi_env env, napi_callback_info info, const char* methodName,'
    ],
    [
      'int32_t rejectStatus(napi_env env, carrier* c, char* file, int32_t line) {',
      'int32_t rejectStatus(napi_env env, carrier* c, const char* file, int32_t line) {'
    ],
  ]);

  patchFile('grandiose_receive.cc', [
    [
      'char* resourceName, napi_async_execute_callback execute,',
      'const char* resourceName, napi_async_execute_callback execute,'
    ],
  ]);

  if (totalFixes === 0) {
    console.log('  No exact matches found. Trying flexible line-by-line patching...\n');

    const allFiles = fs.readdirSync(srcDir).filter(f =>
      f.endsWith('.cc') || f.endsWith('.cpp') || f.endsWith('.h')
    );

    const funcNames = [
      'rejectStatus', 'checkArgs', 'dataAndAudioReceive',
      'dataReceive', 'audioReceive', 'videoReceive'
    ];

    for (const file of allFiles) {
      const filepath = path.join(srcDir, file);
      let content = fs.readFileSync(filepath, 'utf8');
      const original = content;
      const lines = content.split('\n');
      const newLines = [];

      for (let i = 0; i < lines.length; i++) {
        let line = lines[i];

        const isFuncSignature = funcNames.some(fn => line.includes(fn)) &&
          (line.includes('(') || line.includes(')')) &&
          line.includes('char') && !line.includes('const char');

        if (isFuncSignature) {
          const newLine = line.replace(/\bchar\s*\*/g, 'const char*');
          if (newLine !== line) {
            console.log('  [' + file + ':' + (i+1) + '] ' + line.trim());
            console.log('    -> ' + newLine.trim());
            line = newLine;
            totalFixes++;
          }
        }

        newLines.push(line);
      }

      const newContent = newLines.join('\n');
      if (newContent !== original) {
        fs.writeFileSync(filepath, newContent, 'utf8');
      }
    }
  }

  console.log('\n  Total fixes applied: ' + totalFixes + '\n');
}

console.log('Step 5: Building native module...\n');

try {
  execSync('npx node-gyp rebuild', {
    cwd: grandioseDir,
    stdio: 'inherit',
  });
  console.log('\n=== SUCCESS: grandiose built successfully! ===');
  console.log('NDI send/receive is now available in Zoom ISO Capture.');
} catch (err) {
  console.error('\n=== BUILD FAILED ===');
  console.error('');
  console.error('Make sure you have ALL of the following installed:');
  console.error('');
  console.error('  1. NDI SDK            -> https://ndi.video/tools/ (download "NDI SDK")');
  console.error('  2. Visual Studio Build Tools with "Desktop development with C++"');
  console.error('     -> https://visualstudio.microsoft.com/visual-cpp-build-tools/');
  console.error('  3. Python 3.x         -> https://www.python.org/downloads/');
  console.error('     (check "Add to PATH" during install)');
  console.error('');
  console.error('After installing those, run this script again:');
  console.error('  node scripts/fix-grandiose.js');
  process.exit(1);
}
