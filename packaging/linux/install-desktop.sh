#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${HOME}/.local/bin"
DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
APP_DIR="${DATA_HOME}/applications"
MIME_DIR="${DATA_HOME}/mime"
ICON_DIR="${DATA_HOME}/icons/hicolor/256x256/apps"

echo "=== Installing CalculiX GraphiX (GLFW Edition) for User Space ==="

mkdir -p "$BIN_DIR" "$APP_DIR" "$MIME_DIR/packages" "$ICON_DIR"

cp -f "$SCRIPT_DIR/bin/cgx_glfw" "$BIN_DIR/cgx_glfw"
chmod +x "$BIN_DIR/cgx_glfw"
ln -sf "cgx_glfw" "$BIN_DIR/cgx"

cp -f "$SCRIPT_DIR/bin/cgx-open" "$BIN_DIR/cgx-open"
chmod +x "$BIN_DIR/cgx-open"

cp -f "$SCRIPT_DIR/share/icons/hicolor/256x256/apps/calculix-cgx.png" "$ICON_DIR/calculix-cgx.png"
cp -f "$SCRIPT_DIR/share/mime/packages/calculix-cgx.xml" "$MIME_DIR/packages/calculix-cgx.xml"

sed "s|^Exec=.*|Exec=${BIN_DIR}/cgx-open %f|" \
    "$SCRIPT_DIR/share/applications/calculix-cgx.desktop" \
    > "$APP_DIR/calculix-cgx.desktop"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APP_DIR" 2>/dev/null || true
fi
if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database "$MIME_DIR" 2>/dev/null || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "${DATA_HOME}/icons/hicolor" 2>/dev/null || true
fi

echo "[OK] Installed binary:  $BIN_DIR/cgx_glfw (alias: $BIN_DIR/cgx)"
echo "[OK] Installed desktop: $APP_DIR/calculix-cgx.desktop"
echo "[OK] Installed MIME:    $MIME_DIR/packages/calculix-cgx.xml"
if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo ""
    echo "Note: Add $BIN_DIR to your PATH if it is not already configured:"
    echo "  export PATH=\"$BIN_DIR:\$PATH\""
fi
