#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT="$PROJECT_DIR/build/icon.png"
OUTPUT="$PROJECT_DIR/build/icon.icns"
ICONSET="$PROJECT_DIR/build/ZoomLink.iconset"

if [ ! -f "$INPUT" ]; then
    echo "ERROR: $INPUT not found"
    exit 1
fi

if [ -f "$OUTPUT" ] && [ "$OUTPUT" -nt "$INPUT" ]; then
    echo "icon.icns is up to date at $OUTPUT"
    exit 0
fi

rm -f "$OUTPUT"

rm -rf "$ICONSET"
mkdir -p "$ICONSET"

sips -z 16 16     "$INPUT" --out "$ICONSET/icon_16x16.png"      >/dev/null 2>&1
sips -z 32 32     "$INPUT" --out "$ICONSET/icon_16x16@2x.png"   >/dev/null 2>&1
sips -z 32 32     "$INPUT" --out "$ICONSET/icon_32x32.png"      >/dev/null 2>&1
sips -z 64 64     "$INPUT" --out "$ICONSET/icon_32x32@2x.png"   >/dev/null 2>&1
sips -z 128 128   "$INPUT" --out "$ICONSET/icon_128x128.png"    >/dev/null 2>&1
sips -z 256 256   "$INPUT" --out "$ICONSET/icon_128x128@2x.png" >/dev/null 2>&1
sips -z 256 256   "$INPUT" --out "$ICONSET/icon_256x256.png"    >/dev/null 2>&1
sips -z 512 512   "$INPUT" --out "$ICONSET/icon_256x256@2x.png" >/dev/null 2>&1
sips -z 512 512   "$INPUT" --out "$ICONSET/icon_512x512.png"    >/dev/null 2>&1
sips -z 1024 1024 "$INPUT" --out "$ICONSET/icon_512x512@2x.png" >/dev/null 2>&1

iconutil -c icns "$ICONSET" -o "$OUTPUT"
rm -rf "$ICONSET"

if [ -f "$OUTPUT" ]; then
    echo "Created $OUTPUT"
else
    echo "ERROR: Failed to create icns"
    exit 1
fi
