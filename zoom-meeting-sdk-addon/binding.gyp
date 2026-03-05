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
          "defines": ["WIN32"],
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
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
            "OTHER_CPLUSPLUSFLAGS": ["-std=c++17"],
            "OTHER_LDFLAGS": [
              "-Wl,-rpath,@loader_path",
              "-Wl,-rpath,@executable_path/../Frameworks"
            ]
          },
          "libraries": [
            "<(module_root_dir)/sdk/lib/libmeetingsdk.dylib"
          ],
          "copies": [
            {
              "destination": "<(module_root_dir)/build/Release",
              "files": [
                "<(module_root_dir)/sdk/lib/libmeetingsdk.dylib"
              ]
            }
          ],
          "postbuilds": [
            {
              "postbuild_name": "Fix dylib install_name",
              "action": [
                "install_name_tool",
                "-change",
                "/usr/local/lib/libmeetingsdk.dylib",
                "@rpath/libmeetingsdk.dylib",
                "${BUILT_PRODUCTS_DIR}/zoom_meeting_sdk.node"
              ]
            }
          ]
        }]
      ]
    }
  ]
}
