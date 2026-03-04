{
  "targets": [
    {
      "target_name": "zoom_meeting_sdk",
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "sources": [
        "src/zoom_addon.cpp",
        "src/zoom_auth.cpp",
        "src/zoom_meeting.cpp",
        "src/zoom_rawdata.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/sdk/h"
      ],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "conditions": [
        ["OS=='win'", {
          "libraries": [
            "<(module_root_dir)/sdk/lib/x64/libSDKBase.lib"
          ],
          "copies": [
            {
              "destination": "<(module_root_dir)/build/Release",
              "files": [
                "<(module_root_dir)/sdk/bin/x64/libSDKBase.dll",
                "<(module_root_dir)/sdk/bin/x64/sdk.dll",
                "<(module_root_dir)/sdk/bin/x64/aomhost.exe",
                "<(module_root_dir)/sdk/bin/x64/zVideoApp.dll",
                "<(module_root_dir)/sdk/bin/x64/zAudioApp.dll"
              ]
            }
          ]
        }]
      ]
    }
  ]
}
