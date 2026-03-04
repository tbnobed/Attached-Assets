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
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS", "WIN32"],
      "conditions": [
        ["OS=='win'", {
          "libraries": [
            "<(module_root_dir)/sdk/lib/sdk.lib"
          ],
          "copies": [
            {
              "destination": "<(module_root_dir)/build/Release",
              "files": [
                "<(module_root_dir)/sdk/bin/sdk.dll"
              ]
            }
          ]
        }]
      ]
    }
  ]
}
