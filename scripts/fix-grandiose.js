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

console.log('Step 2: Installing rse/grandiose fork (with NDI send support)...\n');
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

const srcDir = path.join(grandioseDir, 'src');

if (fs.existsSync(srcDir)) {
  console.log('Step 3: Applying MSVC const-correctness patches...\n');

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
    console.log('No exact matches found. Trying flexible line-by-line patching...\n');

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

  console.log('\nTotal fixes applied: ' + totalFixes + '\n');
} else {
  console.log('Step 3: No src directory found (prebuilt binary may be available)\n');
}

console.log('Step 4: Building native module...\n');

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
