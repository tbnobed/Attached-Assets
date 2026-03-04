# Zoom ISO Capture

Professional broadcast-grade application for capturing isolated video and audio feeds from Zoom Video SDK sessions. Each participant's feed is captured independently with no overlays, enabling NDI output for live switching and local ISO recording.

## Features

- **Isolated Video Capture**: Per-participant raw video at 1080p/30fps (720p fallback)
- **Isolated Audio Capture**: Per-participant clean audio at 48kHz/16-bit
- **NDI Output**: Each participant pushed as named NDI source for OBS/hardware switchers
- **Local ISO Recording**: Separate MP4 per participant via FFmpeg
- **Test Pattern Mode**: Generate placeholder NDI sources for infrastructure testing
- **Session Reconnection**: Automatic reconnect on connection drops
- **Broadcast-Style UI**: Dark, minimal dashboard with live thumbnails and controls

## Setup

### Prerequisites

- Node.js 20+
- NDI Tools (for NDI output): https://ndi.video/tools/
- FFmpeg: https://ffmpeg.org/download.html (add bin folder to system PATH)
- Visual Studio Build Tools with "Desktop development with C++" (for NDI native module)

### Install & Run

```bash
npm install
npm start
```

### Environment Variables

Create a `.env` file in the project root:

```
ZOOM_SDK_KEY=your_sdk_key_here
ZOOM_SDK_SECRET=your_sdk_secret_here
DEFAULT_OUTPUT_DIR=./recordings
SESSION_NAME=PlexStudioSession
SESSION_PASSWORD=optional
```

Get your SDK Key and Secret from the Zoom Marketplace (https://marketplace.zoom.us/) — create a "Video SDK" app type.

### Installing NDI Support

After `npm install`, install the NDI module:

```bash
npm install grandiose
```

If you get compilation errors about `const char*` (common on newer Visual Studio versions), run the included patch script:

```bash
npm run fix-ndi
```

This automatically patches the grandiose source code for MSVC const-correctness and rebuilds the native module.

**Requirements for grandiose to compile:**
1. NDI Tools installed (https://ndi.video/tools/)
2. Visual Studio Build Tools with "Desktop development with C++" workload
3. Python 3.x (required by node-gyp)
4. Node.js 20+

If NDI support is not installed, the app still runs — recording and all other features work fine. NDI output will show as "Unavailable" in the dashboard.

## Connecting OBS to NDI Sources

1. Install NDI Tools from https://ndi.video/tools/
2. Install the OBS NDI plugin (https://github.com/obs-ndi/obs-ndi/releases)
3. Launch Zoom ISO Capture and join a session
4. In OBS, click "+" under Sources, choose "NDI Source"
5. Select the participant source (e.g., `ZoomISO_ISO1_Alice_Video`)
6. Each participant appears as a separate NDI source

Hardware switchers on the same network will also see these sources automatically.

## Testing Without a Zoom Session

1. Click "Test Pattern" in the toolbar — creates 8 NDI sources with color bars
2. Confirm you see them in OBS or your switcher
3. Click "+ Participant" to add simulated participants for testing recording and UI

## Architecture

```
src/
  main/         - Electron main process (session management, NDI, recording)
  renderer/     - Dashboard UI (participant grid, controls, status)
  zoom/         - Zoom Video SDK session wrapper and JWT generation
  ndi/          - NDI source management per participant (via grandiose)
  recorder/     - FFmpeg-based ISO recording per participant
  config/       - Settings management
scripts/
  fix-grandiose.js - Patches grandiose for MSVC compilation
```

## Tech Stack

- Electron (desktop runtime)
- Zoom Video SDK (@zoom/videosdk)
- grandiose (NDI output, optional)
- FFmpeg (ISO recording)
- Vanilla JS UI with broadcast-style dark theme
