# Zoom ISO Capture

## Project Overview
Electron desktop application for capturing isolated video/audio feeds from Zoom meetings. Designed for professional broadcast TV production. Joins standard Zoom meetings via Meeting ID/passcode and captures per-participant raw video (I420->RGBA) and audio (PCM) streams, outputting them as named NDI sources and/or recording separate MP4 files per participant.

## Architecture
- **Runtime**: Node.js with Electron (Windows desktop)
- **Main Process** (`src/main/`): Session management, NDI output, FFmpeg recording, IPC handlers
- **Renderer** (`src/renderer/`): Dashboard UI with participant grid, meeting join controls, activity log
- **Zoom Integration** (`src/zoom/`): Session manager wrapping the native Meeting SDK addon, stream handler, JWT generator
- **Native Addon** (`zoom-meeting-sdk-addon/`): C++ N-API wrapper around the Zoom Meeting SDK for Windows, providing raw data callbacks (video I420, audio PCM) per participant
- **NDI** (`src/ndi/`): NDI source management via grandiose (graceful fallback if unavailable)
- **Recorder** (`src/recorder/`): FFmpeg-based per-participant MP4 recording
- **Config** (`src/config/`): Settings loader from environment variables

## Key Files
- `src/main/main.js` - Electron main entry point
- `src/main/preload.js` - Context bridge for secure IPC (joinMeeting, leaveMeeting, etc.)
- `src/main/ipc-handlers.js` - All IPC command handlers
- `src/renderer/index.html` - Complete dashboard UI with meeting join toolbar
- `src/zoom/session-manager.js` - Zoom session lifecycle, bridges native addon to stream handler
- `src/zoom/stream-handler.js` - Routes per-participant video/audio frames
- `src/zoom/jwt-generator.js` - JWT token generation (no external deps)
- `src/ndi/ndi-manager.js` - NDI source creation/destruction
- `src/recorder/recorder-manager.js` - FFmpeg spawn and pipe management
- `src/config/settings.js` - Environment variable loader
- `zoom-meeting-sdk-addon/index.js` - JS bridge for the native C++ addon
- `zoom-meeting-sdk-addon/src/zoom_addon.cpp` - N-API entry point
- `zoom-meeting-sdk-addon/src/zoom_auth.cpp` - SDK init/auth
- `zoom-meeting-sdk-addon/src/zoom_meeting.cpp` - Meeting join/leave, participant events
- `zoom-meeting-sdk-addon/src/zoom_rawdata.cpp` - Raw data capture (I420->RGBA video, PCM audio)
- `zoom-meeting-sdk-addon/binding.gyp` - Build config for node-gyp
- `scripts/setup-meeting-sdk.js` - Setup script for SDK file placement
- `scripts/fix-grandiose.js` - Patches grandiose for MSVC const-correctness

## Data Flow
1. Native addon joins Zoom meeting, calls StartRawRecording()
2. Per-participant I420 video frames -> converted to RGBA in C++ -> sent via ThreadSafeFunction to JS
3. Per-participant PCM audio -> sent via ThreadSafeFunction to JS
4. SessionManager receives frames -> routes to StreamHandler
5. StreamHandler emits video-frame/audio-data events
6. main.js routes to NDIManager (RGBA frames -> NDI sources) and RecorderManager (FFmpeg pipes)

## Dependencies
- electron - Desktop app framework
- grandiose - NDI output (native module, requires NDI SDK)
- node-addon-api - For the Zoom Meeting SDK native addon
- ffmpeg - System dependency for recording

## Environment Variables
- `ZOOM_SDK_KEY` - Zoom Meeting SDK key
- `ZOOM_SDK_SECRET` - Zoom Meeting SDK secret
- `ZOOM_MEETING_BOT_NAME` - Display name when joining meetings (default: PlexISO)
- `DEFAULT_OUTPUT_DIR` - Recording output directory (default: ./recordings)
- `SESSION_NAME` - Default session name (used for Video SDK fallback)
- `SESSION_PASSWORD` - Optional session password

## Guest Portal (`guest-portal/`)
- Standalone Express server for remote guests to join sessions via browser (Video SDK mode)
- Serves a web page using the Zoom Video SDK for Web (CDN-loaded)
- Generates JWT tokens server-side for guests (role=0, participant)
- Has its own `package.json` — deployed independently on a Linux server
- Key files: `guest-portal/server.js`, `guest-portal/public/index.html`, `guest-portal/public/app.js`, `guest-portal/public/styles.css`

## Running
- Desktop App: `npm start` (runs `electron .`)
- Guest Portal: `cd guest-portal && npm install && npm start` (runs on port 3000)
- VNC Workflow: `npx electron . --no-sandbox --disable-gpu-sandbox` (Replit dev)

## Building the Native Addon (Windows)
1. Run `node scripts/setup-meeting-sdk.js` to create directory structure
2. Download Zoom Meeting SDK from marketplace.zoom.us
3. Copy SDK headers to `zoom-meeting-sdk-addon/sdk/h/`
4. Copy .lib to `zoom-meeting-sdk-addon/sdk/lib/x64/`
5. Copy .dll files to `zoom-meeting-sdk-addon/sdk/bin/x64/`
6. `cd zoom-meeting-sdk-addon && npm install`
7. `npx node-gyp rebuild --target=<electron-version> --arch=x64 --dist-url=https://electronjs.org/headers`
