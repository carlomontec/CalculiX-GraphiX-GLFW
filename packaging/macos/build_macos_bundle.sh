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

# 1. Compile binary if needed
if [ ! -f "$REPO_ROOT/bin/cgx_glfw" ]; then
    echo "Compiling cgx_glfw binary..."
    make -C "$REPO_ROOT/cgx_2.23/src" -f Makefile.glfw -j4
fi

# 2. Compile AppleScript droplet (handles Finder AppleEvents odoc natively)
osacompile -o "$APP_DIR" "$SCRIPT_DIR/launcher.applescript"

mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources"

# 3. Copy real binary as cgx_glfw_bin
cp "$REPO_ROOT/bin/cgx_glfw" "$APP_DIR/Contents/MacOS/cgx_glfw_bin"
chmod +x "$APP_DIR/Contents/MacOS/cgx_glfw_bin"

# 4. Copy customized Info.plist
cp "$SCRIPT_DIR/Info.plist" "$APP_DIR/Contents/Info.plist"

# 5. Copy AppIcon.icns
cp "$SCRIPT_DIR/AppIcon.icns" "$APP_DIR/Contents/Resources/AppIcon.icns"

# 6. Ad-hoc codesign
codesign --force --deep --sign - "$APP_DIR" || true

echo ">>> Successfully built macOS App Bundle: $APP_DIR <<<"
