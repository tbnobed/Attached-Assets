# Zoom ISO Capture

## Project Overview
Electron desktop application for capturing isolated video/audio feeds from Zoom Video SDK sessions. Designed for professional broadcast TV production.

## Architecture
- **Runtime**: Node.js 20 with Electron
- **Main Process** (`src/main/`): Session management, NDI output, FFmpeg recording, IPC handlers
- **Renderer** (`src/renderer/`): Dashboard UI with participant grid, controls, activity log
- **Zoom SDK** (`src/zoom/`): Session manager, stream handler, JWT generator
- **NDI** (`src/ndi/`): NDI source management via grandiose (graceful fallback if unavailable)
- **Recorder** (`src/recorder/`): FFmpeg-based per-participant MP4 recording
- **Config** (`src/config/`): Settings loader from environment variables

## Key Files
- `src/main/main.js` - Electron main entry point
- `src/main/preload.js` - Context bridge for secure IPC
- `src/main/ipc-handlers.js` - All IPC command handlers
- `src/renderer/index.html` - Complete dashboard UI
- `src/zoom/session-manager.js` - Zoom session lifecycle
- `src/zoom/jwt-generator.js` - JWT token generation (no external deps)
- `src/ndi/ndi-manager.js` - NDI source creation/destruction
- `src/recorder/recorder-manager.js` - FFmpeg spawn and pipe management
- `src/config/settings.js` - Environment variable loader

## Dependencies
- electron - Desktop app framework
- jsonwebtoken - JWT generation
- ffmpeg - System dependency for recording

## Environment Variables
- `ZOOM_SDK_KEY` - Zoom Video SDK key
- `ZOOM_SDK_SECRET` - Zoom Video SDK secret
- `DEFAULT_OUTPUT_DIR` - Recording output directory (default: ./recordings)
- `SESSION_NAME` - Default session name
- `SESSION_PASSWORD` - Optional session password

## Guest Portal (`guest-portal/`)
- Standalone Express server for remote guests to join sessions via browser
- Serves a web page using the Zoom Video SDK for Web (CDN-loaded)
- Generates JWT tokens server-side for guests (role=0, participant)
- Has its own `package.json` — deployed independently on a Linux server
- Requires same `ZOOM_SDK_KEY`, `ZOOM_SDK_SECRET`, and `SESSION_NAME` as the desktop app
- Guests open the URL in Chrome/Edge, enter name, select camera/mic, and join
- Key files: `guest-portal/server.js`, `guest-portal/public/index.html`, `guest-portal/public/app.js`, `guest-portal/public/styles.css`

## Running
- Desktop App Workflow: VNC-based Electron app via `npx electron .`
- Guest Portal: `cd guest-portal && npm install && npm start` (runs on port 3000)
- The app uses `--no-sandbox` flag for Replit compatibility
