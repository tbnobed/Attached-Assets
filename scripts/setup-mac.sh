#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ZOOM_SDK_PATH="${1:-/Users/debo/Documents/zoom-sdk-macos-6.7.6.75900}"
PYTHON_PATH="${PYTHON:-/opt/homebrew/bin/python3.12}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo ""
echo "============================================"
echo "  PlexISO - Full macOS Setup"
echo "============================================"
echo ""
echo "Project dir:  $PROJECT_DIR"
echo "Zoom SDK:     $ZOOM_SDK_PATH"
echo "Python:       $PYTHON_PATH"
echo ""

if [ ! -d "$ZOOM_SDK_PATH" ]; then
    echo -e "${RED}ERROR: Zoom SDK not found at $ZOOM_SDK_PATH${NC}"
    echo "Usage: ./scripts/setup-mac.sh /path/to/zoom-sdk-macos-6.7.6.75900"
    exit 1
fi

if [ ! -f "$PYTHON_PATH" ]; then
    echo -e "${YELLOW}WARNING: Python not found at $PYTHON_PATH${NC}"
    echo "Trying to find a compatible python3..."
    for p in /opt/homebrew/bin/python3.12 /opt/homebrew/bin/python3.11 /opt/homebrew/bin/python3.13 /usr/bin/python3; do
        if [ -f "$p" ]; then
            PYTHON_PATH="$p"
            break
        fi
    done
    if [ ! -f "$PYTHON_PATH" ]; then
        echo -e "${RED}ERROR: No compatible python3 found. Install Python 3.12: brew install python@3.12${NC}"
        exit 1
    fi
    echo "Using: $PYTHON_PATH"
fi

export PYTHON="$PYTHON_PATH"
export NODE_TLS_REJECT_UNAUTHORIZED=0

echo "--------------------------------------------"
echo -e "${GREEN}[1/7]${NC} Installing Node dependencies..."
echo "--------------------------------------------"
cd "$PROJECT_DIR"
npm install --ignore-scripts
cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
npm install
cd "$PROJECT_DIR"

node -e "require('electron')" 2>/dev/null && echo "Electron installed OK" || echo "Running electron postinstall..." && cd "$PROJECT_DIR" && npx electron --version >/dev/null 2>&1 || true

echo ""

echo "--------------------------------------------"
echo -e "${GREEN}[2/7]${NC} Installing Zoom SDK..."
echo "--------------------------------------------"
node scripts/install-zoom-sdk.js "$ZOOM_SDK_PATH"
echo ""

echo "--------------------------------------------"
echo -e "${GREEN}[3/7]${NC} Symlinking frameworks into Electron..."
echo "--------------------------------------------"
npm run link-frameworks
echo ""

echo "--------------------------------------------"
echo -e "${GREEN}[4/7]${NC} Installing grandiose (NDI)..."
echo "--------------------------------------------"
GRANDIOSE_DIR="$PROJECT_DIR/node_modules/grandiose"
if [ -f "$GRANDIOSE_DIR/build/Release/grandiose.node" ]; then
    echo "grandiose already built, skipping."
else
    rm -rf "$GRANDIOSE_DIR"
    echo "Cloning grandiose..."
    git clone https://github.com/rse/grandiose.git "$GRANDIOSE_DIR"

    NDI_PKG="/tmp/ndi_sdk_plexiso.pkg"
    NDI_EXPANDED="/tmp/ndi_sdk_plexiso_expanded"
    NDI_SRC="$NDI_EXPANDED/NDI SDK for Apple"

    if [ ! -d "$NDI_SRC" ]; then
        echo "Downloading NDI SDK v6..."
        curl -k -L -o "$NDI_PKG" "https://downloads.ndi.tv/SDK/NDI_SDK_v6/Install_NDI_SDK_v6_Apple.pkg"
        rm -rf "$NDI_EXPANDED"
        pkgutil --expand "$NDI_PKG" "$NDI_EXPANDED"
        cd "$NDI_EXPANDED"
        cat "NDI SDK for Apple.pkg/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
        cd "$PROJECT_DIR"
    else
        echo "NDI SDK already extracted, reusing."
    fi

    mkdir -p "$GRANDIOSE_DIR/ndi/include"
    mkdir -p "$GRANDIOSE_DIR/ndi/lib/macOS"
    cp -R "$NDI_SRC/include/"* "$GRANDIOSE_DIR/ndi/include/"
    cp -R "$NDI_SRC/lib/macOS/"* "$GRANDIOSE_DIR/ndi/lib/macOS/"

    echo "Building grandiose..."
    cd "$GRANDIOSE_DIR"
    PYTHON="$PYTHON_PATH" npx node-gyp rebuild
    cd "$PROJECT_DIR"
fi
echo ""

echo "--------------------------------------------"
echo -e "${GREEN}[5/7]${NC} Building Zoom native addon..."
echo "--------------------------------------------"
cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
chmod +x build-tools/cxx-objcpp.sh
CXX="$(pwd)/build-tools/cxx-objcpp.sh" PYTHON="$PYTHON_PATH" npx node-gyp rebuild
cd "$PROJECT_DIR"
echo ""

echo "--------------------------------------------"
echo -e "${GREEN}[6/7]${NC} Checking .env file..."
echo "--------------------------------------------"
if [ -f "$PROJECT_DIR/.env" ]; then
    echo ".env file exists."
    if grep -q "your_sdk_key_here" "$PROJECT_DIR/.env" 2>/dev/null; then
        echo -e "${YELLOW}WARNING: .env still has placeholder values. Edit it with your real SDK credentials.${NC}"
    fi
else
    cat > "$PROJECT_DIR/.env" << 'ENVEOF'
ZOOM_SDK_KEY=your_sdk_key_here
ZOOM_SDK_SECRET=your_sdk_secret_here
ZOOM_MEETING_BOT_NAME=PlexISO
ENVEOF
    echo -e "${YELLOW}Created .env with placeholders. Edit it with your real Zoom SDK credentials before running.${NC}"
fi
echo ""

echo "--------------------------------------------"
echo -e "${GREEN}[7/7]${NC} Verifying build..."
echo "--------------------------------------------"
ADDON_PATH="$PROJECT_DIR/zoom-meeting-sdk-addon/build/Release/zoom_meeting_sdk.node"
GRANDIOSE_PATH="$PROJECT_DIR/node_modules/grandiose/build/Release/grandiose.node"

PASS=true
if [ -f "$ADDON_PATH" ]; then
    echo -e "${GREEN}  ✓ Zoom addon built${NC}"
else
    echo -e "${RED}  ✗ Zoom addon NOT found${NC}"
    PASS=false
fi

if [ -f "$GRANDIOSE_PATH" ]; then
    echo -e "${GREEN}  ✓ Grandiose (NDI) built${NC}"
else
    echo -e "${RED}  ✗ Grandiose NOT found${NC}"
    PASS=false
fi

ELECTRON_PATH="$PROJECT_DIR/node_modules/electron/dist/Electron.app"
if [ -d "$ELECTRON_PATH" ]; then
    echo -e "${GREEN}  ✓ Electron.app present${NC}"
else
    echo -e "${RED}  ✗ Electron.app NOT found${NC}"
    PASS=false
fi

FRAMEWORK_LINK="$ELECTRON_PATH/Contents/Frameworks/ZoomSDK.framework"
if [ -L "$FRAMEWORK_LINK" ] || [ -d "$FRAMEWORK_LINK" ]; then
    echo -e "${GREEN}  ✓ ZoomSDK framework linked${NC}"
else
    echo -e "${RED}  ✗ ZoomSDK framework NOT linked in Electron${NC}"
    PASS=false
fi

echo ""
if [ "$PASS" = true ]; then
    echo "============================================"
    echo -e "${GREEN}  Setup complete! All checks passed.${NC}"
    echo "============================================"
    echo ""
    echo "To run:"
    echo "  cd $PROJECT_DIR && npm start"
else
    echo "============================================"
    echo -e "${YELLOW}  Setup finished with warnings above.${NC}"
    echo "============================================"
fi
echo ""
