# ZoomLink

## Project Overview
Electron desktop application that bridges Zoom meetings to professional broadcast production. Joins standard Zoom meetings via Meeting ID/passcode and captures per-participant raw video (I420->RGBA) and audio (PCM) streams, outputting them as named NDI sources, SDI via DeckLink, and/or recording separate MP4 files per participant.

## Architecture

### Two-Machine Split Architecture
- **Mac App** (`mac-app/`): Pure Zoom capture client — joins Zoom, captures ISO video/audio, streams all frames over TCP to the Linux server, and provides UI to control the server. Does NOT do NDI, DeckLink, or recording locally.
- **Linux Server** (`linux-server/`): Does ALL heavy lifting — receives frames over TCP, outputs to DeckLink SDI, NDI, and MP4 recording.
- **Original App** (`src/`): Must NEVER be modified. Contains shared business logic (session-manager, stream-handler, settings, user-config) that both the original app and mac-app reference.

### Original App (src/) — DO NOT MODIFY
- **Runtime**: Node.js with Electron (macOS/Windows desktop)
- **Main Process** (`src/main/`): Session management, NDI output, FFmpeg recording, IPC handlers
- **Renderer** (`src/renderer/`): Dashboard UI with participant grid, meeting join controls, activity log
- **Zoom Integration** (`src/zoom/`): Session manager wrapping the native Meeting SDK addon, stream handler, JWT generator
- **Native Addon** (`zoom-meeting-sdk-addon/`): C++/ObjC++ N-API wrapper around the Zoom Meeting SDK. macOS uses Objective-C ZoomSDK.framework API; Windows uses C++ zoom_sdk API. Both provide raw data callbacks (video I420, audio PCM) per participant via ThreadSafeFunction
- **NDI** (`src/ndi/`): NDI source management via grandiose (graceful fallback if unavailable)
- **DeckLink** (`src/decklink/` + `decklink-output-addon/`): SDI output via custom C++ native addon
- **Recorder** (`src/recorder/`): FFmpeg-based per-participant MP4 recording
- **Config** (`src/config/`): Settings loader from environment variables + persistent user config

## Mac App (`mac-app/`) — Capture Client
```
mac-app/
├── package.json              # Standalone app config
├── main.js                   # Electron main — SessionManager + StreamHandler + RemoteSDIClient only
├── ipc-handlers.js           # Meeting + remote SDI IPC handlers (NO NDI/DeckLink/recorder)
├── preload.js                # Context bridge: meeting + remote SDI methods (NO NDI/DeckLink/recorder)
├── network/stream-client.js  # TCP client — binary protocol to linux-server
├── network/frame-compressor.js # JPEG compress via nativeImage (zero deps)

└── renderer/
    ├── index.html            # Simplified UI: participant feeds + Remote SDI connection + SDI assignments
    └── assets/logo.png       # App logo
```

- **Stripped of**: NDIManager, DeckLinkManager, RecorderManager — these are all on the Linux server now
- **Frame Compression**: JPEG compression via Electron's `nativeImage.createFromBitmap().toJPEG(92)` — reduces ~900KB/frame (360p BGRA) to ~30-60KB JPEG. No extra dependencies needed on Mac side.
- **RemoteSDIClient**: TCP binary protocol (magic `0x5A4C4B31`, 23-byte header), unlimited auto-reconnect with capped backoff, heartbeat, TCP keepalive, back-pressure aware
- **Device Discovery**: On connect, queries Linux server HTTP API (`/api/devices`) at TCP port + 1 to get actual DeckLink device count and names
- **UI shows**: Participant video previews, Remote SDI connection bar, dynamic SDI assignment grid (shows actual server outputs), activity log
- **UI does NOT show**: Local NDI controls, DeckLink device panel, recording buttons, output directory picker
- Run: `cd mac-app && npx electron . --no-sandbox --disable-gpu-sandbox`

## Linux Server (`linux-server/`) — Output Server
```
linux-server/
├── server.js           # Main entry — wires DeckLink + NDI + recorder on every frame
├── receiver.js         # TCP frame receiver, decodes binary protocol
├── protocol.js         # Binary TCP protocol (encode/decode)
├── decklink-manager.js # DeckLink device management, frame pump, BGRA→UYVY
├── ndi-manager.js      # NDI source management via grandiose (BGRA video + PCM→float32 audio)
├── recorder-manager.js # FFmpeg recording (libx264/aac, BGRA pipe:0 + s16le pipe:3)
├── api.js              # REST API: status, devices, assign/unassign, NDI CRUD, recording start/stop
├── web/index.html      # Web dashboard with DeckLink + NDI + recording controls
├── install.sh          # One-command setup for Ubuntu
├── decklink-addon/     # Native C++ addon for Linux DeckLink SDK (ships own DeckLinkAPIDispatch.cpp for Linux)
├── frame-decoder.js    # JPEG auto-detect + sharp decode → BGRA for DeckLink
├── package.json        # Dependencies: sharp, grandiose (optional)
└── README.md           # Full setup and API documentation
```

### Linux Server API Endpoints
- `GET /api/status` — Full server status (receiver, decklink, ndi, recording)
- `GET /api/devices` — List DeckLink devices and display modes
- `GET /api/participants` — Current participants
- `POST /api/output/start` — Start DeckLink output (deviceIndex, mode)
- `POST /api/output/stop` — Stop DeckLink output
- `POST /api/assign` — Assign participant to DeckLink device
- `POST /api/unassign` — Unassign participant
- `POST /api/ndi/create` — Create NDI source for participant
- `POST /api/ndi/destroy` — Destroy NDI source
- `POST /api/ndi/toggle` — Enable/disable NDI source
- `POST /api/recording/start` — Start recording for participant
- `POST /api/recording/stop` — Stop recording
- `POST /api/recording/start-all` — Start recording all participants
- `POST /api/recording/stop-all` — Stop all recordings
- `POST /api/recording/set-dir` — Set recording output directory

### Linux Server Environment Variables
- `TCP_PORT=9300` — Frame stream port
- `API_PORT=9301` — HTTP API + dashboard port
- `NDI_PREFIX=ZoomLink` — NDI source name prefix
- `RECORDING_DIR=./recordings` — Recording output directory

### Linux Server Data Flow
1. Mac app connects via TCP port 9300
2. Mac app JPEG-compresses BGRA frames via `nativeImage.toJPEG(92)` before sending (~30-60KB vs ~900KB raw)
3. Sends binary packets: VIDEO_FRAME(0x01), AUDIO_DATA(0x02), PARTICIPANT_JOIN(0x10), PARTICIPANT_LEAVE(0x11), ASSIGNMENT_UPDATE(0x20), HEARTBEAT(0xFF)
4. Server auto-detects JPEG (magic bytes 0xFF 0xD8) and decodes via `sharp` back to BGRA
5. Server routes every video-frame to: DeckLinkManager + NDIManager + RecorderManager
6. Server routes every audio-data to: DeckLinkManager + NDIManager + RecorderManager
7. NDI auto-creates source on participant-join, destroys on participant-leave
8. Web dashboard on port 9301 provides full control

## Key Files (Original — DO NOT MODIFY)
- `src/main/main.js` - Electron main entry point
- `src/main/preload.js` - Context bridge for secure IPC
- `src/main/ipc-handlers.js` - All IPC command handlers
- `src/renderer/index.html` - Complete dashboard UI
- `src/zoom/session-manager.js` - Zoom session lifecycle
- `src/zoom/stream-handler.js` - Routes per-participant video/audio frames
- `src/zoom/jwt-generator.js` - JWT token generation
- `src/ndi/ndi-manager.js` - NDI source creation/destruction
- `src/decklink/decklink-manager.js` - DeckLink SDI output
- `src/recorder/recorder-manager.js` - FFmpeg spawn and pipe management
- `src/config/settings.js` - Environment variable loader
- `src/config/user-config.js` - Persistent user config

## Platform Support

### macOS (Primary) — Objective-C Framework API
- SDK: `ZoomSDK.framework` (Objective-C, NOT C++)
- CRITICAL: ZoomSDK.framework uses `@executable_path/../Frameworks` rpath
- SDK path: `/Users/debo/Documents/zoom-sdk-macos-6.7.6.75900/ZoomSDK/`

### Windows (Legacy) — C++ API
- SDK uses `const wchar_t*` for string types
- SDK binary: `sdk.dll` + `sdk.lib`

## Dependencies
- electron - Desktop app framework
- grandiose - NDI output (native module, requires NDI SDK) — used on Linux server
- decklink-output-addon - Custom DeckLink SDI output (native module) — used on Linux server
- node-addon-api - For the Zoom Meeting SDK native addon
- ffmpeg - System dependency for recording — used on Linux server

## Environment Variables
- `ZOOM_SDK_KEY` - Zoom Meeting SDK key
- `ZOOM_SDK_SECRET` - Zoom Meeting SDK secret
- `ZOOM_ACCOUNT_ID` - Zoom Server-to-Server OAuth Account ID
- `ZOOM_CLIENT_ID` - Zoom Server-to-Server OAuth Client ID
- `ZOOM_CLIENT_SECRET` - Zoom Server-to-Server OAuth Client Secret
- `ZOOM_MEETING_BOT_NAME` - Display name when joining meetings (default: ZoomLink)

## Quick Setup
Mac (capture client):
```bash
cd mac-app && npm install
npx electron . --no-sandbox --disable-gpu-sandbox
```

Linux (output server):
```bash
cd linux-server && bash install.sh
node server.js
```

## Zoom SDK Notes
- Raw data requires StartRawRecording() BEFORE createRenderer/audio subscribe
- Video resolution set to 360p for reliable multi-stream capture
- Bot's own userId excluded from subscriptions
- RetryVideoSubscriptions detects stuck renderers and recreates them
