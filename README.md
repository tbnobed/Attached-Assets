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
- FFmpeg (included in project dependencies)

### Environment Variables

Set the following environment variables (or create a `.env` file):

```
ZOOM_SDK_KEY=your_sdk_key_here
ZOOM_SDK_SECRET=your_sdk_secret_here
DEFAULT_OUTPUT_DIR=./recordings
SESSION_NAME=PlexStudioSession
SESSION_PASSWORD=optional
```

### Install & Run

```bash
npm install
npm start
```

## Connecting OBS to NDI Sources

1. Install NDI Tools from https://ndi.video/tools/
2. Install the OBS NDI plugin
3. Launch Zoom ISO Capture and join a session
4. In OBS, add a new "NDI Source" input
5. Select the participant source (e.g., `ZoomISO_ISO1_Alice_Video`)
6. Each participant appears as a separate NDI source

## Architecture

```
src/
  main/         - Electron main process (session management, NDI, recording)
  renderer/     - Dashboard UI (participant grid, controls, status)
  zoom/         - Zoom Video SDK session wrapper and JWT generation
  ndi/          - NDI source management per participant (via grandiose)
  recorder/     - FFmpeg-based ISO recording per participant
  config/       - Settings management
```

## Project Structure

- **Main Process**: Handles all heavy stream processing (video/audio capture, NDI output, FFmpeg recording)
- **Renderer Process**: Lightweight UI that receives status updates via IPC
- **IPC Bridge**: Secure context-isolated communication between main and renderer

## Tech Stack

- Electron (desktop runtime)
- Zoom Video SDK (@zoom/videosdk)
- grandiose (NDI output, optional)
- FFmpeg (ISO recording)
- Vanilla JS UI with broadcast-style dark theme
