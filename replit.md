# Zoom ISO Capture

## Project Overview
Electron desktop application for capturing isolated video/audio feeds from Zoom meetings. Designed for professional broadcast TV production. Joins standard Zoom meetings via Meeting ID/passcode and captures per-participant raw video (I420->RGBA) and audio (PCM) streams, outputting them as named NDI sources and/or recording separate MP4 files per participant.

## Architecture
- **Runtime**: Node.js with Electron (macOS/Windows desktop)
- **Main Process** (`src/main/`): Session management, NDI output, FFmpeg recording, IPC handlers
- **Renderer** (`src/renderer/`): Dashboard UI with participant grid, meeting join controls, activity log
- **Zoom Integration** (`src/zoom/`): Session manager wrapping the native Meeting SDK addon, stream handler, JWT generator
- **Native Addon** (`zoom-meeting-sdk-addon/`): C++/ObjC++ N-API wrapper around the Zoom Meeting SDK. macOS uses Objective-C ZoomSDK.framework API; Windows uses C++ zoom_sdk API. Both provide raw data callbacks (video I420, audio PCM) per participant via ThreadSafeFunction
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
- Init: `[[ZoomSDK sharedSDK] initSDKWithParams:]` with `needCustomizedUI=YES`, heap raw data modes
- Auth: `ZoomSDKAuthContext.jwtToken` + `[authService sdkAuth:]`
- Join: `ZoomSDKJoinMeetingElements` with `isNoVideo=YES`, `isNoAudio=YES`, `isMyVoiceInMix=NO`, `audioRawdataSamplingRate=48K`
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
13. main.js routes to NDIManager (BGRA frames → NDI sources) and RecorderManager (FFmpeg pipes)

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

## Quick Setup (macOS)
```bash
npm install
cd zoom-meeting-sdk-addon && npm install && cd ..
node scripts/install-zoom-sdk.js /Users/debo/Documents/zoom-sdk-macos-6.7.6.75900
npm run link-frameworks
cd zoom-meeting-sdk-addon && CXX="$(pwd)/build-tools/cxx-objcpp.sh" npx node-gyp rebuild && cd ..
```

Create `.env` with your ZOOM_SDK_KEY and ZOOM_SDK_SECRET, then:
```bash
npm start
```

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
- RetryVideoSubscriptions detects stuck renderers and recreates them
- If bot lacks recording permission, request privilege and wait for grant callback
- `onRecordingStatus(Recording_Start)` / `onLocalRecordStatus:ZoomSDKRecordingStatus_Start` is the gate signal for raw data
