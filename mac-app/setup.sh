#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo ""
echo "============================================"
echo "  ZoomLink Remote SDI — Mac App Setup"
echo "  (Capture Client)"
echo "============================================"
echo ""

if [ "$(uname)" != "Darwin" ]; then
    echo -e "${RED}ERROR: This script is intended for macOS only${NC}"
    exit 1
fi

# Ensure Homebrew is on PATH
if [ -f /opt/homebrew/bin/brew ]; then
    eval "$(/opt/homebrew/bin/brew shellenv)"
elif [ -f /usr/local/bin/brew ]; then
    eval "$(/usr/local/bin/brew shellenv)"
fi

step_num=0
step() {
    step_num=$((step_num + 1))
    echo ""
    echo "--------------------------------------------"
    echo -e "${GREEN}[Step $step_num]${NC} $1"
    echo "--------------------------------------------"
}

skip() {
    echo -e "${CYAN}  Already installed — skipping${NC}"
}

fail() {
    echo -e "${RED}ERROR: $1${NC}"
    exit 1
}

# ===========================================================
step "Xcode Command Line Tools"
# ===========================================================
if xcode-select -p &>/dev/null; then
    skip
else
    echo "Installing Xcode Command Line Tools..."
    xcode-select --install 2>/dev/null || true
    echo -e "${YELLOW}If a dialog appeared, click Install and wait for it to finish.${NC}"
    echo "Then re-run this script."
    exit 0
fi

# ===========================================================
step "Homebrew"
# ===========================================================
if command -v brew &>/dev/null; then
    skip
else
    echo "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    if [ -f /opt/homebrew/bin/brew ]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [ -f /usr/local/bin/brew ]; then
        eval "$(/usr/local/bin/brew shellenv)"
    fi
    command -v brew &>/dev/null || fail "Homebrew installation failed"
fi

# ===========================================================
step "Node.js"
# ===========================================================
for nd in /opt/homebrew/opt/node@*/bin /opt/homebrew/opt/node/bin /usr/local/opt/node@*/bin /usr/local/opt/node/bin; do
    [ -d "$nd" ] && export PATH="$nd:$PATH"
done

if command -v node &>/dev/null; then
    NODE_VER=$(node -v 2>/dev/null)
    echo -e "${CYAN}  Found Node $NODE_VER${NC}"
else
    echo "Installing Node.js..."
    brew install node
    for nd in /opt/homebrew/opt/node@*/bin /opt/homebrew/opt/node/bin /usr/local/opt/node@*/bin /usr/local/opt/node/bin; do
        [ -d "$nd" ] && export PATH="$nd:$PATH"
    done
    command -v node &>/dev/null || fail "Node.js installation failed"
fi

NODE_MAJOR=$(node -v | sed 's/v//' | cut -d. -f1)
if [ "$NODE_MAJOR" -lt 18 ]; then
    echo -e "${RED}ERROR: Node.js 18+ required (found $(node -v))${NC}"
    exit 1
fi

echo "  node: $(which node)"
echo "  npm:  $(which npm 2>/dev/null || echo 'NOT FOUND')"

# ===========================================================
step "Python 3 (for node-gyp)"
# ===========================================================
PYTHON_PATH=""
for p in /opt/homebrew/bin/python3.12 /opt/homebrew/bin/python3 /usr/local/bin/python3.12 /usr/local/bin/python3 /usr/bin/python3; do
    if [ -f "$p" ]; then
        PYTHON_PATH="$p"
        break
    fi
done

if [ -n "$PYTHON_PATH" ]; then
    echo "  Using: $PYTHON_PATH"
else
    echo "Installing Python 3..."
    brew install python@3.12
    for p in /opt/homebrew/bin/python3.12 /opt/homebrew/bin/python3 /usr/local/bin/python3.12; do
        if [ -f "$p" ]; then
            PYTHON_PATH="$p"
            break
        fi
    done
    [ -n "$PYTHON_PATH" ] || fail "Python 3 installation failed"
fi
export PYTHON="$PYTHON_PATH"

# ===========================================================
step "Root project dependencies (shared src/ modules)"
# ===========================================================
if [ -f "$PROJECT_DIR/package.json" ]; then
    echo "  Installing root project npm packages..."
    cd "$PROJECT_DIR"
    npm install
    echo -e "  ${GREEN}Done${NC}"
else
    echo -e "  ${CYAN}No root package.json found — skipping (mac-app is self-contained)${NC}"
fi

# ===========================================================
step "Zoom Meeting SDK native addon"
# ===========================================================
ZOOM_SDK_PATH="${ZOOM_SDK_PATH:-/Users/debo/Documents/zoom-sdk-macos-6.7.6.75900}"

if [ -d "$PROJECT_DIR/zoom-meeting-sdk-addon/build/Release" ]; then
    echo -e "${CYAN}  Zoom addon already built — skipping${NC}"
    echo "  To rebuild: cd zoom-meeting-sdk-addon && npx node-gyp rebuild"
else
    if [ -d "$ZOOM_SDK_PATH" ]; then
        echo "  Installing Zoom SDK from: $ZOOM_SDK_PATH"
        cd "$PROJECT_DIR"
        node scripts/install-zoom-sdk.js "$ZOOM_SDK_PATH"

        echo "  Building Zoom Meeting SDK addon..."
        cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
        npm install
        CXX="$(pwd)/build-tools/cxx-objcpp.sh" npx node-gyp rebuild || {
            echo -e "${YELLOW}  Zoom addon build failed (non-fatal)${NC}"
            echo "  The app will run in stub mode without actual Zoom connectivity."
        }

        echo "  Linking Electron frameworks..."
        cd "$PROJECT_DIR"
        npm run link-frameworks || {
            echo -e "${YELLOW}  Framework linking failed (non-fatal)${NC}"
        }
    else
        echo -e "${YELLOW}  Zoom SDK not found at: $ZOOM_SDK_PATH${NC}"
        echo ""
        echo "  To set up the Zoom SDK:"
        echo "    1. Download the macOS Meeting SDK from Zoom Marketplace"
        echo "    2. Extract it (e.g., to ~/Documents/zoom-sdk-macos-6.x.x.xxxxx)"
        echo "    3. Re-run this script with:"
        echo "       ZOOM_SDK_PATH=/path/to/zoom-sdk bash mac-app/setup.sh"
        echo ""
        echo -e "${YELLOW}  Continuing without Zoom SDK (app will run in stub mode)...${NC}"
    fi
fi

# ===========================================================
step "Mac app dependencies"
# ===========================================================
cd "$SCRIPT_DIR"
npm install
echo -e "  ${GREEN}Done${NC}"

# ===========================================================
step "Verify setup"
# ===========================================================
echo ""
ALL_OK=true

echo "  Checking shared modules..."
if [ -f "$PROJECT_DIR/src/config/settings.js" ]; then
    echo -e "    ${GREEN}✓${NC} src/config/settings.js"
else
    echo -e "    ${RED}✗${NC} src/config/settings.js — MISSING"
    ALL_OK=false
fi

if [ -f "$PROJECT_DIR/src/zoom/session-manager.js" ]; then
    echo -e "    ${GREEN}✓${NC} src/zoom/session-manager.js"
else
    echo -e "    ${RED}✗${NC} src/zoom/session-manager.js — MISSING"
    ALL_OK=false
fi

if [ -f "$PROJECT_DIR/src/zoom/stream-handler.js" ]; then
    echo -e "    ${GREEN}✓${NC} src/zoom/stream-handler.js"
else
    echo -e "    ${RED}✗${NC} src/zoom/stream-handler.js — MISSING"
    ALL_OK=false
fi

echo ""
echo "  Checking mac-app files..."
for f in main.js ipc-handlers.js preload.js renderer/index.html network/stream-client.js; do
    if [ -f "$SCRIPT_DIR/$f" ]; then
        echo -e "    ${GREEN}✓${NC} mac-app/$f"
    else
        echo -e "    ${RED}✗${NC} mac-app/$f — MISSING"
        ALL_OK=false
    fi
done

echo ""
echo "  Checking Zoom SDK addon..."
if [ -d "$PROJECT_DIR/zoom-meeting-sdk-addon/build/Release" ]; then
    echo -e "    ${GREEN}✓${NC} Zoom Meeting SDK addon (built)"
else
    echo -e "    ${YELLOW}~${NC} Zoom Meeting SDK addon (not built — stub mode)"
fi

echo ""
echo "  Checking Electron..."
if npx electron --version &>/dev/null 2>&1; then
    echo -e "    ${GREEN}✓${NC} Electron $(npx electron --version 2>/dev/null)"
else
    echo -e "    ${YELLOW}~${NC} Electron (will install on first run)"
fi

echo ""
if [ "$ALL_OK" = true ]; then
    echo "============================================"
    echo -e "${GREEN}  Setup complete!${NC}"
    echo "============================================"
    echo ""
    echo "  To start the Mac app:"
    echo "    cd $SCRIPT_DIR && npx electron . --no-sandbox --disable-gpu-sandbox"
    echo ""
    echo "  Or simply:"
    echo "    cd $SCRIPT_DIR && npm start"
    echo ""
    echo "  Configure Zoom SDK credentials via the Settings (gear icon) in the app,"
    echo "  then connect to your Linux server's IP on port 9300."
    echo ""
else
    echo "============================================"
    echo -e "${RED}  Setup had issues — see above.${NC}"
    echo "============================================"
    exit 1
fi
