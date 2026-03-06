#!/bin/bash

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
    for p in /opt/homebrew/bin/python3.12 /opt/homebrew/bin/python3.11 /opt/homebrew/bin/python3.13 /usr/bin/python3; do
        if [ -f "$p" ]; then
            PYTHON_PATH="$p"
            break
        fi
    done
    if [ ! -f "$PYTHON_PATH" ]; then
        echo -e "${RED}ERROR: No python3 found. Run: brew install python@3.12${NC}"
        exit 1
    fi
fi

export PYTHON="$PYTHON_PATH"
export NODE_TLS_REJECT_UNAUTHORIZED=0

ARCH=$(uname -m)

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[1/8]${NC} Installing FFmpeg..."
echo "--------------------------------------------"
if command -v ffmpeg &>/dev/null; then
    echo "FFmpeg already installed: $(ffmpeg -version 2>&1 | head -1)"
else
    if command -v brew &>/dev/null; then
        echo "Installing FFmpeg via Homebrew (this may take a few minutes)..."
        brew install ffmpeg
    else
        echo -e "${RED}ERROR: Homebrew not found. Install it first: https://brew.sh${NC}"
        echo "Then run: brew install ffmpeg"
        exit 1
    fi
fi
echo ""

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[2/8]${NC} Installing Node dependencies..."
echo "--------------------------------------------"
cd "$PROJECT_DIR"
npm install --ignore-scripts || { echo -e "${RED}npm install failed${NC}"; exit 1; }

cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
npm install || { echo -e "${RED}addon npm install failed${NC}"; exit 1; }
cd "$PROJECT_DIR"

ELECTRON_APP="$PROJECT_DIR/node_modules/electron/dist/Electron.app"
if [ ! -d "$ELECTRON_APP" ]; then
    echo "Extracting Electron binary..."
    node node_modules/electron/install.js
fi
if [ ! -d "$ELECTRON_APP" ]; then
    echo -e "${RED}ERROR: Electron.app not found after install${NC}"
    exit 1
fi
echo -e "${GREEN}Electron.app OK${NC}"
echo ""

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[3/8]${NC} Installing Zoom SDK..."
echo "--------------------------------------------"
node scripts/install-zoom-sdk.js "$ZOOM_SDK_PATH" || { echo -e "${RED}SDK install failed${NC}"; exit 1; }
echo ""

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[4/8]${NC} Symlinking frameworks into Electron..."
echo "--------------------------------------------"
node scripts/link-electron-frameworks.js || { echo -e "${RED}Framework linking failed${NC}"; exit 1; }
echo ""

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[5/8]${NC} Installing grandiose (NDI)..."
echo "--------------------------------------------"
GRANDIOSE_DIR="$PROJECT_DIR/node_modules/grandiose"

if [ -f "$GRANDIOSE_DIR/build/Release/grandiose.node" ]; then
    echo "grandiose already built, skipping."
else
    # Clean slate
    rm -rf "$GRANDIOSE_DIR"

    echo "Cloning grandiose..."
    git clone https://github.com/rse/grandiose.git "$GRANDIOSE_DIR"
    if [ ! -d "$GRANDIOSE_DIR" ]; then
        echo -e "${RED}ERROR: git clone failed${NC}"
        exit 1
    fi

    # --- NDI SDK ---
    NDI_EXPANDED="/tmp/ndi_sdk_plexiso_expanded"
    NDI_SRC="$NDI_EXPANDED/NDI SDK for Apple"
    NDI_PKG="/tmp/ndi_sdk_plexiso.pkg"

    if [ ! -d "$NDI_SRC/include" ]; then
        echo "Downloading NDI SDK v6..."
        rm -rf "$NDI_EXPANDED"
        rm -f "$NDI_PKG"

        # Try known working URLs in order
        NDI_DOWNLOADED=false
        for URL in \
            "https://downloads.ndi.tv/SDK/NDI_SDK_Mac/Install_NDI_SDK_v6_Apple.pkg" \
            "https://downloads.ndi.tv/SDK/NDI_SDK_v6/Install_NDI_SDK_v6_Apple.pkg" \
            "https://downloads.ndi.tv/SDK/NDI_SDK_v6_Apple/Install_NDI_SDK_v6_Apple.pkg"; do
            echo "  Trying: $URL"
            curl -k -L -o "$NDI_PKG" "$URL" 2>/dev/null
            FILETYPE=$(file "$NDI_PKG" 2>/dev/null)
            if echo "$FILETYPE" | grep -q "xar"; then
                echo -e "  ${GREEN}Downloaded OK${NC}"
                NDI_DOWNLOADED=true
                break
            fi
            rm -f "$NDI_PKG"
        done

        if [ "$NDI_DOWNLOADED" = false ]; then
            echo -e "${RED}ERROR: Could not download NDI SDK from any URL.${NC}"
            echo "Download manually from https://ndi.video/for-developers/ndi-sdk/"
            echo "Then set NDI_SDK_PATH and re-run."
            exit 1
        fi

        # Expand pkg
        rm -rf "$NDI_EXPANDED"
        pkgutil --expand "$NDI_PKG" "$NDI_EXPANDED"

        # Extract payload
        cd "$NDI_EXPANDED"
        if [ -d "NDI SDK for Apple.pkg" ] && [ -f "NDI SDK for Apple.pkg/Payload" ]; then
            cat "NDI SDK for Apple.pkg/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
        else
            # Try any .pkg subdirectory
            for pkg_dir in *.pkg; do
                if [ -f "$pkg_dir/Payload" ]; then
                    cat "$pkg_dir/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
                fi
            done
        fi
        cd "$PROJECT_DIR"

        # If standard path doesn't exist, search for headers
        if [ ! -d "$NDI_SRC/include" ]; then
            FOUND_HEADER=$(find "$NDI_EXPANDED" -name "Processing.NDI.Lib.h" -type f 2>/dev/null | head -1)
            if [ -n "$FOUND_HEADER" ]; then
                FOUND_INCLUDE_DIR=$(dirname "$FOUND_HEADER")
                FOUND_ROOT=$(dirname "$FOUND_INCLUDE_DIR")
                echo "Found NDI SDK at: $FOUND_ROOT"
                mkdir -p "$NDI_SRC"
                cp -R "$FOUND_ROOT/"* "$NDI_SRC/"
            fi
        fi
    else
        echo "NDI SDK already extracted, reusing."
    fi

    if [ ! -d "$NDI_SRC/include" ]; then
        echo -e "${RED}ERROR: NDI SDK extraction failed — no include/ directory found${NC}"
        echo "Contents of $NDI_EXPANDED:"
        find "$NDI_EXPANDED" -maxdepth 3 -type d 2>/dev/null
        exit 1
    fi

    # --- Copy to grandiose expected paths ---
    echo "Setting up NDI libraries for grandiose..."
    mkdir -p "$GRANDIOSE_DIR/ndi/include"
    cp -R "$NDI_SRC/include/"* "$GRANDIOSE_DIR/ndi/include/"

    # grandiose binding.gyp expects: ndi/lib/mac-a64/ (arm64) or ndi/lib/mac-x64/ (x64)
    if [ "$ARCH" = "arm64" ]; then
        GRANDIOSE_LIB_DIR="$GRANDIOSE_DIR/ndi/lib/mac-a64"
    else
        GRANDIOSE_LIB_DIR="$GRANDIOSE_DIR/ndi/lib/mac-x64"
    fi
    mkdir -p "$GRANDIOSE_LIB_DIR"

    # Find libndi.dylib wherever the SDK put it
    NDI_DYLIB=$(find "$NDI_SRC" -name "libndi.dylib" -type f 2>/dev/null | head -1)
    if [ -z "$NDI_DYLIB" ]; then
        NDI_DYLIB=$(find "$NDI_EXPANDED" -name "libndi.dylib" -type f 2>/dev/null | head -1)
    fi

    if [ -n "$NDI_DYLIB" ]; then
        echo "Found libndi.dylib: $NDI_DYLIB"
        cp "$NDI_DYLIB" "$GRANDIOSE_LIB_DIR/libndi.dylib"
        echo "Copied to: $GRANDIOSE_LIB_DIR/libndi.dylib"
    else
        echo -e "${RED}ERROR: libndi.dylib not found anywhere in NDI SDK${NC}"
        echo "SDK contents:"
        find "$NDI_SRC" -type f -name "*.dylib" 2>/dev/null
        find "$NDI_EXPANDED" -type f -name "*.dylib" 2>/dev/null
        exit 1
    fi

    # --- Build grandiose ---
    echo "Building grandiose..."
    cd "$GRANDIOSE_DIR"
    PYTHON="$PYTHON_PATH" npx node-gyp rebuild
    BUILD_RESULT=$?
    cd "$PROJECT_DIR"

    if [ $BUILD_RESULT -ne 0 ]; then
        echo -e "${RED}ERROR: grandiose build failed${NC}"
        exit 1
    fi

    if [ ! -f "$GRANDIOSE_DIR/build/Release/grandiose.node" ]; then
        echo -e "${RED}ERROR: grandiose.node not produced${NC}"
        exit 1
    fi

    echo -e "${GREEN}grandiose built successfully${NC}"
fi
echo ""

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[6/8]${NC} Building Zoom native addon..."
echo "--------------------------------------------"
cd "$PROJECT_DIR/zoom-meeting-sdk-addon"
chmod +x build-tools/cxx-objcpp.sh
CXX="$(pwd)/build-tools/cxx-objcpp.sh" PYTHON="$PYTHON_PATH" npx node-gyp rebuild
BUILD_RESULT=$?
cd "$PROJECT_DIR"

if [ $BUILD_RESULT -ne 0 ]; then
    echo -e "${RED}ERROR: Zoom addon build failed${NC}"
    exit 1
fi
echo ""

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[7/8]${NC} Checking .env file..."
echo "--------------------------------------------"
if [ -f "$PROJECT_DIR/.env" ]; then
    echo ".env file exists."
    if grep -q "your_sdk_key_here" "$PROJECT_DIR/.env" 2>/dev/null; then
        echo -e "${YELLOW}WARNING: .env has placeholder values — edit with real SDK credentials${NC}"
    fi
else
    cat > "$PROJECT_DIR/.env" << 'ENVEOF'
ZOOM_SDK_KEY=your_sdk_key_here
ZOOM_SDK_SECRET=your_sdk_secret_here
ZOOM_MEETING_BOT_NAME=PlexISO
ENVEOF
    echo -e "${YELLOW}Created .env — edit with real Zoom SDK credentials before running${NC}"
fi
echo ""

# ============================================================
echo "--------------------------------------------"
echo -e "${GREEN}[8/8]${NC} Verifying build..."
echo "--------------------------------------------"
ALL_OK=true

if [ -f "$PROJECT_DIR/zoom-meeting-sdk-addon/build/Release/zoom_meeting_sdk.node" ]; then
    echo -e "${GREEN}  ✓ Zoom addon${NC}"
else
    echo -e "${RED}  ✗ Zoom addon NOT found${NC}"; ALL_OK=false
fi

if [ -f "$PROJECT_DIR/node_modules/grandiose/build/Release/grandiose.node" ]; then
    echo -e "${GREEN}  ✓ Grandiose (NDI)${NC}"
else
    echo -e "${RED}  ✗ Grandiose NOT found${NC}"; ALL_OK=false
fi

if [ -d "$ELECTRON_APP" ]; then
    echo -e "${GREEN}  ✓ Electron.app${NC}"
else
    echo -e "${RED}  ✗ Electron.app NOT found${NC}"; ALL_OK=false
fi

FRAMEWORK_LINK="$ELECTRON_APP/Contents/Frameworks/ZoomSDK.framework"
if [ -L "$FRAMEWORK_LINK" ] || [ -d "$FRAMEWORK_LINK" ]; then
    echo -e "${GREEN}  ✓ ZoomSDK framework linked${NC}"
else
    echo -e "${RED}  ✗ ZoomSDK framework NOT linked${NC}"; ALL_OK=false
fi

if command -v ffmpeg &>/dev/null; then
    echo -e "${GREEN}  ✓ FFmpeg${NC}"
else
    echo -e "${RED}  ✗ FFmpeg NOT found${NC}"; ALL_OK=false
fi

echo ""
if [ "$ALL_OK" = true ]; then
    echo "============================================"
    echo -e "${GREEN}  Setup complete! All checks passed.${NC}"
    echo "============================================"
    echo ""
    echo "To run:"
    echo "  cd $PROJECT_DIR && npm start"
else
    echo "============================================"
    echo -e "${RED}  Setup failed — see errors above.${NC}"
    echo "============================================"
    exit 1
fi
echo ""
