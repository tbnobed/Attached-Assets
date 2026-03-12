#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

APP_NAME="ZoomLink"
ELECTRON_APP="$SCRIPT_DIR/node_modules/electron/dist/Electron.app"
OUTPUT_APP="$SCRIPT_DIR/dist/$APP_NAME.app"

echo ""
echo "============================================"
echo "  Building $APP_NAME.app (Remote SDI)"
echo "============================================"
echo ""

if [ ! -d "$ELECTRON_APP" ]; then
    echo "  Electron.app not found in mac-app/node_modules, checking root..."
    ELECTRON_APP="$PROJECT_DIR/node_modules/electron/dist/Electron.app"
fi

if [ ! -d "$ELECTRON_APP" ]; then
    echo -e "${RED}ERROR: Electron.app not found. Run setup.sh first.${NC}"
    exit 1
fi

echo "[1/6] Creating app bundle..."
rm -rf "$OUTPUT_APP"
mkdir -p "$SCRIPT_DIR/dist"
cp -R "$ELECTRON_APP" "$OUTPUT_APP"

CONTENTS="$OUTPUT_APP/Contents"
RESOURCES="$CONTENTS/Resources"
APP_CODE="$RESOURCES/app"

mv "$CONTENTS/MacOS/Electron" "$CONTENTS/MacOS/$APP_NAME" 2>/dev/null || true

echo "[2/6] Creating icon..."
ICNS_FILE="$PROJECT_DIR/build/icon.icns"
ICON_NAME="electron"
if [ -f "$ICNS_FILE" ]; then
    cp "$ICNS_FILE" "$RESOURCES/ZoomLink.icns"
    rm -f "$RESOURCES/electron.icns" 2>/dev/null
    ICON_NAME="ZoomLink"
    echo -e "${GREEN}  Custom icon set${NC}"
elif [ -f "$SCRIPT_DIR/build/icon.icns" ]; then
    cp "$SCRIPT_DIR/build/icon.icns" "$RESOURCES/ZoomLink.icns"
    rm -f "$RESOURCES/electron.icns" 2>/dev/null
    ICON_NAME="ZoomLink"
    echo -e "${GREEN}  Custom icon set${NC}"
else
    echo -e "${YELLOW}  Using default Electron icon${NC}"
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
    <string>ZoomLink Remote SDI</string>
    <key>CFBundleIdentifier</key>
    <string>com.obtv.zoomlink-remote</string>
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

cat > "$APP_CODE/package.json" << 'PKGEOF'
{
  "name": "zoomlink-remote-sdi",
  "version": "1.0.0",
  "main": "mac-app/main.js",
  "private": true
}
PKGEOF

mkdir -p "$APP_CODE/mac-app"
cp "$SCRIPT_DIR/main.js" "$APP_CODE/mac-app/"
cp "$SCRIPT_DIR/preload.js" "$APP_CODE/mac-app/"
cp "$SCRIPT_DIR/ipc-handlers.js" "$APP_CODE/mac-app/"
cp -R "$SCRIPT_DIR/renderer" "$APP_CODE/mac-app/"
cp -R "$SCRIPT_DIR/network" "$APP_CODE/mac-app/"

mkdir -p "$APP_CODE/src"
if [ -d "$PROJECT_DIR/src" ]; then
    cp -R "$PROJECT_DIR/src/config" "$APP_CODE/src/" 2>/dev/null || true
    cp -R "$PROJECT_DIR/src/zoom" "$APP_CODE/src/" 2>/dev/null || true
    echo -e "  ${GREEN}Copied shared src/ modules${NC}"
else
    echo -e "${RED}ERROR: $PROJECT_DIR/src/ not found — required for shared modules${NC}"
    exit 1
fi

mkdir -p "$APP_CODE/build"
if [ -f "$PROJECT_DIR/build/icon.png" ]; then
    cp "$PROJECT_DIR/build/icon.png" "$APP_CODE/build/"
    echo -e "  ${GREEN}Copied app icon (icon.png)${NC}"
elif [ -f "$SCRIPT_DIR/build/icon.png" ]; then
    cp "$SCRIPT_DIR/build/icon.png" "$APP_CODE/build/"
    echo -e "  ${GREEN}Copied app icon (icon.png)${NC}"
else
    echo -e "${YELLOW}  No icon.png found — app will use default icon${NC}"
fi

mkdir -p "$APP_CODE/node_modules"
MODULES_SRC="$SCRIPT_DIR/node_modules"
[ ! -d "$MODULES_SRC" ] && MODULES_SRC="$PROJECT_DIR/node_modules"

for dep in $(ls "$MODULES_SRC/" 2>/dev/null); do
    [ "$dep" = "electron" ] && continue
    [ "$dep" = ".package-lock.json" ] && continue
    if [ -d "$MODULES_SRC/$dep" ]; then
        cp -R "$MODULES_SRC/$dep" "$APP_CODE/node_modules/" 2>/dev/null || true
    fi
done

if [ -d "$PROJECT_DIR/zoom-meeting-sdk-addon" ]; then
    cp -R "$PROJECT_DIR/zoom-meeting-sdk-addon" "$APP_CODE/"
    echo -e "  ${GREEN}Included Zoom SDK addon${NC}"

    SDK_LIB="$APP_CODE/zoom-meeting-sdk-addon/sdk/lib"
    if [ -d "$SDK_LIB" ]; then
        BUNDLE_FRAMEWORKS="$CONTENTS/Frameworks"
        mkdir -p "$BUNDLE_FRAMEWORKS"
        for item in "$SDK_LIB"/*; do
            bname="$(basename "$item")"
            rm -rf "$BUNDLE_FRAMEWORKS/$bname" 2>/dev/null
            cp -R "$item" "$BUNDLE_FRAMEWORKS/$bname" 2>/dev/null || true
        done
        echo -e "  ${GREEN}Copied Zoom SDK frameworks into app bundle${NC}"
    fi
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
check_file "$APP_CODE/mac-app/main.js" "Main process"
check_file "$APP_CODE/mac-app/renderer/index.html" "Renderer"
check_file "$APP_CODE/src/config/settings.js" "Shared config"
check_file "$APP_CODE/src/zoom/session-manager.js" "Session manager"
check_file "$CONTENTS/Info.plist" "Info.plist"

if [ -d "$APP_CODE/zoom-meeting-sdk-addon/build/Release" ]; then
    echo -e "${GREEN}  ✓ Zoom SDK addon${NC}"
else
    echo -e "${YELLOW}  ~ Zoom SDK addon (not built — stub mode)${NC}"
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
