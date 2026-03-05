# Zoom ISO Capture

Professional broadcast-grade Electron application for capturing isolated video and audio feeds from Zoom meetings. Each participant's feed is captured independently via the Zoom Meeting SDK's raw data API, enabling NDI output for live switching and local ISO recording.

## Features

- **Isolated Video Capture**: Per-participant raw video at 1080p/30fps (720p fallback)
- **Isolated Audio Capture**: Per-participant clean audio at 48kHz/16-bit
- **NDI Output**: Each participant pushed as named NDI source for OBS/hardware switchers
- **Local ISO Recording**: Separate MP4 per participant via FFmpeg
- **Test Pattern Mode**: Generate placeholder NDI sources for infrastructure testing
- **Broadcast-Style UI**: Dark, minimal dashboard with live thumbnails and controls

## Mac Studio Setup (Step by Step)

### Step 1: Install Homebrew (if needed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Step 2: Install System Dependencies

```bash
brew install node@20 python3 ffmpeg
```

Add Node to your PATH if needed:
```bash
echo 'export PATH="/opt/homebrew/opt/node@20/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

Verify everything:
```bash
node --version       # 20+
ffmpeg -version
python3 --version
xcode-select --install   # Xcode Command Line Tools (needed for native compilation)
```

### Step 3: Install Node Dependencies

```bash
cd /path/to/zoom-iso-capture
npm install
cd zoom-meeting-sdk-addon && npm install && cd ..
```

### Step 4: Install the Zoom Meeting SDK

The install script auto-discovers headers and libraries inside the SDK folder:

```bash
node scripts/install-zoom-sdk.js /Users/debo/Documents/zoom-sdk-macos-6.7.6.75900
```

The script scans for `zoom_sdk.h` and `.dylib`/`.framework` files recursively, so it works regardless of the SDK's internal folder structure.

### Step 5: Build the Native Addon

```bash
npm run build-addon
```

If successful, you'll see `zoom-meeting-sdk-addon/build/Release/zoom_meeting_sdk.node`.

### Step 6: NDI Support (Optional)

NDI Tools for Mac installs `libndi.dylib` inside application bundles. To make it available for grandiose:

```bash
sudo mkdir -p /usr/local/lib
sudo cp "/Library/CoreMediaIO/Plug-Ins/DAL/NDIVideoOut.plugin/Contents/Frameworks/libndi.dylib" /usr/local/lib/
sudo mkdir -p /usr/local/include
```

Then install grandiose:
```bash
npm install grandiose
```

If grandiose won't compile, skip it — the app works without NDI. Recording still captures every participant to separate MP4 files.

### Step 7: Configure Environment

```bash
cp .env.example .env
```

Edit `.env`:
```
ZOOM_SDK_KEY=your_meeting_sdk_key
ZOOM_SDK_SECRET=your_meeting_sdk_secret
ZOOM_ACCOUNT_ID=your_account_id
ZOOM_CLIENT_ID=your_client_id
ZOOM_CLIENT_SECRET=your_client_secret
DEFAULT_OUTPUT_DIR=./recordings
```

### Step 8: Run

```bash
npm start
```

## Quick Reference

| Command | What it does |
|---|---|
| `npm start` | Launch the app |
| `npm run build-addon` | Rebuild the native C++ addon |
| `npm run install-sdk -- /path/to/sdk` | Install/update Zoom SDK files |

## Troubleshooting

- **"Native addon not available"** — Rebuild with `npm run build-addon` and check for compile errors.
- **"dyld: Library not loaded"** — The dylib can't be found. Make sure `build/Release/libmeetingsdk.dylib` exists after building.
- **FFmpeg not found** — Run `brew install ffmpeg` and verify with `which ffmpeg`.
- **NDI unavailable** — Expected if grandiose isn't installed. Recording still works.
- **Recording permission denied** — The bot needs the meeting host to grant local recording permission, or configure Server-to-Server OAuth credentials for auto-authorization.
- **Xcode errors during build** — Run `xcode-select --install` to install Command Line Tools.

## Architecture

```
src/
  main/         - Electron main process (session management, NDI, recording)
  renderer/     - Dashboard UI (participant grid, controls, status)
  zoom/         - Zoom Meeting SDK session wrapper and JWT generation
  ndi/          - NDI source management per participant (via grandiose)
  recorder/     - FFmpeg-based ISO recording per participant
  config/       - Settings management
zoom-meeting-sdk-addon/
  src/          - C++ native addon wrapping Zoom Meeting SDK raw data API
  binding.gyp   - Build config (macOS dylib / Windows lib+dll)
  index.js      - JS bridge with platform-aware library loading
scripts/
  install-zoom-sdk.js - Auto-discovers and copies SDK files, fixes dylib paths
```
