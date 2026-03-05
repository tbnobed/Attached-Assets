{
  "targets": [
    {
      "target_name": "zoom_meeting_sdk",
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "sources": [
        "src/zoom_addon.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "conditions": [
        ["OS=='win'", {
          "defines": ["WIN32"],
          "sources": [
            "src/zoom_auth.cpp",
            "src/zoom_meeting.cpp",
            "src/zoom_rawdata.cpp"
          ],
          "include_dirs": [
            "<(module_root_dir)/sdk/h"
          ],
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
        }],
        ["OS=='mac'", {
          "sources": [
            "src/zoom_auth.cpp",
            "src/zoom_meeting.cpp",
            "src/zoom_rawdata.cpp"
          ],
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
            "GCC_INPUT_FILETYPE": "sourcecode.cpp.objcpp",
            "OTHER_CPLUSPLUSFLAGS": [
              "-x", "objective-c++",
              "-std=c++17",
              "-fobjc-arc",
              "-F<(module_root_dir)/sdk/lib"
            ],
            "OTHER_LDFLAGS": [
              "-F<(module_root_dir)/sdk/lib",
              "-framework ZoomSDK",
              "-framework Foundation",
              "-framework AppKit",
              "-Wl,-rpath,@loader_path",
              "-Wl,-rpath,@loader_path/../../sdk/lib",
              "-Wl,-rpath,@executable_path/../Frameworks"
            ],
            "FRAMEWORK_SEARCH_PATHS": [
              "<(module_root_dir)/sdk/lib"
            ]
          },
          "libraries": [
            "-F<(module_root_dir)/sdk/lib",
            "-framework ZoomSDK",
            "-framework Foundation",
            "-framework AppKit"
          ]
        }]
      ]
    }
  ]
}
