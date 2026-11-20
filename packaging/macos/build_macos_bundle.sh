#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
APP_NAME="CalculiX GraphiX GLFW.app"
DIST_DIR="$REPO_ROOT/dist"
APP_DIR="$DIST_DIR/$APP_NAME"

echo "=== Building CalculiX GraphiX GLFW macOS App Bundle ==="

mkdir -p "$DIST_DIR"
rm -rf "$APP_DIR"

# 1. Compile standalone static-GLFW binary if needed
if [ ! -f "$REPO_ROOT/bin/cgx_glfw" ]; then
    echo "Compiling cgx_glfw binary with static GLFW..."
    cmake -S "$REPO_ROOT" -B "$REPO_ROOT/build" -DCMAKE_BUILD_TYPE=Release -DSTATIC_GLFW=ON
    cmake --build "$REPO_ROOT/build" --config Release --parallel
fi

# Verify binary does not depend on Homebrew GLFW dylibs unless explicitly allowed
if [ "${ALLOW_DYNAMIC_GLFW:-0}" != "1" ] && otool -L "$REPO_ROOT/bin/cgx_glfw" | grep -qE "libglfw|/opt/homebrew"; then
    echo "ERROR: $REPO_ROOT/bin/cgx_glfw links against Homebrew/dynamic libglfw."
    echo "Rebuild with: cmake -B build -DCMAKE_BUILD_TYPE=Release -DSTATIC_GLFW=ON && cmake --build build --parallel"
    exit 1
fi

# 2. Compile AppleScript droplet (handles Finder AppleEvents odoc natively)
osacompile -o "$APP_DIR" "$SCRIPT_DIR/launcher.applescript"

mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources/Licenses"

# 3. Copy real binary as cgx_glfw_bin
cp "$REPO_ROOT/bin/cgx_glfw" "$APP_DIR/Contents/MacOS/cgx_glfw_bin"
chmod +x "$APP_DIR/Contents/MacOS/cgx_glfw_bin"

# 4. Copy customized Info.plist
cp "$SCRIPT_DIR/Info.plist" "$APP_DIR/Contents/Info.plist"

# 5. Copy AppIcon.icns
cp "$SCRIPT_DIR/AppIcon.icns" "$APP_DIR/Contents/Resources/AppIcon.icns"

# 6. Bundle licenses
cp "$REPO_ROOT/cgx_2.23/COPYING" "$APP_DIR/Contents/Resources/Licenses/COPYING"
cp "$REPO_ROOT/THIRD_PARTY_LICENSES.md" "$APP_DIR/Contents/Resources/Licenses/THIRD_PARTY_LICENSES.md"

# 7. Ad-hoc codesign
codesign --force --deep --sign - "$APP_DIR" || true

echo ">>> Successfully built macOS App Bundle: $APP_DIR <<<"
