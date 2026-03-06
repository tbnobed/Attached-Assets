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

ELECTRON_APP="$PROJECT_DIR/node_modules/electron/dist/Electron.app"
if [ ! -d "$ELECTRON_APP" ]; then
    echo "Extracting Electron binary..."
    node node_modules/electron/install.js
fi

if [ -d "$ELECTRON_APP" ]; then
    echo -e "${GREEN}Electron.app extracted OK${NC}"
else
    echo -e "${RED}ERROR: Electron.app still not found after install. Check node_modules/electron/.${NC}"
    exit 1
fi
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

    NDI_EXPANDED="/tmp/ndi_sdk_plexiso_expanded"
    NDI_SRC="$NDI_EXPANDED/NDI SDK for Apple"

    if [ ! -d "$NDI_SRC/include" ]; then
        echo "Downloading NDI SDK v6..."
        NDI_DMG="/tmp/ndi_sdk_plexiso.dmg"
        NDI_MOUNT="/tmp/ndi_sdk_mount"

        rm -rf "$NDI_EXPANDED"

        curl -k -L -o "$NDI_DMG" "https://downloads.ndi.tv/SDK/NDI_SDK_Mac/Install_NDI_SDK_v6_Apple.pkg" 2>/dev/null || true

        FILETYPE=$(file "$NDI_DMG" 2>/dev/null || echo "unknown")
        echo "Downloaded file type: $FILETYPE"

        if echo "$FILETYPE" | grep -q "xar"; then
            echo "File is a .pkg (xar archive), expanding..."
            rm -rf "$NDI_EXPANDED"
            pkgutil --expand "$NDI_DMG" "$NDI_EXPANDED"
            cd "$NDI_EXPANDED"
            if [ -d "NDI SDK for Apple.pkg" ]; then
                cat "NDI SDK for Apple.pkg/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
            elif ls *.pkg 1>/dev/null 2>&1; then
                for pkg in *.pkg; do
                    if [ -f "$pkg/Payload" ]; then
                        cat "$pkg/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
                    fi
                done
            fi
            cd "$PROJECT_DIR"
        elif echo "$FILETYPE" | grep -q "Disk Image\|ISO"; then
            echo "File is a DMG, mounting..."
            mkdir -p "$NDI_MOUNT"
            hdiutil attach "$NDI_DMG" -mountpoint "$NDI_MOUNT" -nobrowse -quiet 2>/dev/null || true
            INNER_PKG=$(find "$NDI_MOUNT" -name "*.pkg" -maxdepth 2 2>/dev/null | head -1)
            if [ -n "$INNER_PKG" ]; then
                echo "Found inner pkg: $INNER_PKG"
                pkgutil --expand "$INNER_PKG" "$NDI_EXPANDED"
                cd "$NDI_EXPANDED"
                for pkg_dir in *.pkg; do
                    if [ -f "$pkg_dir/Payload" ]; then
                        cat "$pkg_dir/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
                    fi
                done
                cd "$PROJECT_DIR"
            else
                cp -R "$NDI_MOUNT/"* "$NDI_EXPANDED/" 2>/dev/null || true
            fi
            hdiutil detach "$NDI_MOUNT" -quiet 2>/dev/null || true
        else
            echo -e "${YELLOW}Downloaded file is not a recognized package format.${NC}"
            echo "Trying alternative NDI SDK URLs..."

            for URL in \
                "https://downloads.ndi.tv/SDK/NDI_SDK_Mac/Install_NDI_SDK_v6_Apple.pkg" \
                "https://downloads.ndi.tv/SDK/NDI_SDK_v6_Apple/Install_NDI_SDK_v6_Apple.pkg" \
                "https://downloads.ndi.tv/SDK/NdiSdkApple/Install_NDI_SDK_v6_Apple.pkg"; do
                echo "  Trying: $URL"
                curl -k -L -o "$NDI_DMG" "$URL" 2>/dev/null || continue
                FILETYPE=$(file "$NDI_DMG" 2>/dev/null || echo "unknown")
                if echo "$FILETYPE" | grep -q "xar"; then
                    echo "  Got valid pkg from $URL"
                    rm -rf "$NDI_EXPANDED"
                    pkgutil --expand "$NDI_DMG" "$NDI_EXPANDED"
                    cd "$NDI_EXPANDED"
                    for pkg_dir in *.pkg; do
                        if [ -f "$pkg_dir/Payload" ]; then
                            cat "$pkg_dir/Payload" | gunzip -dc | cpio -i 2>/dev/null || true
                        fi
                    done
                    cd "$PROJECT_DIR"
                    break
                fi
            done
        fi

        if [ ! -d "$NDI_SRC/include" ]; then
            FOUND_INCLUDE=$(find "$NDI_EXPANDED" -name "Processing.NDI.Lib.h" -type f 2>/dev/null | head -1)
            if [ -n "$FOUND_INCLUDE" ]; then
                FOUND_DIR=$(dirname "$FOUND_INCLUDE")
                FOUND_ROOT=$(dirname "$FOUND_DIR")
                echo "Found NDI headers at: $FOUND_DIR"
                mkdir -p "$NDI_SRC"
                cp -R "$FOUND_ROOT/"* "$NDI_SRC/" 2>/dev/null || true
            fi
        fi
    else
        echo "NDI SDK already extracted, reusing."
    fi

    if [ ! -d "$NDI_SRC/include" ]; then
        echo ""
        echo -e "${YELLOW}Could not automatically download NDI SDK.${NC}"
        echo "Please manually download the NDI SDK for Apple from:"
        echo "  https://ndi.video/for-developers/ndi-sdk/"
        echo ""
        echo "Then extract it and re-run with NDI_SDK_PATH set:"
        echo "  export NDI_SDK_PATH=/path/to/NDI\\ SDK\\ for\\ Apple"
        echo "  ./scripts/setup-mac.sh"
        echo ""
        echo "Or place the extracted SDK at: $NDI_SRC"
        echo "  (with include/ and lib/ subdirectories)"
        echo ""

        if [ -n "$NDI_SDK_PATH" ] && [ -d "$NDI_SDK_PATH/include" ]; then
            echo "Using NDI_SDK_PATH=$NDI_SDK_PATH"
            NDI_SRC="$NDI_SDK_PATH"
        else
            echo -e "${YELLOW}Continuing without NDI support. NDI features will be disabled.${NC}"
            echo ""
        fi
    fi

    if [ -d "$NDI_SRC/include" ]; then
        mkdir -p "$GRANDIOSE_DIR/ndi/include"

        ARCH=$(uname -m)
        if [ "$ARCH" = "arm64" ]; then
            NDI_LIB_DEST="$GRANDIOSE_DIR/ndi/lib/mac-a64"
        else
            NDI_LIB_DEST="$GRANDIOSE_DIR/ndi/lib/mac-x64"
        fi
        mkdir -p "$NDI_LIB_DEST"

        cp -R "$NDI_SRC/include/"* "$GRANDIOSE_DIR/ndi/include/"

        NDI_DYLIB=""
        for candidate in \
            "$NDI_SRC/lib/macOS/libndi.dylib" \
            "$NDI_SRC/lib/macos/libndi.dylib" \
            "$NDI_SRC/lib/mac-a64/libndi.dylib" \
            "$NDI_SRC/lib/mac-x64/libndi.dylib"; do
            if [ -f "$candidate" ]; then
                NDI_DYLIB="$candidate"
                break
            fi
        done

        if [ -z "$NDI_DYLIB" ]; then
            NDI_DYLIB=$(find "$NDI_SRC" -name "libndi.dylib" -type f 2>/dev/null | head -1)
        fi

        if [ -n "$NDI_DYLIB" ]; then
            echo "Found libndi.dylib at: $NDI_DYLIB"
            cp "$NDI_DYLIB" "$NDI_LIB_DEST/libndi.dylib"
            echo "Copied to: $NDI_LIB_DEST/libndi.dylib"
        else
            echo -e "${YELLOW}WARNING: libndi.dylib not found in NDI SDK${NC}"
            find "$NDI_SRC/lib" -type f 2>/dev/null | head -20
        fi

        echo "Building grandiose..."
        cd "$GRANDIOSE_DIR"
        PYTHON="$PYTHON_PATH" npx node-gyp rebuild
        cd "$PROJECT_DIR"
    fi
fi
echo ""

echo "--------------------------------------------"
echo -e "${GREEN}[4b/7]${NC} Checking FFmpeg..."
echo "--------------------------------------------"
if command -v ffmpeg &>/dev/null; then
    echo -e "${GREEN}FFmpeg already installed${NC}"
else
    echo "FFmpeg not found. Installing via Homebrew..."
    if command -v brew &>/dev/null; then
        brew install ffmpeg
    else
        echo -e "${YELLOW}Homebrew not found. Install FFmpeg manually: brew install ffmpeg${NC}"
    fi
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
    echo -e "${YELLOW}  ⚠ Grandiose NOT built (NDI features unavailable)${NC}"
fi

if [ -d "$ELECTRON_APP" ]; then
    echo -e "${GREEN}  ✓ Electron.app present${NC}"
else
    echo -e "${RED}  ✗ Electron.app NOT found${NC}"
    PASS=false
fi

FRAMEWORK_LINK="$ELECTRON_APP/Contents/Frameworks/ZoomSDK.framework"
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
    echo -e "${YELLOW}  Setup finished with issues above.${NC}"
    echo "============================================"
fi
echo ""
