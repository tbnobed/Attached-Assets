# Zoom ISO Capture

## Project Overview
Electron desktop application for capturing isolated video/audio feeds from Zoom meetings. Designed for professional broadcast TV production. Joins standard Zoom meetings via Meeting ID/passcode and captures per-participant raw video (I420->RGBA) and audio (PCM) streams, outputting them as named NDI sources and/or recording separate MP4 files per participant.

## Architecture
- **Runtime**: Node.js with Electron (Windows desktop)
- **Main Process** (`src/main/`): Session management, NDI output, FFmpeg recording, IPC handlers
- **Renderer** (`src/renderer/`): Dashboard UI with participant grid, meeting join controls, activity log
- **Zoom Integration** (`src/zoom/`): Session manager wrapping the native Meeting SDK addon, stream handler, JWT generator
- **Native Addon** (`zoom-meeting-sdk-addon/`): C++ N-API wrapper around the Zoom Meeting SDK v6.7.5 for Windows, providing raw data callbacks (video I420, audio PCM) per participant
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
- `zoom-meeting-sdk-addon/src/zoom_addon.h` - Shared header (ZoomAddon singleton, structs, enums)
- `zoom-meeting-sdk-addon/src/zoom_auth.cpp` - SDK init/auth (JWT-based)
- `zoom-meeting-sdk-addon/src/zoom_meeting.cpp` - Meeting join/leave, participant events
- `zoom-meeting-sdk-addon/src/zoom_rawdata.cpp` - Raw data capture (I420->RGBA video, PCM audio)
- `zoom-meeting-sdk-addon/binding.gyp` - Build config for node-gyp
- `scripts/install-zoom-sdk.js` - Copies SDK files from download to addon directory
- `scripts/scan-zoom-sdk.js` - Lists SDK headers/libs/bins for debugging
- `scripts/fix-grandiose.js` - Patches grandiose for MSVC const-correctness

## Data Flow
1. Native addon joins Zoom meeting, raw capture starts automatically when MEETING_STATUS_INMEETING
2. C++ actively enumerates participants via GetParticipantsList() + onUserJoin callback (dual approach)
3. JS bridge polls enumerateParticipants() at 0.5s, 2s, 5s, 10s after joining for reliability
4. Per-participant I420 video frames -> converted to RGBA in C++ -> sent via ThreadSafeFunction to JS
5. Per-participant PCM audio -> sent via ThreadSafeFunction to JS
6. SessionManager receives frames -> routes to StreamHandler
7. StreamHandler emits video-frame/audio-data events
8. main.js routes to NDIManager (RGBA frames -> NDI sources) and RecorderManager (FFmpeg pipes)
9. NDI send uses per-source locks (_videoSending/_audioSending) to prevent promise queue overflow

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

## Guest Portal (`guest-portal/`)
- Standalone Express server for remote guests to join sessions via browser (Video SDK mode)
- Serves a web page using the Zoom Video SDK for Web (CDN-loaded)
- Generates JWT tokens server-side for guests (role=0, participant)
- Has its own `package.json` — deployed independently on a Linux server

## Quick Setup (Windows)
One command does everything:
```
setup.bat "C:\path\to\zoom-sdk\x64"
```
Or via Node:
```
node scripts/setup.js "C:\path\to\zoom-sdk\x64"
```
Or via PowerShell:
```
.\setup.ps1 -ZoomSdkPath "C:\path\to\zoom-sdk\x64"
```

The setup script will:
1. Check prerequisites (Node, Python, FFmpeg)
2. `npm install` (root + addon)
3. Copy Zoom SDK headers/libs/DLLs
4. Build the native C++ addon
5. Install and patch grandiose (NDI)
6. Create `.env` template
7. Create recordings directory

After setup, edit `.env` with your ZOOM_SDK_KEY and ZOOM_SDK_SECRET, then:
```
npm start
```

## Manual Setup (Windows)
1. `npm install`
2. `cd zoom-meeting-sdk-addon && npm install`
3. `node scripts/install-zoom-sdk.js "C:\path\to\zoom-sdk-x64"`
4. `cd zoom-meeting-sdk-addon && npx node-gyp rebuild`
5. `node scripts/fix-grandiose.js`
6. Create `.env` with ZOOM_SDK_KEY and ZOOM_SDK_SECRET
7. `npm start`

## Zoom SDK v6.7.5 Notes
- SDK lib: `sdk/lib/sdk.lib` (single file)
- SDK DLLs: 92+ DLLs in `sdk/bin/`
- `IAuthServiceEvent::onNotificationServiceStatus` takes two params (status, error)
- `IMeetingServiceEvent` requires: onMeetingTopicChanged, onMeetingFullToWatchLiveStream, onUserNetworkStatusChanged, onAppSignalPanelUpdated
- `IMeetingParticipantsCtrlEvent` requires: onUserNamesChanged (replaces onUserNameChanged), onBotAuthorizerRelationChanged, onVirtualNameTagStatusChanged, onVirtualNameTagRosterInfoUpdated, onCreateCompanionRelation, onRemoveCompanionRelation, onGrantCoOwnerPrivilegeChanged
- `IZoomSDKAudioRawDataDelegate::onShareAudioRawDataReceived` takes (AudioRawData*, uint32_t)
- `IZoomSDKAudioRawDataDelegate::onOneWayInterpreterAudioRawDataReceived` is new abstract method
- `GetYBuffer()/GetUBuffer()/GetVBuffer()` return `char*`, need reinterpret_cast to unsigned char*
- Audio helper: use `GetAudioRawdataHelper()` directly (not via GetRawdataAPIHelper)
- Include `meeting_audio_interface.h` before `meeting_participants_ctrl_interface.h` for AudioType
