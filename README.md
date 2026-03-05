# Zoom ISO Capture

Professional broadcast-grade Electron application for capturing isolated video and audio feeds from Zoom meetings. Each participant's feed is captured independently via the Zoom Meeting SDK's raw data API, enabling NDI output for live switching and local ISO recording.

## Features

- **Isolated Video Capture**: Per-participant raw video at 1080p/30fps (720p fallback)
- **Isolated Audio Capture**: Per-participant clean audio at 48kHz/16-bit
- **NDI Output**: Each participant pushed as named NDI source for OBS/hardware switchers
- **Local ISO Recording**: Separate MP4 per participant via FFmpeg
- **Test Pattern Mode**: Generate placeholder NDI sources for infrastructure testing
- **Broadcast-Style UI**: Dark, minimal dashboard with live thumbnails and controls

## Prerequisites

- Node.js 20+
- macOS 10.15+ (primary) or Windows 10+ (legacy)
- Zoom Meeting SDK (download from https://marketplace.zoom.us/)
- FFmpeg (for local recording): https://ffmpeg.org/download.html
- NDI Tools (optional, for NDI output): https://ndi.video/tools/

## Setup

### 1. Install Dependencies

```bash
npm install
cd zoom-meeting-sdk-addon && npm install && cd ..
```

### 2. Install Zoom Meeting SDK

Download the macOS Meeting SDK from Zoom Marketplace, then:

```bash
node scripts/install-zoom-sdk.js ~/Downloads/zoom-meeting-sdk-mac-6.x.x
```

### 3. Build the Native Addon

```bash
npm run build-addon
```

### 4. Configure Environment

Create a `.env` file in the project root:

```
ZOOM_SDK_KEY=your_sdk_key_here
ZOOM_SDK_SECRET=your_sdk_secret_here
ZOOM_ACCOUNT_ID=your_account_id_here
ZOOM_CLIENT_ID=your_client_id_here
ZOOM_CLIENT_SECRET=your_client_secret_here
DEFAULT_OUTPUT_DIR=./recordings
```

Get your SDK Key and Secret from the Zoom Marketplace — create a "Meeting SDK" app type.

### 5. Run

```bash
npm start
```

## Installing NDI Support (Optional)

```bash
npm install grandiose
```

If NDI support is not installed, the app still runs — recording and all other features work fine. NDI output will show as "Unavailable" in the dashboard.

## Connecting OBS to NDI Sources

1. Install NDI Tools from https://ndi.video/tools/
2. Install the OBS NDI plugin (https://github.com/obs-ndi/obs-ndi/releases)
3. Launch Zoom ISO Capture and join a meeting
4. In OBS, click "+" under Sources, choose "NDI Source"
5. Select the participant source (e.g., `ZoomISO_ISO1_Alice`)
6. Each participant appears as a separate NDI source

Hardware switchers on the same network will also see these sources automatically.

## Testing Without a Zoom Meeting

1. Click "Test Pattern" in the toolbar — creates 8 NDI sources with color bars
2. Confirm you see them in OBS or your switcher
3. Click "+ Participant" to add simulated participants for testing recording and UI

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
  install-zoom-sdk.js - Copies SDK files and fixes dylib paths
```

## Tech Stack

- Electron (desktop runtime)
- Zoom Meeting SDK (native C++ addon via N-API)
- grandiose (NDI output, optional)
- FFmpeg (ISO recording)
- Vanilla JS UI with broadcast-style dark theme
