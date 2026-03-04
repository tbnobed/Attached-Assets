const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const grandioseDir = path.join(__dirname, '..', 'node_modules', 'grandiose');

if (!fs.existsSync(grandioseDir)) {
  console.log('grandiose not found in node_modules. Run "npm install grandiose" first.');
  process.exit(1);
}

const srcDir = path.join(grandioseDir, 'src');

function patchFile(filepath) {
  if (!fs.existsSync(filepath)) return false;

  let content = fs.readFileSync(filepath, 'utf8');
  const original = content;

  content = content.replace(
    /int32_t\s+rejectStatus\s*\(\s*napi_env\s+env\s*,\s*carrier\s*\*\s*c\s*,\s*char\s*\*\s*msg/g,
    'int32_t rejectStatus(napi_env env, carrier* c, const char* msg'
  );

  content = content.replace(
    /int32_t\s+rejectStatus\s*\(\s*napi_env\s+env\s*,\s*carrier\s*\*\s*c\s*,\s*char\s*\*\s+msg/g,
    'int32_t rejectStatus(napi_env env, carrier* c, const char* msg'
  );

  if (content !== original) {
    fs.writeFileSync(filepath, content, 'utf8');
    console.log(`  Patched: ${path.basename(filepath)}`);
    return true;
  }
  return false;
}

console.log('Patching grandiose source files for MSVC const-correctness...\n');

let patched = 0;

const utilHeader = path.join(srcDir, 'grandiose_util.h');
if (fs.existsSync(utilHeader)) {
  let content = fs.readFileSync(utilHeader, 'utf8');
  const original = content;

  content = content.replace(
    /int32_t\s+rejectStatus\s*\([^)]*char\s*\*\s*msg/g,
    (match) => match.replace(/char\s*\*\s*msg/, 'const char* msg')
  );

  if (content !== original) {
    fs.writeFileSync(utilHeader, content, 'utf8');
    console.log('  Patched: grandiose_util.h');
    patched++;
  }
}

const sourceFiles = fs.readdirSync(srcDir).filter(f => f.endsWith('.cc') || f.endsWith('.cpp') || f.endsWith('.h'));
for (const file of sourceFiles) {
  const filepath = path.join(srcDir, file);
  let content = fs.readFileSync(filepath, 'utf8');
  const original = content;

  content = content.replace(
    /rejectStatus\s*\([^)]*char\s*\*\s*msg/g,
    (match) => match.replace(/char\s*\*\s*msg/, 'const char* msg')
  );

  if (content !== original) {
    fs.writeFileSync(filepath, content, 'utf8');
    console.log(`  Patched: ${file}`);
    patched++;
  }
}

if (patched === 0) {
  console.log('No files needed patching (may already be fixed).');
} else {
  console.log(`\nPatched ${patched} file(s). Rebuilding native module...\n`);
  try {
    execSync('npx node-gyp rebuild', {
      cwd: grandioseDir,
      stdio: 'inherit',
    });
    console.log('\ngrandiose rebuilt successfully!');
  } catch (err) {
    console.error('\nRebuild failed. Make sure you have:');
    console.error('  1. Visual Studio Build Tools with "Desktop development with C++" workload');
    console.error('  2. NDI SDK or NDI Tools installed');
    console.error('  3. Python 3.x installed (required by node-gyp)');
    process.exit(1);
  }
}
