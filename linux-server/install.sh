#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo ""
echo "============================================"
echo "  ZoomLink SDI Server — Linux Setup"
echo "  DeckLink + NDI + Recording"
echo "============================================"
echo ""

if [ "$(uname)" != "Linux" ]; then
    echo -e "${RED}ERROR: This script is intended for Linux only${NC}"
    exit 1
fi

echo -e "${CYAN}[1/7] Checking system dependencies...${NC}"

if ! command -v node &>/dev/null; then
    echo -e "${YELLOW}  Node.js not found. Installing via NodeSource...${NC}"
    if command -v apt-get &>/dev/null; then
        curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
        sudo apt-get install -y nodejs
    elif command -v dnf &>/dev/null; then
        curl -fsSL https://rpm.nodesource.com/setup_20.x | sudo bash -
        sudo dnf install -y nodejs
    else
        echo -e "${RED}ERROR: Unsupported package manager. Install Node.js 18+ manually.${NC}"
        exit 1
    fi
fi

NODE_VER=$(node -v | sed 's/v//' | cut -d. -f1)
if [ "$NODE_VER" -lt 18 ]; then
    echo -e "${RED}ERROR: Node.js 18+ required (found v$(node -v))${NC}"
    exit 1
fi
echo -e "  Node.js: $(node -v)"

if ! command -v npm &>/dev/null; then
    echo -e "${RED}ERROR: npm not found${NC}"
    exit 1
fi
echo -e "  npm: $(npm -v)"

if command -v apt-get &>/dev/null; then
    echo "  Installing build tools..."
    sudo apt-get install -y build-essential python3 pkg-config 2>/dev/null || true
fi

echo ""
echo -e "${CYAN}[2/7] Checking FFmpeg (for recording)...${NC}"

if command -v ffmpeg &>/dev/null; then
    FFMPEG_VER=$(ffmpeg -version 2>/dev/null | head -1 | awk '{print $3}')
    echo -e "  FFmpeg found: $FFMPEG_VER"
else
    echo -e "${YELLOW}  FFmpeg not found. Installing...${NC}"
    if command -v apt-get &>/dev/null; then
        sudo apt-get install -y ffmpeg
    elif command -v dnf &>/dev/null; then
        sudo dnf install -y ffmpeg
    elif command -v pacman &>/dev/null; then
        sudo pacman -S --noconfirm ffmpeg
    else
        echo -e "${YELLOW}  Could not install FFmpeg automatically.${NC}"
        echo "  Please install FFmpeg manually for recording support."
        echo "  The server will run without recording if FFmpeg is missing."
    fi

    if command -v ffmpeg &>/dev/null; then
        echo -e "  FFmpeg installed: $(ffmpeg -version 2>/dev/null | head -1 | awk '{print $3}')"
    else
        echo -e "${YELLOW}  FFmpeg not available — recording will be disabled${NC}"
    fi
fi

echo ""
echo -e "${CYAN}[3/7] Checking NDI SDK...${NC}"

NDI_FOUND=0
NDI_LIB_PATHS=(
    "/usr/lib/libndi.so"
    "/usr/local/lib/libndi.so"
    "/usr/lib/x86_64-linux-gnu/libndi.so"
    "/opt/ndi/lib/libndi.so"
)

for p in "${NDI_LIB_PATHS[@]}"; do
    if [ -f "$p" ]; then
        NDI_FOUND=1
        echo -e "  NDI runtime library found: $p"
        break
    fi
done

if [ "$NDI_FOUND" = "0" ]; then
    echo -e "${YELLOW}  NDI runtime library not found.${NC}"
    echo ""
    echo "  To enable NDI output, install the NDI SDK:"
    echo "    1. Download from: https://ndi.video/tools/download/"
    echo "    2. Run the installer (e.g., Install_NDI_SDK_v6_Linux.sh)"
    echo "    3. Copy libndi.so to /usr/lib/ or /usr/local/lib/"
    echo "    4. Run: sudo ldconfig"
    echo "    5. Re-run this script"
    echo ""
    echo -e "${YELLOW}  Continuing without NDI support (grandiose will use mock mode)...${NC}"
fi

echo ""
echo -e "${CYAN}[4/7] Installing npm dependencies...${NC}"
cd "$SCRIPT_DIR"
npm install 2>&1 || {
    echo -e "${YELLOW}  npm install had warnings (grandiose may fail without NDI SDK — this is OK)${NC}"
    echo "  The server will run with mock NDI if grandiose can't load."
}

echo ""
echo -e "${CYAN}[5/7] Checking DeckLink SDK...${NC}"

DECKLINK_HEADER=""
SEARCH_PATHS=(
    "/usr/include/DeckLinkAPI.h"
    "/usr/local/include/DeckLinkAPI.h"
    "/opt/decklink/include/DeckLinkAPI.h"
)

for p in "${SEARCH_PATHS[@]}"; do
    if [ -f "$p" ]; then
        DECKLINK_HEADER="$p"
        break
    fi
done

if [ -z "$DECKLINK_HEADER" ]; then
    echo -e "${YELLOW}  DeckLink SDK headers not found in standard locations.${NC}"
    echo ""
    echo "  To install DeckLink support:"
    echo "    1. Download Blackmagic Desktop Video from:"
    echo "       https://www.blackmagicdesign.com/support/download/deb"
    echo "    2. Install: sudo dpkg -i desktopvideo_*.deb"
    echo "    3. The SDK headers will be installed to /usr/include/"
    echo "    4. Re-run this script to build the DeckLink addon"
    echo ""
    echo -e "${YELLOW}  Continuing without DeckLink support...${NC}"
    SKIP_DECKLINK=1
else
    echo -e "  DeckLink SDK found: $DECKLINK_HEADER"
    SKIP_DECKLINK=0
fi

echo ""
echo -e "${CYAN}[6/7] Building DeckLink addon...${NC}"

if [ "$SKIP_DECKLINK" = "1" ]; then
    echo -e "${YELLOW}  Skipped (DeckLink SDK not found)${NC}"
else
    DISPATCH_CPP="$SCRIPT_DIR/decklink-addon/src/DeckLinkAPIDispatch.cpp"
    if [ ! -f "$DISPATCH_CPP" ]; then
        DISPATCH_SRC=""
        for d in /usr/include /usr/local/include /opt/decklink/include; do
            if [ -f "$d/DeckLinkAPIDispatch.cpp" ]; then
                DISPATCH_SRC="$d/DeckLinkAPIDispatch.cpp"
                break
            fi
        done

        if [ -n "$DISPATCH_SRC" ]; then
            cp "$DISPATCH_SRC" "$DISPATCH_CPP"
            echo "  Copied DeckLinkAPIDispatch.cpp from $DISPATCH_SRC"
        else
            echo -e "${YELLOW}  DeckLinkAPIDispatch.cpp not found — addon may not build${NC}"
        fi
    fi

    cd "$SCRIPT_DIR/decklink-addon"
    npm install
    npx node-gyp rebuild || {
        echo -e "${YELLOW}  DeckLink addon build failed (non-fatal)${NC}"
        echo "  The server will run without DeckLink output."
    }
    cd "$SCRIPT_DIR"
fi

echo ""
echo -e "${CYAN}[7/7] Setting up systemd service...${NC}"

SERVICE_FILE="$SCRIPT_DIR/zoomlink-sdi.service"
INSTALL_DIR="$SCRIPT_DIR"

cat > "$SERVICE_FILE" << EOF
[Unit]
Description=ZoomLink SDI Server
After=network.target

[Service]
Type=simple
User=$(whoami)
WorkingDirectory=$INSTALL_DIR
ExecStart=$(command -v node) $INSTALL_DIR/server.js
Restart=always
RestartSec=5
Environment=NODE_ENV=production
Environment=TCP_PORT=9300
Environment=API_PORT=9301
Environment=NDI_PREFIX=ZoomLink
Environment=RECORDING_DIR=$INSTALL_DIR/recordings

[Install]
WantedBy=multi-user.target
EOF

echo "  Service file created: $SERVICE_FILE"
echo ""
echo "  To install as a system service:"
echo "    sudo cp $SERVICE_FILE /etc/systemd/system/"
echo "    sudo systemctl daemon-reload"
echo "    sudo systemctl enable zoomlink-sdi"
echo "    sudo systemctl start zoomlink-sdi"
echo ""

mkdir -p "$SCRIPT_DIR/recordings"

echo "============================================"
echo -e "${GREEN}  Setup complete!${NC}"
echo "============================================"
echo ""
echo "  Features available:"
if [ "$SKIP_DECKLINK" = "0" ]; then
    echo -e "    ${GREEN}✓${NC} DeckLink SDI output"
else
    echo -e "    ${YELLOW}✗${NC} DeckLink SDI output (install Blackmagic drivers)"
fi
if [ "$NDI_FOUND" = "1" ]; then
    echo -e "    ${GREEN}✓${NC} NDI output"
else
    echo -e "    ${YELLOW}✗${NC} NDI output (install NDI SDK)"
fi
if command -v ffmpeg &>/dev/null; then
    echo -e "    ${GREEN}✓${NC} MP4 recording"
else
    echo -e "    ${YELLOW}✗${NC} MP4 recording (install FFmpeg)"
fi
echo ""
echo "  To start the server:"
echo "    cd $SCRIPT_DIR && node server.js"
echo ""
echo "  Or with custom settings:"
echo "    TCP_PORT=9300 API_PORT=9301 NDI_PREFIX=ZoomLink RECORDING_DIR=./recordings node server.js"
echo ""
echo "  Dashboard: http://localhost:9301/"
echo ""
echo "  Ports to open in firewall:"
echo "    TCP 9300 — frame stream from Mac app"
echo "    TCP 9301 — HTTP API / dashboard"
echo ""
echo "  Recording output: $SCRIPT_DIR/recordings/"
echo ""
