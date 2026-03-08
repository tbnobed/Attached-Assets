{
  "targets": [
    {
      "target_name": "decklink_output",
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "sources": [
        "src/decklink_output.mm",
        "/Library/Frameworks/DeckLinkAPI.framework/Headers/DeckLinkAPIDispatch.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "conditions": [
        ["OS=='mac'", {
          "include_dirs": [
            "/Library/Frameworks/DeckLinkAPI.framework/Headers"
          ],
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
            "GCC_INPUT_FILETYPE": "sourcecode.cpp.objcpp",
            "OTHER_CPLUSPLUSFLAGS": [
              "-x", "objective-c++",
              "-std=c++17",
              "-fobjc-arc"
            ],
            "OTHER_LDFLAGS": [
              "-framework", "CoreFoundation"
            ]
          },
          "libraries": [
            "-framework CoreFoundation"
          ]
        }]
      ]
    }
  ]
}
