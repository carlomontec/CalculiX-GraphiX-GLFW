#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

RAW_ARCH="${1:-$(uname -m)}"
case "$RAW_ARCH" in
    aarch64|arm64)
        ARCH="arm64"
        ;;
    x86_64|amd64)
        ARCH="x86_64"
        ;;
    *)
        ARCH="$RAW_ARCH"
        ;;
esac

DIST_DIR="$REPO_ROOT/dist"
PKG_NAME="cgx_glfw-linux-${ARCH}"
TARBALL_PATH="$DIST_DIR/${PKG_NAME}.tar.gz"
STAGE_PARENT="$DIST_DIR/.stage_${PKG_NAME}"
STAGE_DIR="$STAGE_PARENT/${PKG_NAME}"

SRC_BIN="$DIST_DIR/${PKG_NAME}"
if [ ! -f "$SRC_BIN" ]; then
    SRC_BIN="$REPO_ROOT/bin/cgx_glfw"
fi
if [ ! -f "$SRC_BIN" ]; then
    echo "Error: Binary not found at $DIST_DIR/${PKG_NAME} or $REPO_ROOT/bin/cgx_glfw" >&2
    exit 1
fi

rm -rf "$STAGE_PARENT" "$TARBALL_PATH"
mkdir -p "$STAGE_DIR/bin" \
         "$STAGE_DIR/share/applications" \
         "$STAGE_DIR/share/mime/packages" \
         "$STAGE_DIR/share/icons/hicolor/256x256/apps" \
         "$STAGE_DIR/licenses"

cp "$SRC_BIN" "$STAGE_DIR/bin/cgx_glfw"
chmod +x "$STAGE_DIR/bin/cgx_glfw"
ln -s "cgx_glfw" "$STAGE_DIR/bin/cgx"

cp "$SCRIPT_DIR/cgx-open" "$STAGE_DIR/bin/cgx-open"
chmod +x "$STAGE_DIR/bin/cgx-open"

cp "$SCRIPT_DIR/calculix-cgx.desktop" "$STAGE_DIR/share/applications/calculix-cgx.desktop"
cp "$SCRIPT_DIR/calculix-cgx.xml" "$STAGE_DIR/share/mime/packages/calculix-cgx.xml"
cp "$SCRIPT_DIR/calculix-cgx.png" "$STAGE_DIR/share/icons/hicolor/256x256/apps/calculix-cgx.png"

cp "$SCRIPT_DIR/install-desktop.sh" "$STAGE_DIR/install-desktop.sh"
chmod +x "$STAGE_DIR/install-desktop.sh"

cp "$REPO_ROOT/cgx_2.23/COPYING" "$STAGE_DIR/licenses/COPYING"
cp "$REPO_ROOT/THIRD_PARTY_LICENSES.md" "$STAGE_DIR/licenses/THIRD_PARTY_LICENSES.md"

tar -C "$STAGE_PARENT" -czf "$TARBALL_PATH" "$PKG_NAME"
rm -rf "$STAGE_PARENT"

echo ">>> Successfully created Linux tarball: $TARBALL_PATH <<<"
