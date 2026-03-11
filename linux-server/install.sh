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

npm install --ignore-scripts 2>&1 || {
    echo -e "${YELLOW}  npm install had warnings${NC}"
}

GRANDIOSE_DIR="$SCRIPT_DIR/node_modules/grandiose"
if [ -d "$GRANDIOSE_DIR" ]; then
    echo "  Cleaning previous grandiose build for fresh patch..."
    rm -rf "$GRANDIOSE_DIR"
fi

npm install --ignore-scripts 2>&1 || {
    echo -e "${YELLOW}  npm install had warnings${NC}"
}

GRANDIOSE_DIR="$SCRIPT_DIR/node_modules/grandiose"
if [ -d "$GRANDIOSE_DIR" ]; then
    echo "  Patching grandiose for Linux compatibility (itoa -> snprintf)..."
    UTIL_CC="$GRANDIOSE_DIR/src/grandiose_util.cc"
    if [ -f "$UTIL_CC" ]; then
        if grep -q 'itoa' "$UTIL_CC"; then
            cat > /tmp/grandiose_patch.py << 'PYEOF'
import re, sys
with open(sys.argv[1], 'r') as f:
    code = f.read()

# Fix 1: Original source has (across two lines):
#   throwStatus = napi_throw_error(env,
#     itoa(errorInfo->error_code, errorCode, 10), errorInfo->error_message);
# Replace with:
#   snprintf(errorCode, sizeof(errorCode), "%d", errorInfo->error_code);
#   throwStatus = napi_throw_error(env, errorCode, errorInfo->error_message);
code = re.sub(
    r'throwStatus\s*=\s*napi_throw_error\(env,\s*\n\s*itoa\(errorInfo->error_code,\s*errorCode,\s*10\),\s*errorInfo->error_message\)',
    'snprintf(errorCode, sizeof(errorCode), "%d", errorInfo->error_code);\n  throwStatus = napi_throw_error(env, errorCode, errorInfo->error_message)',
    code
)

# Fix 2: Original source has (across two lines):
#   status = napi_create_string_utf8(env, itoa(c->status, errorChars, 10),
#     NAPI_AUTO_LENGTH, &errorCode);
# Replace with:
#   snprintf(errorChars, sizeof(errorChars), "%d", c->status);
#   status = napi_create_string_utf8(env, errorChars,
#     NAPI_AUTO_LENGTH, &errorCode);
code = re.sub(
    r'status\s*=\s*napi_create_string_utf8\(env,\s*itoa\(c->status,\s*errorChars,\s*10\)',
    'snprintf(errorChars, sizeof(errorChars), "%d", c->status);\n    status = napi_create_string_utf8(env, errorChars',
    code
)

with open(sys.argv[1], 'w') as f:
    f.write(code)
print("  Applied grandiose itoa patch successfully")
PYEOF
            python3 /tmp/grandiose_patch.py "$UTIL_CC"
            rm -f /tmp/grandiose_patch.py
        else
            echo "  grandiose_util.cc already patched or doesn't use itoa"
        fi
    fi

    SEND_CC="$GRANDIOSE_DIR/src/grandiose_send.cc"
    if [ -f "$SEND_CC" ]; then
        if ! grep -q '<cmath>' "$SEND_CC"; then
            sed -i '1s/^/#include <cmath>\n/' "$SEND_CC"
            echo "  Patched grandiose_send.cc: added #include <cmath>"
        fi
    fi

    echo "  Building grandiose native addon..."
    cd "$GRANDIOSE_DIR"
    npx node-gyp rebuild 2>&1 || {
        echo -e "${YELLOW}  grandiose build failed — NDI will run in mock mode${NC}"
        echo "  This is expected if the NDI SDK runtime library is not installed."
    }
    cd "$SCRIPT_DIR"
else
    echo -e "${YELLOW}  grandiose not downloaded — NDI will run in mock mode${NC}"
fi

echo ""
echo -e "${CYAN}[5/7] Checking DeckLink SDK...${NC}"

DECKLINK_SDK_INCLUDE=""

if [ -n "$DECKLINK_SDK_PATH" ]; then
    if [ -f "$DECKLINK_SDK_PATH/LinuxCOM.h" ] && [ -f "$DECKLINK_SDK_PATH/DeckLinkAPI.h" ]; then
        DECKLINK_SDK_INCLUDE="$DECKLINK_SDK_PATH"
        echo -e "  DeckLink SDK (Linux) found: $DECKLINK_SDK_INCLUDE"
    else
        echo "  Searching for Linux headers inside DECKLINK_SDK_PATH..."
        while IFS= read -r -d '' found; do
            DECKLINK_SDK_INCLUDE="$(dirname "$found")"
            echo -e "  DeckLink SDK (Linux) found: $DECKLINK_SDK_INCLUDE"
            break
        done < <(find "$DECKLINK_SDK_PATH" -path "*/Linux/include/LinuxCOM.h" -print0 2>/dev/null)

        if [ -z "$DECKLINK_SDK_INCLUDE" ]; then
            echo -e "${YELLOW}  Could not find Linux/include/ inside DECKLINK_SDK_PATH${NC}"
            echo "  Path given: $DECKLINK_SDK_PATH"
            echo "  Expected structure: .../Linux/include/DeckLinkAPI.h + LinuxCOM.h"
        fi
    fi
fi

if [ -z "$DECKLINK_SDK_INCLUDE" ]; then
    SYSTEM_PATHS=(
        "/usr/include"
        "/usr/local/include"
        "/opt/decklink/include"
    )
    for p in "${SYSTEM_PATHS[@]}"; do
        if [ -f "$p/DeckLinkAPI.h" ] && [ -f "$p/LinuxCOM.h" ]; then
            DECKLINK_SDK_INCLUDE="$p"
            echo -e "  DeckLink SDK (Linux) found: $p"
            break
        fi
    done
fi

if [ -z "$DECKLINK_SDK_INCLUDE" ]; then
    echo "  Searching common download locations for Linux SDK headers..."
    SEARCH_ROOTS=(
        "$HOME/Downloads"
        "$HOME/Desktop"
        "/tmp"
        "/opt"
    )
    for root in "${SEARCH_ROOTS[@]}"; do
        if [ -d "$root" ]; then
            while IFS= read -r -d '' found; do
                DECKLINK_SDK_INCLUDE="$(dirname "$found")"
                echo -e "  DeckLink SDK (Linux) found: $DECKLINK_SDK_INCLUDE"
                break 2
            done < <(find "$root" -maxdepth 6 -path "*/Linux/include/LinuxCOM.h" -print0 2>/dev/null)
        fi
    done
fi

if [ -z "$DECKLINK_SDK_INCLUDE" ]; then
    echo -e "${YELLOW}  DeckLink SDK headers not found.${NC}"
    echo ""
    echo "  To install DeckLink support, provide the SDK path:"
    echo ""
    echo "    DECKLINK_SDK_PATH=~/Downloads/Blackmagic_DeckLink_SDK_15.2 bash install.sh"
    echo ""
    echo "  The path should point to either:"
    echo "    - The extracted SDK root (containing Linux/include/)"
    echo "    - Or directly to the include/ folder with DeckLinkAPI.h"
    echo ""
    echo "  Download the SDK from:"
    echo "    https://www.blackmagicdesign.com/developer/product/decklink"
    echo ""
    echo "  Also install Desktop Video drivers:"
    echo "    sudo dpkg -i desktopvideo_*.deb"
    echo ""
    echo -e "${YELLOW}  Continuing without DeckLink support...${NC}"
    SKIP_DECKLINK=1
else
    SKIP_DECKLINK=0
fi

echo ""
echo -e "${CYAN}[6/7] Building DeckLink addon...${NC}"

if [ "$SKIP_DECKLINK" = "1" ]; then
    echo -e "${YELLOW}  Skipped (DeckLink SDK not found)${NC}"
else
    ADDON_SRC="$SCRIPT_DIR/decklink-addon/src"
    ADDON_INCLUDE="$SCRIPT_DIR/decklink-addon/sdk-include"

    mkdir -p "$ADDON_INCLUDE"
    echo "  Copying SDK headers from $DECKLINK_SDK_INCLUDE..."
    for hdr in "$DECKLINK_SDK_INCLUDE"/DeckLinkAPI*.h; do
        [ -f "$hdr" ] && cp -f "$hdr" "$ADDON_INCLUDE/"
    done
    cp -f "$DECKLINK_SDK_INCLUDE"/LinuxCOM.h "$ADDON_INCLUDE/" 2>/dev/null || true

    DISPATCH_CPP="$ADDON_SRC/DeckLinkAPIDispatch.cpp"
    if [ ! -f "$DISPATCH_CPP" ]; then
        echo -e "${RED}  DeckLinkAPIDispatch.cpp missing from addon src — build will fail${NC}"
        echo "  This file ships with the addon and should not need to be copied."
    else
        echo "  Using bundled Linux DeckLinkAPIDispatch.cpp"
    fi

    cd "$SCRIPT_DIR/decklink-addon"
    npm install
    npx node-gyp rebuild --decklink_sdk_include="$ADDON_INCLUDE" || {
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
