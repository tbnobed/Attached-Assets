# Zoom ISO Capture

## Project Overview
Electron desktop application for capturing isolated video/audio feeds from Zoom meetings. Designed for professional broadcast TV production. Joins standard Zoom meetings via Meeting ID/passcode and captures per-participant raw video (I420->RGBA) and audio (PCM) streams, outputting them as named NDI sources and/or recording separate MP4 files per participant.

## Architecture
- **Runtime**: Node.js with Electron (macOS/Windows desktop)
- **Main Process** (`src/main/`): Session management, NDI output, FFmpeg recording, IPC handlers
- **Renderer** (`src/renderer/`): Dashboard UI with participant grid, meeting join controls, activity log
- **Zoom Integration** (`src/zoom/`): Session manager wrapping the native Meeting SDK addon, stream handler, JWT generator
- **Native Addon** (`zoom-meeting-sdk-addon/`): C++ N-API wrapper around the Zoom Meeting SDK, providing raw data callbacks (video I420, audio PCM) per participant. Supports both macOS and Windows via platform-specific `#ifdef` blocks
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
- `src/zoom/recording-token.js` - Fetches local recording join token via Zoom REST API (Server-to-Server OAuth)
- `src/ndi/ndi-manager.js` - NDI source creation/destruction
- `src/recorder/recorder-manager.js` - FFmpeg spawn and pipe management
- `src/config/settings.js` - Environment variable loader
- `zoom-meeting-sdk-addon/index.js` - JS bridge for the native C++ addon (handles DLL/dylib paths per platform)
- `zoom-meeting-sdk-addon/src/zoom_addon.cpp` - N-API entry point
- `zoom-meeting-sdk-addon/src/zoom_addon.h` - Shared header (ZoomAddon singleton, structs, enums)
- `zoom-meeting-sdk-addon/src/zoom_auth.cpp` - SDK init/auth (JWT-based, platform-specific)
- `zoom-meeting-sdk-addon/src/zoom_meeting.cpp` - Meeting join/leave, participant events (platform-specific string handling)
- `zoom-meeting-sdk-addon/src/zoom_rawdata.cpp` - Raw data capture (I420->RGBA video, PCM audio) — shared between macOS and Windows
- `zoom-meeting-sdk-addon/binding.gyp` - Build config for node-gyp (macOS dylib / Windows lib+dll)
- `scripts/install-zoom-sdk.js` - Copies SDK files from download to addon directory (platform-aware)
- `scripts/scan-zoom-sdk.js` - Lists SDK headers/libs/bins for debugging
- `scripts/fix-grandiose.js` - Patches grandiose for MSVC const-correctness

## Platform Support

### macOS (Primary)
- SDK uses `const char*` (UTF-8) for all string types (`zchar_t`)
- No HWND or Windows API dependencies
- `InitParam` only needs `strWebDomain` and `enableLogByDefault`
- `JoinParam4WithoutLogin` uses `const char*` for `userName`, `psw`, `app_privilege_token`
- Meeting number parsed via `std::stoll()` instead of `_wtoi64()`
- SDK binary: `libmeetingsdk.dylib` in `sdk/lib/`
- DYLD_LIBRARY_PATH set automatically by `index.js`

### Windows (Legacy)
- SDK uses `const wchar_t*` for string types
- Requires hidden HWND for SDK initialization
- `InitParam` needs `hResInstance = GetModuleHandle(nullptr)`
- Uses `std::wstring` conversions for all SDK string parameters
- SDK binary: `sdk.dll` + `sdk.lib`

## Data Flow
1. Native addon joins Zoom meeting, MEETING_STATUS_INMEETING fires
2. C++ calls StartRawRecording() on IMeetingRecordingController — if bot isn't host, requests recording privilege
3. onRecordingStatus(Recording_Start) callback fires -> OnRawRecordingStarted() -> StartRawDataCapture()
4. StartRawDataCapture subscribes audio (GetAudioRawdataHelper) + creates IZoomSDKRenderer per existing participant
5. C++ actively enumerates participants via GetParticipantsList() + onUserJoin callback (dual approach)
6. JS bridge polls enumerateParticipants() at 0.5s, 2s, 5s, 10s after joining for reliability
7. RetryVideoSubscriptions at 3s/8s/15s cascades: StartRawRecording -> StartRawDataCapture -> subscribe missing
8. Per-participant I420 video frames -> converted to RGBA in C++ -> sent via ThreadSafeFunction to JS
9. Per-participant PCM audio -> sent via ThreadSafeFunction to JS
10. SessionManager receives frames -> routes to StreamHandler
11. StreamHandler emits video-frame/audio-data events
12. main.js routes to NDIManager (RGBA frames -> NDI sources) and RecorderManager (FFmpeg pipes)

## Dependencies
- electron - Desktop app framework
- grandiose - NDI output (native module, requires NDI SDK)
- node-addon-api - For the Zoom Meeting SDK native addon
- ffmpeg - System dependency for recording

## Environment Variables
- `ZOOM_SDK_KEY` - Zoom Meeting SDK key (App Credentials)
- `ZOOM_SDK_SECRET` - Zoom Meeting SDK secret (App Credentials)
- `ZOOM_ACCOUNT_ID` - Zoom Server-to-Server OAuth Account ID (for local recording token)
- `ZOOM_CLIENT_ID` - Zoom Server-to-Server OAuth Client ID (for local recording token)
- `ZOOM_CLIENT_SECRET` - Zoom Server-to-Server OAuth Client Secret (for local recording token)
- `ZOOM_MEETING_BOT_NAME` - Display name when joining meetings (default: PlexISO)
- `DEFAULT_OUTPUT_DIR` - Recording output directory (default: ./recordings)

## Guest Portal (`guest-portal/`)
- Standalone Express server for remote guests to join sessions via browser (Video SDK mode)
- Serves a web page using the Zoom Video SDK for Web (CDN-loaded)
- Generates JWT tokens server-side for guests (role=0, participant)
- Has its own `package.json` — deployed independently on a Linux server

## Quick Setup (macOS)
```bash
npm install
cd zoom-meeting-sdk-addon && npm install && cd ..
node scripts/install-zoom-sdk.js ~/Downloads/zoom-meeting-sdk-mac-6.x.x
cd zoom-meeting-sdk-addon && npx node-gyp rebuild && cd ..
```

Create `.env` with your ZOOM_SDK_KEY and ZOOM_SDK_SECRET, then:
```bash
npm start
```

## Quick Setup (Windows)
One command does everything:
```
setup.bat "C:\path\to\zoom-sdk\x64"
```
Or via Node:
```
node scripts/setup.js "C:\path\to\zoom-sdk\x64"
```

## Zoom SDK Notes
- Raw data requires `StartRawRecording()` on IMeetingRecordingController BEFORE createRenderer/audio subscribe
- InitParam.rawdataOpts.enableRawdataIntermediateMode MUST be false — true produces empty frames
- Bot's own userId (selfUserId_) is detected via GetMySelfUser() at INMEETING and excluded from video subscriptions
- CRITICAL ORDERING: EnumerateParticipants MUST run inside OnRawRecordingStarted (after onRecordingStatus(Recording_Start) callback), NOT at MEETING_STATUS_INMEETING
- On Video_ON: if already subscribed with RawData_On fired, keep working renderer. If subscribed but RawData_On never fired (stuck), destroy+recreate
- RetryVideoSubscriptions detects stuck renderers and recreates them
- If bot lacks recording permission, call `RequestLocalRecordingPrivilege()` and wait for `onRecordPrivilegeChanged(true)`
- `onRecordingStatus(Recording_Start)` is the gate signal: only after this can renderers and audio subscriptions succeed
