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
    echo "Trying to find python3..."
    PYTHON_PATH="$(which python3 2>/dev/null || true)"
    if [ -z "$PYTHON_PATH" ]; then
        echo -e "${RED}ERROR: No python3 found. Install Python 3 first.${NC}"
        exit 1
    fi
    echo "Using: $PYTHON_PATH"
fi

echo "--------------------------------------------"
echo -e "${GREEN}[1/7]${NC} Installing Node dependencies..."
echo "--------------------------------------------"
cd "$PROJECT_DIR"
npm install
cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
npm install
cd "$PROJECT_DIR"
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
if [ -d "$GRANDIOSE_DIR/build/Release" ] && [ -f "$GRANDIOSE_DIR/build/Release/grandiose.node" ]; then
    echo "grandiose already built, skipping."
else
    rm -rf "$GRANDIOSE_DIR"
    git clone https://github.com/rse/grandiose.git "$GRANDIOSE_DIR"

    NDI_PKG="/tmp/ndi_sdk_plexiso.pkg"
    NDI_EXPANDED="/tmp/ndi_sdk_plexiso_expanded"

    if [ ! -d "$NDI_EXPANDED/NDI SDK for Apple" ]; then
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

    NDI_SRC="$NDI_EXPANDED/NDI SDK for Apple"
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
CXX="$(pwd)/build-tools/cxx-objcpp.sh" NODE_TLS_REJECT_UNAUTHORIZED=0 PYTHON="$PYTHON_PATH" npx node-gyp rebuild
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

if [ -f "$ADDON_PATH" ]; then
    echo -e "${GREEN}  ✓ Zoom addon built successfully${NC}"
else
    echo -e "${RED}  ✗ Zoom addon NOT found at $ADDON_PATH${NC}"
fi

if [ -f "$GRANDIOSE_PATH" ]; then
    echo -e "${GREEN}  ✓ Grandiose (NDI) built successfully${NC}"
else
    echo -e "${RED}  ✗ Grandiose NOT found at $GRANDIOSE_PATH${NC}"
fi

echo ""
echo "============================================"
echo -e "${GREEN}  Setup complete!${NC}"
echo "============================================"
echo ""
echo "To run:"
echo "  cd $PROJECT_DIR && npm start"
echo ""
