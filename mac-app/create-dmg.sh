#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

APP_NAME="ZoomLink"
APP_PATH="$SCRIPT_DIR/dist/$APP_NAME.app"
DMG_NAME="$APP_NAME-Installer"
DMG_PATH="$SCRIPT_DIR/dist/$DMG_NAME.dmg"
DMG_TEMP="$SCRIPT_DIR/dist/dmg-staging"
VOLUME_NAME="Install $APP_NAME"

echo ""
echo "============================================"
echo "  Creating $APP_NAME.dmg (Remote SDI)"
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

echo "[1/3] Preparing staging area..."
rm -rf "$DMG_TEMP"
rm -f "$DMG_PATH"
mkdir -p "$DMG_TEMP"

cp -R "$APP_PATH" "$DMG_TEMP/"
ln -s /Applications "$DMG_TEMP/Applications"

echo "[2/3] Creating DMG image..."

MOUNT_DIR="/Volumes/$VOLUME_NAME"
if [ -d "$MOUNT_DIR" ]; then
    echo "  Detaching existing volume..."
    hdiutil detach "$MOUNT_DIR" -force 2>/dev/null || true
    sleep 1
fi

if ! hdiutil create \
    -volname "$VOLUME_NAME" \
    -srcfolder "$DMG_TEMP" \
    -ov \
    -format UDZO \
    -imagekey zlib-level=9 \
    "$DMG_PATH"; then
    echo -e "${RED}ERROR: hdiutil create failed${NC}"
    rm -rf "$DMG_TEMP"
    exit 1
fi

echo "[3/3] Cleaning up..."
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
