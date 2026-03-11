# ZoomLink

## Project Overview
Electron desktop application that bridges Zoom meetings to professional broadcast production. Joins standard Zoom meetings via Meeting ID/passcode and captures per-participant raw video (I420->RGBA) and audio (PCM) streams, outputting them as named NDI sources, SDI via DeckLink, and/or recording separate MP4 files per participant.

## Architecture
- **Runtime**: Node.js with Electron (macOS/Windows desktop)
- **Main Process** (`src/main/`): Session management, NDI output, FFmpeg recording, IPC handlers
- **Renderer** (`src/renderer/`): Dashboard UI with participant grid, meeting join controls, activity log
- **Zoom Integration** (`src/zoom/`): Session manager wrapping the native Meeting SDK addon, stream handler, JWT generator
- **Native Addon** (`zoom-meeting-sdk-addon/`): C++/ObjC++ N-API wrapper around the Zoom Meeting SDK. macOS uses Objective-C ZoomSDK.framework API; Windows uses C++ zoom_sdk API. Both provide raw data callbacks (video I420, audio PCM) per participant via ThreadSafeFunction
- **NDI** (`src/ndi/`): NDI source management via grandiose (graceful fallback if unavailable)
- **DeckLink** (`src/decklink/` + `decklink-output-addon/`): SDI output via custom C++ native addon that directly calls the Blackmagic DeckLink SDK v15.2 (replaced macadam). Enumerates DeckLink devices, maps participants to physical outputs, converts BGRA→UYVY with scaling, upmixes mono audio to stereo. Uses `DisplayVideoFrameSync` + `WriteAudioSamplesSync` for synchronous frame output. SDK v15.2 specifics: `IDeckLinkMutableVideoFrame` does NOT inherit from `IDeckLinkVideoFrame` and has no `GetBytes`; must use `CreateVideoFrameWithBuffer` with a custom `SimpleVideoBuffer` (implements `IDeckLinkVideoBuffer`) to provide the pixel buffer, then QI the mutable frame for `IDeckLinkVideoFrame` (via `IID_IDeckLinkVideoFrame`) to pass to `DisplayVideoFrameSync`. Uses `DeckLinkAPIDispatch.cpp` (copied locally) for `CreateDeckLinkIteratorInstance`. Audio is written per-pump-iteration with only real accumulated samples (no fixed-size padding) to prevent buffer overrun; `maxSamplesPerWrite` caps at 2× frame's worth. UI allows per-output participant assignment.
- **Recorder** (`src/recorder/`): FFmpeg-based per-participant MP4 recording
- **Config** (`src/config/`): Settings loader from environment variables + persistent user config. `user-config.js` stores credentials in `~/Library/Application Support/ZoomLink/config.json`. Settings modal in UI (gear icon) allows configuring SDK Key/Secret, OAuth credentials, and bot name. Auto-opens on first launch if no credentials are configured. Env vars take precedence over saved config.

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
- `src/decklink/decklink-manager.js` - DeckLink SDI output, BGRA→UYVY conversion, participant→device mapping
- `decklink-output-addon/src/decklink_output.mm` - C++ native addon wrapping DeckLink SDK directly (getDevices, openOutput, displayFrame, closeOutput)
- `decklink-output-addon/index.js` - JS wrapper with soft-loading and BMD constant exports
- `src/recorder/recorder-manager.js` - FFmpeg spawn and pipe management
- `src/config/settings.js` - Environment variable loader
- `src/config/user-config.js` - Persistent user config (~/Library/Application Support/ZoomLink/config.json)
- `zoom-meeting-sdk-addon/index.js` - JS bridge for the native addon (handles DLL/dylib/framework paths per platform)
- `zoom-meeting-sdk-addon/src/zoom_addon.cpp` - N-API entry point (platform-independent callbacks, participant tracking)
- `zoom-meeting-sdk-addon/src/zoom_addon.h` - Shared header (ZoomAddon singleton, structs, enums)
- `zoom-meeting-sdk-addon/src/zoom_auth.cpp` - SDK init/auth: macOS uses ZoomSDKAuthService/ZoomSDKAuthDelegate (ObjC), Windows uses IAuthService (C++)
- `zoom-meeting-sdk-addon/src/zoom_meeting.cpp` - Meeting join/leave/participants: macOS uses ZoomSDKMeetingService/ZoomSDKMeetingActionController (ObjC), Windows uses IMeetingService (C++)
- `zoom-meeting-sdk-addon/src/zoom_rawdata.cpp` - Raw data capture: macOS uses ZoomSDKRenderer/ZoomSDKRendererDelegate/ZoomSDKAudioRawDataHelper (ObjC), Windows uses IZoomSDKRenderer/IZoomSDKAudioRawDataDelegate (C++)
- `zoom-meeting-sdk-addon/binding.gyp` - Build config: macOS links ZoomSDK.framework with ObjC++ compilation; Windows links sdk.lib+dll
- `scripts/install-zoom-sdk.js` - Copies SDK files: macOS copies entire ZoomSDK.framework + companion frameworks/dylibs; Windows copies h/lib/bin dirs

## Platform Support

### macOS (Primary) — Objective-C Framework API
- SDK: `ZoomSDK.framework` (Objective-C, NOT C++)
- All SDK classes are ObjC: `ZoomSDKAuthService`, `ZoomSDKMeetingService`, `ZoomSDKRenderer`, etc.
- Delegates: `ZoomSDKAuthDelegate`, `ZoomSDKMeetingServiceDelegate`, `ZoomSDKMeetingActionControllerDelegate`, `ZoomSDKRendererDelegate`, `ZoomSDKAudioRawDataDelegate`, `ZoomSDKMeetingRecordDelegate`
- Compiled as Objective-C++ via `GCC_INPUT_FILETYPE: sourcecode.cpp.objcpp` in binding.gyp
- ARC enabled via `-fobjc-arc`
- Framework search path: `-F<addon>/sdk/lib`
- Runtime: `DYLD_FRAMEWORK_PATH` set by index.js
- **CRITICAL**: ZoomSDK.framework uses `@executable_path/../Frameworks` rpath to find companion frameworks at runtime. All SDK files from `sdk/lib/` MUST be symlinked into `Electron.app/Contents/Frameworks/`. Without this, auth silently fails with `ZoomSDKError_Failed=1`. Run `npm run link-frameworks` after `npm install`.
- SDK path: `/Users/debo/Documents/zoom-sdk-macos-6.7.6.75900/ZoomSDK/`
- Init: `[[ZoomSDK sharedSDK] initSDKWithParams:]` with `needCustomizedUI=NO` (default UI — shows standard Zoom meeting window with camera/audio device selection), heap raw data modes
- Auth: `ZoomSDKAuthContext.jwtToken` + `[authService sdkAuth:]`
- Join: `ZoomSDKJoinMeetingElements` with `isNoVideo=NO`, `isNoAudio=NO` (video/audio enabled so user can select devices in Zoom window), `isMyVoiceInMix=NO`, `audioRawdataSamplingRate=48K`
- Self detection: `[[actionCtrl getMyself] getUserID]`
- Participants: `[actionCtrl getParticipantsList]` returns NSArray of NSNumbers
- Video: `[rawCtrl createRender:&renderer]` → `renderer.delegate` → `[renderer subscribe:userId rawDataType:ZoomSDKRawDataType_Video]`
- Audio: `[rawCtrl getAudioRawDataHelper:&helper]` → `helper.delegate` → `[helper subscribe]`
- Recording: `[[meetingSvc getRecordController] startRawRecording]`
- Video frames: `ZoomSDKYUVRawDataI420` → getYBuffer/getUBuffer/getVBuffer → YUV→BGRA
- Audio frames: `ZoomSDKAudioRawData` → getBuffer/getBufferLen/getSampleRate/getChannelNum

### Windows (Legacy) — C++ API
- SDK uses `const wchar_t*` for string types
- Requires hidden HWND for SDK initialization
- `InitParam` needs `hResInstance = GetModuleHandle(nullptr)`
- Uses `std::wstring` conversions for all SDK string parameters
- SDK binary: `sdk.dll` + `sdk.lib`

## Mac Companion App (`mac-app/`)
A completely separate standalone Electron app that includes ALL original app functionality PLUS Remote SDI streaming to the Linux server. The original app files are left completely untouched — `mac-app/` references shared business logic modules from `../src/` via relative paths.

```
mac-app/
├── package.json              # Standalone app config
├── main.js                   # Electron main — all original managers + RemoteSDIClient
├── ipc-handlers.js           # All original IPC + remote-sdi-connect/disconnect/assign
├── preload.js                # All original bridge + remoteSDI* methods
├── network/stream-client.js  # TCP client — binary protocol to linux-server
└── renderer/
    ├── index.html            # Full UI with Remote SDI Server panel
    └── assets/logo.png       # App logo
```

- **RemoteSDIClient**: TCP binary protocol matching linux-server exactly (magic `0x5A4C4B31`, 23-byte header)
- Auto-reconnect with exponential backoff (up to 10 attempts), heartbeat every 5s
- Back-pressure aware: drops video/audio frames if socket can't drain
- Video frames + audio data are forwarded to remote client alongside NDI/DeckLink/recorder
- On connect, backfills all current participants to the server
- UI: host/port input, connect/disconnect, status badge, live stats (frames/dropped/bytes/fps), 8-output assignment grid
- Status update includes `remoteSDI` key with connection stats
- IPC: `remote-sdi-connect`, `remote-sdi-disconnect`, `remote-sdi-status`, `remote-sdi-assign`, `remote-sdi-unassign`
- Run: `cd mac-app && npx electron . --no-sandbox --disable-gpu-sandbox`

## Data Flow
1. Native addon joins Zoom meeting, MEETING_STATUS_INMEETING fires
2. Calls StartRawRecording() — macOS: `[recCtrl startRawRecording]`; Windows: `recCtrl->StartRawRecording()`
3. If bot isn't host, requests recording privilege and waits for grant callback
4. Recording status callback fires → OnRawRecordingStarted() → StartRawDataCapture()
5. StartRawDataCapture subscribes audio + creates per-user video renderers
6. Participant enumeration via SDK list + delegate callbacks (dual approach)
7. JS bridge polls enumerateParticipants() at 0.5s, 2s, 5s, 10s after joining
8. RetryVideoSubscriptions at 3s/8s/15s cascades: StartRawRecording → StartRawDataCapture → subscribe missing
9. Per-participant I420 video frames → converted to BGRA in C++/ObjC++ → sent via ThreadSafeFunction to JS
10. Per-participant PCM audio → sent via ThreadSafeFunction to JS
11. SessionManager receives frames → routes to StreamHandler
12. StreamHandler emits video-frame/audio-data events
13. main.js routes to NDIManager (BGRA frames → NDI sources), DeckLinkManager (BGRA→UYVY for SDI output), RecorderManager (FFmpeg pipes), and RemoteSDIClient (TCP to Linux server)

## Dependencies
- electron - Desktop app framework
- grandiose - NDI output (native module, requires NDI SDK)
- decklink-output-addon - Custom DeckLink SDI output (native module, requires Blackmagic Desktop Video drivers + SDK headers)
- node-addon-api - For the Zoom Meeting SDK native addon
- ffmpeg - System dependency for recording

## Environment Variables
- `ZOOM_SDK_KEY` - Zoom Meeting SDK key (App Credentials)
- `ZOOM_SDK_SECRET` - Zoom Meeting SDK secret (App Credentials)
- `ZOOM_ACCOUNT_ID` - Zoom Server-to-Server OAuth Account ID (for local recording token)
- `ZOOM_CLIENT_ID` - Zoom Server-to-Server OAuth Client ID (for local recording token)
- `ZOOM_CLIENT_SECRET` - Zoom Server-to-Server OAuth Client Secret (for local recording token)
- `ZOOM_MEETING_BOT_NAME` - Display name when joining meetings (default: ZoomLink)
- `DEFAULT_OUTPUT_DIR` - Recording output directory (default: ./recordings)

## Quick Setup (macOS)
```bash
bash scripts/setup-mac.sh
```

Or manually:
```bash
npm install
cd zoom-meeting-sdk-addon && npm install && cd ..
node scripts/install-zoom-sdk.js /Users/debo/Documents/zoom-sdk-macos-6.7.6.75900
npm run link-frameworks
cd zoom-meeting-sdk-addon && CXX="$(pwd)/build-tools/cxx-objcpp.sh" npx node-gyp rebuild && cd ..
cd decklink-output-addon && npm install && npx node-gyp rebuild && cd ..
```

Configure credentials via the Settings UI (gear icon) or `.env` file, then:
```bash
npm start
```

## Building the .app Bundle
```bash
npm run build-app
```
Creates `dist/ZoomLink.app` — a double-clickable macOS app bundle. The setup script runs this automatically. Drag to /Applications to install.

```
npm run create-dmg
```
Creates `dist/ZoomLink-Installer.dmg` — a distributable macOS disk image with drag-to-Applications layout. Setup script builds this automatically after the .app.

## Linux SDI Server
A completely separate standalone project in `linux-server/` that receives video/audio frames from the Mac app over TCP and outputs to DeckLink SDI cards on a Linux machine.

```
linux-server/
├── server.js           # Main entry point — TCP receiver + API
├── receiver.js         # TCP frame receiver, decodes binary protocol
├── protocol.js         # Binary TCP protocol (encode/decode)
├── decklink-manager.js # DeckLink device management, frame pump, BGRA→UYVY
├── api.js              # REST API (status, devices, assign/unassign)
├── web/index.html      # Web dashboard
├── install.sh          # One-command setup for Ubuntu
├── decklink-addon/     # Native C++ addon for Linux DeckLink SDK
└── README.md           # Full setup and API documentation
```

Setup on Linux:
```bash
cd linux-server && bash install.sh
node server.js
```
TCP port 9300 (frame stream), HTTP port 9301 (API + dashboard).

## Quick Setup (Windows)
```
node scripts/install-zoom-sdk.js "C:\path\to\zoom-sdk\x64"
cd zoom-meeting-sdk-addon && npx node-gyp rebuild
```

## Zoom SDK Notes
- Raw data requires StartRawRecording() BEFORE createRenderer/audio subscribe
- macOS: `enableRawdataIntermediateMode = NO` on ZoomSDK sharedSDK
- Windows: `InitParam.rawdataOpts.enableRawdataIntermediateMode = false`
- Bot's own userId (selfUserId_) detected early in onUserJoin via getMyself/GetMySelfUser (fallback at INMEETING), excluded from video/audio subscriptions. PurgeSelfFromParticipants() cleans up if self was added before detection.
- JS bridge (`index.js`) BUFFERS participant-joined events until self-detection is complete (`_selfDetected=true`). Events are flushed after INMEETING or self-purge, filtering out the bot's own userId. This prevents the bot from appearing as a participant in the UI.
- `getSelfUserId()` is exposed from the native addon so the JS bridge can query it directly.
- NDI is AUTO-ENABLED for every participant on join (`ndiManager.toggleSource(userId, true)` in main.js).
- Status bar in UI shows meeting progress: Connecting → Waiting for permission → Raw recording active.
- Self audio frames are filtered at the native level (both platforms) to prevent bot's own audio from being sent to NDI
- RetryVideoSubscriptions calls EnumerateParticipants each cycle to discover pre-existing participants that were missed
- CRITICAL ORDERING: EnumerateParticipants MUST run inside OnRawRecordingStarted, NOT at MEETING_STATUS_INMEETING
- Video resolution set to 360p (`ZoomSDKResolution_360P`) for all renderers. At 1080p, the Zoom SDK throttles bandwidth and only delivers 1-2 concurrent video streams. At 360p, 4-8+ concurrent streams work reliably. This affects all 3 renderer creation sites (Windows initial, Windows video-status-change, macOS).
- RetryVideoSubscriptions detects stuck renderers and recreates them
- If bot lacks recording permission, request privilege and wait for grant callback
- `onRecordingStatus(Recording_Start)` / `onLocalRecordStatus:ZoomSDKRecordingStatus_Start` is the gate signal for raw data
