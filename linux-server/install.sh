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
echo "============================================"
echo ""

if [ "$(uname)" != "Linux" ]; then
    echo -e "${RED}ERROR: This script is intended for Linux only${NC}"
    exit 1
fi

echo -e "${CYAN}[1/5] Checking system dependencies...${NC}"

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
echo -e "${CYAN}[2/5] Installing npm dependencies...${NC}"
cd "$SCRIPT_DIR"
npm install

echo ""
echo -e "${CYAN}[3/5] Checking DeckLink SDK...${NC}"

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
echo -e "${CYAN}[4/5] Building DeckLink addon...${NC}"

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
echo -e "${CYAN}[5/5] Setting up systemd service...${NC}"

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

echo "============================================"
echo -e "${GREEN}  Setup complete!${NC}"
echo "============================================"
echo ""
echo "  To start the server:"
echo "    cd $SCRIPT_DIR && node server.js"
echo ""
echo "  Or with custom ports:"
echo "    TCP_PORT=9300 API_PORT=9301 node server.js"
echo ""
echo "  Dashboard: http://localhost:9301/"
echo ""
echo "  Ports to open in firewall:"
echo "    TCP 9300 — frame stream from Mac app"
echo "    TCP 9301 — HTTP API / dashboard"
echo ""
if [ "$SKIP_DECKLINK" = "1" ]; then
    echo -e "${YELLOW}  Note: DeckLink support not available. Install Blackmagic"
    echo -e "  Desktop Video drivers and re-run this script.${NC}"
    echo ""
fi
