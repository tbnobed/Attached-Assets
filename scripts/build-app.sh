#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

APP_NAME="ZoomLink"
ELECTRON_APP="$PROJECT_DIR/node_modules/electron/dist/Electron.app"
OUTPUT_APP="$PROJECT_DIR/dist/$APP_NAME.app"

echo ""
echo "============================================"
echo "  Building $APP_NAME.app"
echo "============================================"
echo ""

if [ ! -d "$ELECTRON_APP" ]; then
    echo -e "${RED}ERROR: Electron.app not found. Run setup-mac.sh first.${NC}"
    exit 1
fi

ADDON_NODE="$PROJECT_DIR/zoom-meeting-sdk-addon/build/Release/zoom_meeting_sdk.node"
if [ ! -f "$ADDON_NODE" ]; then
    echo -e "${RED}ERROR: Zoom addon not built. Run setup-mac.sh first.${NC}"
    exit 1
fi

echo "[1/6] Creating app bundle..."
rm -rf "$OUTPUT_APP"
mkdir -p "$PROJECT_DIR/dist"
cp -R "$ELECTRON_APP" "$OUTPUT_APP"

CONTENTS="$OUTPUT_APP/Contents"
RESOURCES="$CONTENTS/Resources"
APP_CODE="$RESOURCES/app"

mv "$CONTENTS/MacOS/Electron" "$CONTENTS/MacOS/$APP_NAME" 2>/dev/null || true

echo "[2/6] Creating icon..."
ICNS_FILE="$PROJECT_DIR/build/icon.icns"
if [ ! -f "$ICNS_FILE" ]; then
    if command -v sips &>/dev/null && command -v iconutil &>/dev/null; then
        bash "$SCRIPT_DIR/create-icns.sh"
    fi
fi

ICON_NAME="electron"
if [ -f "$ICNS_FILE" ]; then
    cp "$ICNS_FILE" "$RESOURCES/ZoomLink.icns"
    rm -f "$RESOURCES/electron.icns" 2>/dev/null
    ICON_NAME="ZoomLink"
    echo -e "${GREEN}  Custom icon set${NC}"
else
    echo -e "${YELLOW}  Using default Electron icon (run create-icns.sh on macOS to create custom icon)${NC}"
fi

echo "[3/6] Writing Info.plist..."
cat > "$CONTENTS/Info.plist" << PLISTEOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>ZoomLink</string>
    <key>CFBundleDisplayName</key>
    <string>ZoomLink</string>
    <key>CFBundleIdentifier</key>
    <string>com.obtv.zoomlink</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleExecutable</key>
    <string>ZoomLink</string>
    <key>CFBundleIconFile</key>
    <string>${ICON_NAME}.icns</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>NSCameraUsageDescription</key>
    <string>ZoomLink needs camera access for Zoom meetings</string>
    <key>NSMicrophoneUsageDescription</key>
    <string>ZoomLink needs microphone access for Zoom meetings</string>
</dict>
</plist>
PLISTEOF

echo "[4/6] Copying application code..."
mkdir -p "$APP_CODE"

cp "$PROJECT_DIR/package.json" "$APP_CODE/"

for dir in src scripts build; do
    if [ -d "$PROJECT_DIR/$dir" ]; then
        cp -R "$PROJECT_DIR/$dir" "$APP_CODE/"
    fi
done

mkdir -p "$APP_CODE/node_modules"
for dep in electron grandiose jsonwebtoken; do
    if [ -d "$PROJECT_DIR/node_modules/$dep" ]; then
        cp -R "$PROJECT_DIR/node_modules/$dep" "$APP_CODE/node_modules/"
    fi
done

for dep in $(ls "$PROJECT_DIR/node_modules/" 2>/dev/null); do
    [ -d "$PROJECT_DIR/node_modules/$dep" ] || continue
    [ "$dep" = "electron" ] && continue
    [ "$dep" = ".package-lock.json" ] && continue
    [ -d "$APP_CODE/node_modules/$dep" ] && continue
    cp -R "$PROJECT_DIR/node_modules/$dep" "$APP_CODE/node_modules/" 2>/dev/null || true
done

if [ -d "$PROJECT_DIR/zoom-meeting-sdk-addon" ]; then
    cp -R "$PROJECT_DIR/zoom-meeting-sdk-addon" "$APP_CODE/"
fi

if [ -d "$PROJECT_DIR/decklink-output-addon" ]; then
    cp -R "$PROJECT_DIR/decklink-output-addon" "$APP_CODE/"
fi

echo "[5/6] Creating launcher wrapper..."
LAUNCHER="$CONTENTS/MacOS/$APP_NAME"
REAL_ELECTRON="$CONTENTS/MacOS/${APP_NAME}_electron"

if [ -f "$LAUNCHER" ]; then
    mv "$LAUNCHER" "$REAL_ELECTRON"
fi

cat > "$LAUNCHER" << 'LAUNCHEOF'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
CONTENTS="$(cd "$DIR/.." && pwd)"
RESOURCES="$CONTENTS/Resources"
APP_DIR="$RESOURCES/app"
FRAMEWORKS="$CONTENTS/Frameworks"

export DYLD_LIBRARY_PATH="$APP_DIR/node_modules/grandiose/build/Release:$DYLD_LIBRARY_PATH"
export DYLD_FRAMEWORK_PATH="$FRAMEWORKS:$DYLD_FRAMEWORK_PATH"

for p in /opt/homebrew/bin /usr/local/bin /opt/homebrew/sbin /usr/local/sbin; do
    [ -d "$p" ] && export PATH="$p:$PATH"
done

exec "$DIR/ZoomLink_electron" "$APP_DIR" --no-sandbox --disable-gpu-sandbox "$@"
LAUNCHEOF
chmod +x "$LAUNCHER"

echo "[6/6] Verifying..."
CHECKS_OK=true

check_file() {
    if [ -e "$1" ]; then
        echo -e "${GREEN}  ✓ $2${NC}"
    else
        echo -e "${RED}  ✗ $2 ($1)${NC}"
        CHECKS_OK=false
    fi
}

check_file "$CONTENTS/MacOS/$APP_NAME" "Launcher script"
check_file "$REAL_ELECTRON" "Electron binary"
check_file "$APP_CODE/package.json" "package.json"
check_file "$APP_CODE/src/main/main.js" "Main process"
check_file "$APP_CODE/src/renderer/index.html" "Renderer"
check_file "$APP_CODE/zoom-meeting-sdk-addon/build/Release/zoom_meeting_sdk.node" "Zoom addon"
check_file "$CONTENTS/Info.plist" "Info.plist"

GRANDIOSE_NODE="$APP_CODE/node_modules/grandiose/build/Release/grandiose.node"
if [ -f "$GRANDIOSE_NODE" ]; then
    echo -e "${GREEN}  ✓ Grandiose (NDI)${NC}"
else
    echo -e "${YELLOW}  ~ Grandiose (NDI) — not found${NC}"
fi

DECKLINK_NODE="$APP_CODE/decklink-output-addon/build/Release/decklink_output.node"
if [ -f "$DECKLINK_NODE" ]; then
    echo -e "${GREEN}  ✓ DeckLink addon${NC}"
else
    echo -e "${YELLOW}  ~ DeckLink addon — not built (optional)${NC}"
fi

echo ""
if [ "$CHECKS_OK" = true ]; then
    APP_SIZE=$(du -sh "$OUTPUT_APP" | cut -f1)
    echo "============================================"
    echo -e "${GREEN}  Build complete! ($APP_SIZE)${NC}"
    echo "============================================"
    echo ""
    echo "  App location:"
    echo "    $OUTPUT_APP"
    echo ""
    echo "  To install, drag ZoomLink.app to /Applications"
    echo "  or double-click it from the dist/ folder."
    echo ""
    echo -e "${YELLOW}  First launch: Right-click → Open (bypasses Gatekeeper)${NC}"
    echo ""
else
    echo "============================================"
    echo -e "${RED}  Build had errors — see above${NC}"
    echo "============================================"
    exit 1
fi
