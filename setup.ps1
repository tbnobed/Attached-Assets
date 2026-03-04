param(
    [Parameter(Mandatory=$false)]
    [string]$ZoomSdkPath,

    [Parameter(Mandatory=$false)]
    [switch]$SkipNDI,

    [Parameter(Mandatory=$false)]
    [switch]$Help
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
if (-not $ProjectRoot) { $ProjectRoot = (Get-Location).Path }

function Write-Step($num, $msg) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  STEP $num : $msg" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Ok($msg) {
    Write-Host "  [OK] $msg" -ForegroundColor Green
}

function Write-Warn($msg) {
    Write-Host "  [!!] $msg" -ForegroundColor Yellow
}

function Write-Fail($msg) {
    Write-Host "  [FAIL] $msg" -ForegroundColor Red
}

function Test-Command($cmd) {
    try { Get-Command $cmd -ErrorAction Stop | Out-Null; return $true }
    catch { return $false }
}

if ($Help) {
    Write-Host ""
    Write-Host "Zoom ISO Capture - Setup Script" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\setup.ps1 -ZoomSdkPath ""C:\path\to\zoom-sdk\x64"""
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -ZoomSdkPath   Path to the extracted Zoom Meeting SDK x64 folder"
    Write-Host "                 (the folder containing h\, lib\, and bin\ subfolders)"
    Write-Host "  -SkipNDI       Skip grandiose/NDI installation"
    Write-Host "  -Help          Show this help message"
    Write-Host ""
    Write-Host "Prerequisites:"
    Write-Host "  - Node.js 18+ (https://nodejs.org)"
    Write-Host "  - Visual Studio Build Tools with 'Desktop development with C++'"
    Write-Host "    (https://visualstudio.microsoft.com/visual-cpp-build-tools/)"
    Write-Host "  - Python 3.x on PATH (https://www.python.org)"
    Write-Host "  - Zoom Meeting SDK downloaded from https://marketplace.zoom.us"
    Write-Host "  - FFmpeg on PATH (https://ffmpeg.org) [for recording]"
    Write-Host "  - NDI Tools (https://ndi.video/tools) [optional, for NDI output]"
    Write-Host ""
    exit 0
}

Write-Host ""
Write-Host "=============================================" -ForegroundColor Magenta
Write-Host "   Zoom ISO Capture - Full Setup Installer" -ForegroundColor Magenta
Write-Host "=============================================" -ForegroundColor Magenta
Write-Host ""

# ── Pre-flight checks ──

Write-Step 0 "Checking prerequisites"

$ready = $true

if (Test-Command "node") {
    $nodeVer = (node --version 2>&1)
    Write-Ok "Node.js $nodeVer"
} else {
    Write-Fail "Node.js not found. Install from https://nodejs.org"
    $ready = $false
}

if (Test-Command "npm") {
    $npmVer = (npm --version 2>&1)
    Write-Ok "npm $npmVer"
} else {
    Write-Fail "npm not found."
    $ready = $false
}

if (Test-Command "python") {
    $pyVer = (python --version 2>&1)
    Write-Ok "$pyVer"
} elseif (Test-Command "python3") {
    $pyVer = (python3 --version 2>&1)
    Write-Ok "$pyVer"
} else {
    Write-Warn "Python not found. node-gyp may fail. Install from https://www.python.org"
}

if (Test-Command "ffmpeg") {
    Write-Ok "FFmpeg found"
} else {
    Write-Warn "FFmpeg not on PATH. Recording will not work without it."
}

if (-not $ready) {
    Write-Host ""
    Write-Fail "Missing required prerequisites. Install them and try again."
    exit 1
}

# ── Check Zoom SDK path ──

if (-not $ZoomSdkPath) {
    $defaultPaths = @(
        "$env:USERPROFILE\Downloads\zoom-sdk-windows\zoom-sdk-windows-*\x64",
        "$env:USERPROFILE\Downloads\zoom-meeting-sdk-windows-*\x64",
        "$env:USERPROFILE\Desktop\zoom-sdk*\x64"
    )
    foreach ($pattern in $defaultPaths) {
        $found = Get-Item $pattern -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found -and (Test-Path "$($found.FullName)\h\zoom_sdk.h")) {
            $ZoomSdkPath = $found.FullName
            Write-Ok "Auto-detected Zoom SDK at: $ZoomSdkPath"
            break
        }
    }

    if (-not $ZoomSdkPath) {
        Write-Host ""
        Write-Warn "Zoom SDK path not provided and could not be auto-detected."
        $ZoomSdkPath = Read-Host "  Enter the path to the Zoom SDK x64 folder (or press Enter to skip)"
        if (-not $ZoomSdkPath) {
            Write-Warn "Skipping Zoom SDK installation. The app will run in stub mode."
        }
    }
}

if ($ZoomSdkPath -and -not (Test-Path "$ZoomSdkPath\h\zoom_sdk.h")) {
    Write-Fail "zoom_sdk.h not found at $ZoomSdkPath\h\"
    Write-Fail "Make sure you point to the x64 folder (e.g., zoom-sdk-windows-6.7.5.30939\x64)"
    exit 1
}

# ── Step 1: Install root dependencies ──

Write-Step 1 "Installing root npm dependencies"

Set-Location $ProjectRoot
npm install 2>&1 | ForEach-Object { Write-Host "  $_" }
if ($LASTEXITCODE -ne 0) {
    Write-Fail "npm install failed"
    exit 1
}
Write-Ok "Root dependencies installed"

# ── Step 2: Install addon dependencies ──

Write-Step 2 "Installing native addon dependencies"

$addonDir = Join-Path $ProjectRoot "zoom-meeting-sdk-addon"
Set-Location $addonDir
npm install 2>&1 | ForEach-Object { Write-Host "  $_" }
if ($LASTEXITCODE -ne 0) {
    Write-Fail "Addon npm install failed"
    exit 1
}
Write-Ok "Addon dependencies installed"
Set-Location $ProjectRoot

# ── Step 3: Copy Zoom SDK files ──

if ($ZoomSdkPath) {
    Write-Step 3 "Copying Zoom Meeting SDK files"

    node scripts/install-zoom-sdk.js "$ZoomSdkPath" 2>&1 | ForEach-Object { Write-Host "  $_" }
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "SDK copy failed"
        exit 1
    }
    Write-Ok "Zoom SDK files copied"
} else {
    Write-Step 3 "Skipping Zoom SDK copy (no path provided)"
}

# ── Step 4: Build native addon ──

if ($ZoomSdkPath) {
    Write-Step 4 "Building Zoom Meeting SDK native addon"

    Set-Location $addonDir
    npx node-gyp rebuild 2>&1 | ForEach-Object { Write-Host "  $_" }
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "Native addon build failed!"
        Write-Host ""
        Write-Host "  Common fixes:" -ForegroundColor Yellow
        Write-Host "    - Install Visual Studio Build Tools with 'Desktop development with C++'" -ForegroundColor Yellow
        Write-Host "    - Make sure Python 3 is on your PATH" -ForegroundColor Yellow
        exit 1
    }
    Set-Location $ProjectRoot
    Write-Ok "Native addon built successfully"

    # Copy SDK DLLs to build output so they're found at runtime
    Write-Host ""
    Write-Host "  Copying SDK DLLs to build output..." -ForegroundColor Gray
    $sdkBinDir = Join-Path $addonDir "sdk\bin"
    $buildDir = Join-Path $addonDir "build\Release"
    if ((Test-Path $sdkBinDir) -and (Test-Path $buildDir)) {
        $dllCount = 0
        Get-ChildItem "$sdkBinDir\*.dll" | ForEach-Object {
            Copy-Item $_.FullName $buildDir -Force
            $dllCount++
        }
        Write-Ok "$dllCount SDK DLLs copied to build\Release\"
    }
} else {
    Write-Step 4 "Skipping addon build (no SDK)"
}

# ── Step 5: Install and fix grandiose (NDI) ──

if (-not $SkipNDI) {
    Write-Step 5 "Installing NDI support (grandiose)"

    Set-Location $ProjectRoot
    node scripts/fix-grandiose.js 2>&1 | ForEach-Object { Write-Host "  $_" }
    if ($LASTEXITCODE -ne 0) {
        Write-Warn "Grandiose build failed. NDI output will be unavailable."
        Write-Warn "This is optional — the app works without NDI."
        Write-Warn "To try again later: node scripts\fix-grandiose.js"
    } else {
        Write-Ok "Grandiose (NDI) installed and built"
    }
} else {
    Write-Step 5 "Skipping NDI installation (--SkipNDI)"
}

# ── Step 6: Create .env if missing ──

Write-Step 6 "Checking environment configuration"

Set-Location $ProjectRoot
$envFile = Join-Path $ProjectRoot ".env"

if (-not (Test-Path $envFile)) {
    @"
# Zoom Meeting SDK Credentials
# Get these from https://marketplace.zoom.us -> Build App -> Meeting SDK
ZOOM_SDK_KEY=
ZOOM_SDK_SECRET=

# Bot display name when joining meetings
ZOOM_MEETING_BOT_NAME=PlexISO

# Recording output directory
DEFAULT_OUTPUT_DIR=./recordings
"@ | Set-Content $envFile -Encoding UTF8

    Write-Ok "Created .env template"
    Write-Warn "IMPORTANT: Edit .env and add your ZOOM_SDK_KEY and ZOOM_SDK_SECRET"
} else {
    Write-Ok ".env file already exists"

    $envContent = Get-Content $envFile -Raw
    if ($envContent -match "ZOOM_SDK_KEY=\s*$" -or $envContent -match "ZOOM_SDK_KEY=$") {
        Write-Warn "ZOOM_SDK_KEY is empty in .env — add your key before joining meetings"
    }
}

# ── Step 7: Create recordings directory ──

$recordingsDir = Join-Path $ProjectRoot "recordings"
if (-not (Test-Path $recordingsDir)) {
    New-Item -ItemType Directory -Path $recordingsDir -Force | Out-Null
    Write-Ok "Created recordings directory"
}

# ── Done ──

Write-Host ""
Write-Host "=============================================" -ForegroundColor Green
Write-Host "   Setup Complete!" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green
Write-Host ""
Write-Host "  To launch the app:" -ForegroundColor White
Write-Host "    cd $ProjectRoot" -ForegroundColor Yellow
Write-Host "    npx electron . --no-sandbox --disable-gpu-sandbox" -ForegroundColor Yellow
Write-Host ""

$envContent = ""
if (Test-Path $envFile) { $envContent = Get-Content $envFile -Raw }
if (-not $ZoomSdkPath) {
    Write-Host "  NOTE: Zoom SDK was not installed. The app will run in stub mode." -ForegroundColor Yellow
    Write-Host "  To install later:" -ForegroundColor Yellow
    Write-Host "    .\setup.ps1 -ZoomSdkPath ""C:\path\to\zoom-sdk\x64""" -ForegroundColor Yellow
    Write-Host ""
}
if ($envContent -match "ZOOM_SDK_KEY=\s*$" -or $envContent -match "ZOOM_SDK_KEY=$" -or -not (Test-Path $envFile)) {
    Write-Host "  REMINDER: Edit .env with your Zoom SDK credentials before joining meetings." -ForegroundColor Yellow
    Write-Host ""
}
