{
  "targets": [
    {
      "target_name": "decklink_output",
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "cflags_cc": ["-std=c++17", "-fexceptions"],
      "sources": [
        "src/decklink_output.cpp",
        "src/DeckLinkAPIDispatch.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "/usr/include",
        "/usr/local/include"
      ],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "conditions": [
        ["OS=='linux'", {
          "libraries": [
            "-ldl",
            "-lpthread"
          ]
        }]
      ]
    }
  ]
}
