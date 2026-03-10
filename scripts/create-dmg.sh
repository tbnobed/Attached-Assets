#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

APP_NAME="ZoomLink"
APP_PATH="$PROJECT_DIR/dist/$APP_NAME.app"
DMG_NAME="$APP_NAME-Installer"
DMG_PATH="$PROJECT_DIR/dist/$DMG_NAME.dmg"
DMG_TEMP="$PROJECT_DIR/dist/dmg-staging"
VOLUME_NAME="$APP_NAME"

echo ""
echo "============================================"
echo "  Creating $APP_NAME.dmg"
echo "============================================"
echo ""

if [ ! -d "$APP_PATH" ]; then
    echo -e "${YELLOW}  ZoomLink.app not found, building it first...${NC}"
    bash "$SCRIPT_DIR/build-app.sh" || {
        echo -e "${RED}ERROR: Failed to build ZoomLink.app${NC}"
        exit 1
    }
fi

if [ ! -d "$APP_PATH" ]; then
    echo -e "${RED}ERROR: $APP_PATH not found${NC}"
    exit 1
fi

echo "[1/4] Preparing staging area..."
rm -rf "$DMG_TEMP"
rm -f "$DMG_PATH"
mkdir -p "$DMG_TEMP"

cp -R "$APP_PATH" "$DMG_TEMP/"

ln -s /Applications "$DMG_TEMP/Applications"

if [ -f "$PROJECT_DIR/build/icon.png" ]; then
    cp "$PROJECT_DIR/build/icon.png" "$DMG_TEMP/.background_icon.png"
fi

echo "[2/4] Creating DMG image..."
APP_SIZE=$(du -sm "$DMG_TEMP" | cut -f1)
DMG_SIZE=$((APP_SIZE + 20))

hdiutil create \
    -volname "$VOLUME_NAME" \
    -srcfolder "$DMG_TEMP" \
    -ov \
    -format UDRW \
    -size "${DMG_SIZE}m" \
    "${DMG_PATH}.rw" \
    -quiet

echo "[3/4] Customizing DMG layout..."
MOUNT_DIR="/Volumes/$VOLUME_NAME"

if mount | grep -q "$MOUNT_DIR"; then
    hdiutil detach "$MOUNT_DIR" -quiet -force 2>/dev/null
    sleep 1
fi

hdiutil attach "${DMG_PATH}.rw" -noautoopen -quiet

sleep 1

if [ -d "$MOUNT_DIR" ]; then
    osascript <<APPLESCRIPT 2>/dev/null
tell application "Finder"
    tell disk "$VOLUME_NAME"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set bounds of container window to {100, 100, 640, 400}
        set theViewOptions to icon view options of container window
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 96
        set position of item "$APP_NAME.app" of container window to {140, 150}
        set position of item "Applications" of container window to {400, 150}
        close
        open
        update without registering applications
        delay 1
    end tell
end tell
APPLESCRIPT
    sync
    sleep 2
fi

hdiutil detach "$MOUNT_DIR" -quiet -force 2>/dev/null
sleep 1

echo "[4/4] Compressing final DMG..."
hdiutil convert "${DMG_PATH}.rw" \
    -format UDZO \
    -imagekey zlib-level=9 \
    -o "$DMG_PATH" \
    -quiet

rm -f "${DMG_PATH}.rw"
rm -rf "$DMG_TEMP"

if [ -f "$DMG_PATH" ]; then
    DMG_FILE_SIZE=$(du -sh "$DMG_PATH" | cut -f1)
    echo ""
    echo "============================================"
    echo -e "${GREEN}  DMG created successfully! ($DMG_FILE_SIZE)${NC}"
    echo "============================================"
    echo ""
    echo "  File: $DMG_PATH"
    echo ""
    echo "  To install:"
    echo "    1. Double-click the .dmg file"
    echo "    2. Drag ZoomLink to the Applications folder"
    echo "    3. Eject the disk image"
    echo "    4. Launch ZoomLink from Applications"
    echo ""
    echo -e "${YELLOW}  First launch: Right-click > Open (bypasses Gatekeeper)${NC}"
    echo ""
else
    echo -e "${RED}ERROR: DMG creation failed${NC}"
    exit 1
fi
