#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
APP_NAME="CalculiX GraphiX GLFW.app"
DIST_DIR="$REPO_ROOT/dist"
APP_DIR="$DIST_DIR/$APP_NAME"
DMG_NAME="CalculiX-GraphiX-GLFW-macOS-arm64.dmg"
DMG_PATH="$DIST_DIR/$DMG_NAME"
TEMP_DMG_DIR="$DIST_DIR/dmg_temp"

echo "=== Creating macOS Drag-and-Drop DMG Installer ==="

# Build bundle first
"$SCRIPT_DIR/build_macos_bundle.sh"

rm -rf "$TEMP_DMG_DIR" "$DMG_PATH"
mkdir -p "$TEMP_DMG_DIR"

# Copy App to temp folder
cp -R "$APP_DIR" "$TEMP_DMG_DIR/"

# Copy CLI installation helper
cp "$SCRIPT_DIR/Install CLI Tool.command" "$TEMP_DMG_DIR/"
chmod +x "$TEMP_DMG_DIR/Install CLI Tool.command"

# Create symlink to /Applications
ln -s /Applications "$TEMP_DMG_DIR/Applications"

# Create DMG with hdiutil
hdiutil create -volname "CalculiX GraphiX GLFW" \
               -srcfolder "$TEMP_DMG_DIR" \
               -ov -format UDZO \
               "$DMG_PATH"

rm -rf "$TEMP_DMG_DIR"

echo ">>> Successfully created DMG: $DMG_PATH <<<"
