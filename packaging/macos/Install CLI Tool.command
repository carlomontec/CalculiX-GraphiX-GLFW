#!/bin/bash
# ==============================================================================
# CalculiX GraphiX (GLFW Edition) - Command Line Tool Installer (User Space)
# ==============================================================================

clear
echo "========================================================================"
echo "         CalculiX GraphiX GLFW - CLI Installation Helper                "
echo "========================================================================"
echo ""

APP_PATH="/Applications/CalculiX GraphiX GLFW.app"
BIN_PATH="$APP_PATH/Contents/MacOS/cgx_glfw_bin"
LOCAL_BIN="$HOME/.local/bin"

if [ ! -d "$APP_PATH" ]; then
    echo "CalculiX GraphiX GLFW.app was not found in /Applications!"
    echo ""
    echo "   Please drag 'CalculiX GraphiX GLFW.app' into your Applications"
    echo "   folder first, then run this installer again."
    echo ""
    read -r -p "Press [Enter] to exit..."
    exit 1
fi

echo "This script will create user-space command line shortcuts in $LOCAL_BIN:"
echo "   - 'cgx'       -> $BIN_PATH"
echo "   - 'cgx_glfw'  -> $BIN_PATH"
echo ""
echo "This allows you to type 'cgx model.frd' directly from any terminal window"
echo "without administrator privileges."
echo ""

mkdir -p "$LOCAL_BIN"
ln -sf "$BIN_PATH" "$LOCAL_BIN/cgx"
ln -sf "$BIN_PATH" "$LOCAL_BIN/cgx_glfw"

# Ensure ~/.local/bin is on PATH in user shell profile
CURRENT_SHELL="$(basename "${SHELL:-zsh}")"
if [ "$CURRENT_SHELL" = "zsh" ]; then
    SHELL_PROFILE="$HOME/.zshrc"
elif [ "$CURRENT_SHELL" = "bash" ]; then
    SHELL_PROFILE="$HOME/.bash_profile"
else
    SHELL_PROFILE="$HOME/.profile"
fi

if [[ ":$PATH:" != *":$LOCAL_BIN:"* ]]; then
    if [ ! -f "$SHELL_PROFILE" ] || ! grep -q "$LOCAL_BIN" "$SHELL_PROFILE"; then
        {
            echo ""
            echo "# CalculiX GraphiX PATH"
            echo "export PATH=\"$LOCAL_BIN:\$PATH\""
        } >> "$SHELL_PROFILE"
        echo "Added $LOCAL_BIN to $SHELL_PROFILE"
    fi
fi

# Verify installation
if [ -x "$LOCAL_BIN/cgx" ] && [ -x "$LOCAL_BIN/cgx_glfw" ]; then
    echo ""
    echo "========================================================================"
    echo "  Installation Successful!"
    echo "========================================================================"
    echo ""
    echo "  Open a new Terminal window (or run: source \"$SHELL_PROFILE\") and run:"
    echo "     cgx file.frd"
    echo "     cgx_glfw file.frd"
    echo ""
else
    echo ""
    echo "Failed to create symlinks in $LOCAL_BIN."
    echo "   You can manually add this alias to your ~/.zshrc file:"
    echo "   alias cgx=\"'$BIN_PATH'\""
    echo ""
fi

read -r -p "Press [Enter] to close this window..."
