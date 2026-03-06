#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ZOOM_SDK_PATH="${1:-/Users/debo/Documents/zoom-sdk-macos-6.7.6.75900}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

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

echo ""
echo "============================================"
echo "  PlexISO - Full macOS Setup"
echo "  (brand-new Mac compatible)"
echo "============================================"
echo ""

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
step "Python 3.12"
# ===========================================================
PYTHON_PATH=""
for p in /opt/homebrew/bin/python3.12 /usr/local/bin/python3.12; do
    if [ -f "$p" ]; then
        PYTHON_PATH="$p"
        break
    fi
done

if [ -n "$PYTHON_PATH" ]; then
    skip
    echo "  Using: $PYTHON_PATH"
else
    echo "Installing Python 3.12..."
    brew install python@3.12
    for p in /opt/homebrew/bin/python3.12 /usr/local/bin/python3.12; do
        if [ -f "$p" ]; then
            PYTHON_PATH="$p"
            break
        fi
    done
    [ -n "$PYTHON_PATH" ] || fail "Python 3.12 installation failed"
fi
export PYTHON="$PYTHON_PATH"

# ===========================================================
step "Node.js 20"
# ===========================================================
if command -v node &>/dev/null; then
    NODE_VER=$(node -v 2>/dev/null)
    echo -e "${CYAN}  Found Node $NODE_VER — skipping${NC}"
else
    echo "Installing Node.js 20..."
    brew install node@20
    export PATH="/opt/homebrew/opt/node@20/bin:$PATH"
    command -v node &>/dev/null || fail "Node.js installation failed"
fi

# ===========================================================
step "Git"
# ===========================================================
if command -v git &>/dev/null; then
    skip
else
    echo "Installing Git..."
    brew install git
    command -v git &>/dev/null || fail "Git installation failed"
fi

# ===========================================================
step "FFmpeg"
# ===========================================================
if command -v ffmpeg &>/dev/null; then
    skip
    echo "  $(ffmpeg -version 2>&1 | head -1)"
else
    echo "Installing FFmpeg (this may take a few minutes)..."
    brew install ffmpeg
    command -v ffmpeg &>/dev/null || fail "FFmpeg installation failed"
fi

# ===========================================================
step "Zoom SDK"
# ===========================================================
if [ ! -d "$ZOOM_SDK_PATH" ]; then
    fail "Zoom SDK not found at $ZOOM_SDK_PATH
  Download from https://developers.zoom.us/docs/meeting-sdk/
  Usage: ./scripts/setup-mac.sh /path/to/zoom-sdk-macos-6.7.6.75900"
fi
echo "  Source: $ZOOM_SDK_PATH"

# ===========================================================
step "Node dependencies (npm install)"
# ===========================================================
export NODE_TLS_REJECT_UNAUTHORIZED=0
cd "$PROJECT_DIR"

if [ -d "node_modules/electron/dist/Electron.app" ] && [ -d "zoom-meeting-sdk-addon/node_modules/node-addon-api" ]; then
    skip
else
    npm install --ignore-scripts || fail "npm install failed"

    cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
    npm install || fail "addon npm install failed"
    cd "$PROJECT_DIR"

    ELECTRON_APP="$PROJECT_DIR/node_modules/electron/dist/Electron.app"
    if [ ! -d "$ELECTRON_APP" ]; then
        echo "  Extracting Electron binary..."
        node node_modules/electron/install.js
    fi
    [ -d "$ELECTRON_APP" ] || fail "Electron.app extraction failed"
    echo -e "${GREEN}  Electron.app OK${NC}"
fi

# ===========================================================
step "Install Zoom SDK into addon"
# ===========================================================
SDK_DEST="$PROJECT_DIR/zoom-meeting-sdk-addon/sdk/lib/ZoomSDK.framework"
if [ -d "$SDK_DEST" ]; then
    skip
else
    node scripts/install-zoom-sdk.js "$ZOOM_SDK_PATH" || fail "SDK install failed"
fi

# ===========================================================
step "Symlink frameworks into Electron"
# ===========================================================
ELECTRON_APP="$PROJECT_DIR/node_modules/electron/dist/Electron.app"
FRAMEWORK_LINK="$ELECTRON_APP/Contents/Frameworks/ZoomSDK.framework"
if [ -L "$FRAMEWORK_LINK" ] || [ -d "$FRAMEWORK_LINK" ]; then
    skip
else
    node scripts/link-electron-frameworks.js || fail "Framework linking failed"
fi

# ===========================================================
step "Macadam (DeckLink SDI)"
# ===========================================================
MACADAM_DIR="$PROJECT_DIR/node_modules/macadam"

if [ -f "$MACADAM_DIR/build/Release/macadam.node" ]; then
    skip
else
    echo "  Installing macadam for DeckLink SDI output..."
    cd "$PROJECT_DIR"
    rm -rf "$MACADAM_DIR"
    npm install macadam --no-save --ignore-scripts 2>/dev/null

    if [ -d "$MACADAM_DIR" ]; then
        cd "$MACADAM_DIR"

        echo "  Patching binding.gyp to remove capture sources (playback-only)..."
        if [ -f binding.gyp ]; then
            sed -i '' '/"src\/capture/d' binding.gyp
        fi

        PYTHON="$PYTHON_PATH" npx node-gyp rebuild 2>&1 || {
            echo -e "${YELLOW}  macadam build failed — DeckLink output will be unavailable${NC}"
            echo -e "${YELLOW}  This is OK if you don't have Blackmagic Desktop Video installed${NC}"
        }
        cd "$PROJECT_DIR"

        if [ -f "$MACADAM_DIR/build/Release/macadam.node" ]; then
            echo -e "${GREEN}  macadam built OK (playback-only)${NC}"
        else
            echo -e "${YELLOW}  macadam.node not produced — DeckLink features disabled${NC}"
        fi
    else
        echo -e "${YELLOW}  macadam package not found — DeckLink features disabled${NC}"
    fi
fi

# ===========================================================
step "Grandiose (NDI)"
# ===========================================================
GRANDIOSE_DIR="$PROJECT_DIR/node_modules/grandiose"

if [ -f "$GRANDIOSE_DIR/build/Release/grandiose.node" ]; then
    skip
else
    rm -rf "$GRANDIOSE_DIR"
    echo "  Cloning grandiose..."
    git clone https://github.com/rse/grandiose.git "$GRANDIOSE_DIR" || fail "git clone grandiose failed"

    # --- Download NDI SDK ---
    NDI_EXPANDED="/tmp/ndi_sdk_plexiso_expanded"
    NDI_SRC="$NDI_EXPANDED/NDI SDK for Apple"
    NDI_PKG="/tmp/ndi_sdk_plexiso.pkg"

    if [ ! -d "$NDI_SRC/include" ]; then
        echo "  Downloading NDI SDK v6..."
        rm -rf "$NDI_EXPANDED"
        rm -f "$NDI_PKG"

        NDI_OK=false
        for URL in \
            "https://downloads.ndi.tv/SDK/NDI_SDK_Mac/Install_NDI_SDK_v6_Apple.pkg" \
            "https://downloads.ndi.tv/SDK/NDI_SDK_v6/Install_NDI_SDK_v6_Apple.pkg" \
            "https://downloads.ndi.tv/SDK/NDI_SDK_v6_Apple/Install_NDI_SDK_v6_Apple.pkg"; do
            echo "    Trying: $URL"
            curl -k -L -o "$NDI_PKG" "$URL" 2>/dev/null
            if file "$NDI_PKG" 2>/dev/null | grep -q "xar"; then
                NDI_OK=true
                break
            fi
            rm -f "$NDI_PKG"
        done
        [ "$NDI_OK" = true ] || fail "Could not download NDI SDK. Get it from https://ndi.video/for-developers/ndi-sdk/"

        rm -rf "$NDI_EXPANDED"
        pkgutil --expand "$NDI_PKG" "$NDI_EXPANDED"
        cd "$NDI_EXPANDED"
        if [ -d "NDI SDK for Apple.pkg" ] && [ -f "NDI SDK for Apple.pkg/Payload" ]; then
            cat "NDI SDK for Apple.pkg/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
        else
            for pkg_dir in *.pkg; do
                [ -f "$pkg_dir/Payload" ] && cat "$pkg_dir/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
            done
        fi
        cd "$PROJECT_DIR"

        if [ ! -d "$NDI_SRC/include" ]; then
            FOUND=$(find "$NDI_EXPANDED" -name "Processing.NDI.Lib.h" -type f 2>/dev/null | head -1)
            if [ -n "$FOUND" ]; then
                FOUND_ROOT=$(dirname "$(dirname "$FOUND")")
                mkdir -p "$NDI_SRC"
                cp -R "$FOUND_ROOT/"* "$NDI_SRC/"
            fi
        fi
        [ -d "$NDI_SRC/include" ] || fail "NDI SDK extraction failed"
    else
        echo "  NDI SDK already extracted, reusing."
    fi

    # --- Copy to grandiose expected paths ---
    mkdir -p "$GRANDIOSE_DIR/ndi/include"
    cp -R "$NDI_SRC/include/"* "$GRANDIOSE_DIR/ndi/include/"

    ARCH=$(uname -m)
    if [ "$ARCH" = "arm64" ]; then
        GR_LIB="$GRANDIOSE_DIR/ndi/lib/mac-a64"
    else
        GR_LIB="$GRANDIOSE_DIR/ndi/lib/mac-x64"
    fi
    mkdir -p "$GR_LIB"

    NDI_DYLIB=$(find "$NDI_SRC" -name "libndi.dylib" -type f 2>/dev/null | head -1)
    [ -z "$NDI_DYLIB" ] && NDI_DYLIB=$(find "$NDI_EXPANDED" -name "libndi.dylib" -type f 2>/dev/null | head -1)
    [ -n "$NDI_DYLIB" ] || fail "libndi.dylib not found in NDI SDK"
    cp "$NDI_DYLIB" "$GR_LIB/libndi.dylib"
    echo "  libndi.dylib → $GR_LIB/"

    # --- Install deps + build ---
    cd "$GRANDIOSE_DIR"
    npm install --ignore-scripts || fail "grandiose npm install failed"
    PYTHON="$PYTHON_PATH" npx node-gyp rebuild || fail "grandiose build failed"
    cd "$PROJECT_DIR"

    [ -f "$GRANDIOSE_DIR/build/Release/grandiose.node" ] || fail "grandiose.node not produced"
    echo -e "${GREEN}  grandiose built OK${NC}"
fi

# ===========================================================
step "Build Zoom native addon"
# ===========================================================
ADDON_NODE="$PROJECT_DIR/zoom-meeting-sdk-addon/build/Release/zoom_meeting_sdk.node"
if [ -f "$ADDON_NODE" ]; then
    skip
else
    cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
    chmod +x build-tools/cxx-objcpp.sh
    CXX="$(pwd)/build-tools/cxx-objcpp.sh" PYTHON="$PYTHON_PATH" npx node-gyp rebuild || fail "Zoom addon build failed"
    cd "$PROJECT_DIR"
    [ -f "$ADDON_NODE" ] || fail "zoom_meeting_sdk.node not produced"
fi

# ===========================================================
step ".env configuration"
# ===========================================================
if [ -f "$PROJECT_DIR/.env" ]; then
    echo "  .env exists."
    if grep -q "your_sdk_key_here" "$PROJECT_DIR/.env" 2>/dev/null; then
        echo -e "${YELLOW}  WARNING: .env has placeholder values — edit with real SDK credentials${NC}"
    fi
else
    cat > "$PROJECT_DIR/.env" << 'ENVEOF'
ZOOM_SDK_KEY=your_sdk_key_here
ZOOM_SDK_SECRET=your_sdk_secret_here
ZOOM_MEETING_BOT_NAME=PlexISO
ENVEOF
    echo -e "${YELLOW}  Created .env — edit with real Zoom SDK credentials before running${NC}"
fi

# ===========================================================
step "Final verification"
# ===========================================================
ALL_OK=true

check() {
    local label="$1"
    shift
    if "$@" &>/dev/null; then
        echo -e "${GREEN}  ✓ $label${NC}"
    else
        echo -e "${RED}  ✗ $label${NC}"
        ALL_OK=false
    fi
}

check "Zoom addon"     test -f "$ADDON_NODE"
check "Grandiose (NDI)" test -f "$GRANDIOSE_DIR/build/Release/grandiose.node"
check "Electron.app"   test -d "$ELECTRON_APP"
check "ZoomSDK linked" test -e "$FRAMEWORK_LINK"
check "FFmpeg"         command -v ffmpeg
check "Node.js"        command -v node
check "Python"         test -f "$PYTHON_PATH"
check "Git"            command -v git

if [ -f "$MACADAM_DIR/build/Release/macadam.node" ]; then
    echo -e "${GREEN}  ✓ Macadam (DeckLink)${NC}"
else
    echo -e "${YELLOW}  ~ Macadam (DeckLink) — not built (optional, needs Blackmagic drivers)${NC}"
fi

echo ""
if [ "$ALL_OK" = true ]; then
    echo "============================================"
    echo -e "${GREEN}  Setup complete! All checks passed.${NC}"
    echo "============================================"
    echo ""
    echo "  To run:  cd $PROJECT_DIR && npm start"
    echo ""
else
    echo "============================================"
    echo -e "${RED}  Setup failed — see errors above.${NC}"
    echo "============================================"
    exit 1
fi
