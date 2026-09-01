#!/bin/bash
# ==============================================================================
# CalculiX GraphiX (GLFW Edition) - Command Line Tool Installer
# ==============================================================================

clear
echo "========================================================================"
echo "         CalculiX GraphiX GLFW - CLI Installation Helper                "
echo "========================================================================"
echo ""

APP_PATH="/Applications/CalculiX GraphiX GLFW.app"
BIN_PATH="$APP_PATH/Contents/MacOS/cgx_glfw_bin"

if [ ! -d "$APP_PATH" ]; then
    echo "⚠️  CalculiX GraphiX GLFW.app was not found in /Applications!"
    echo ""
    echo "   Please drag 'CalculiX GraphiX GLFW.app' into your Applications"
    echo "   folder first, then run this installer again."
    echo ""
    read -p "Press [Enter] to exit..."
    exit 1
fi

echo "This script will create command line shortcuts in /usr/local/bin:"
echo "   • 'cgx'       -> $BIN_PATH"
echo "   • 'cgx_glfw'  -> $BIN_PATH"
echo ""
echo "This allows you to type 'cgx model.frd' directly from any terminal window."
echo ""

# Request administrator privileges if /usr/local/bin requires sudo
if [ ! -d "/usr/local/bin" ]; then
    echo "Creating /usr/local/bin directory..."
    sudo mkdir -p /usr/local/bin
fi

echo "Creating symlinks (may require administrator password)..."
sudo ln -sf "$BIN_PATH" /usr/local/bin/cgx
sudo ln -sf "$BIN_PATH" /usr/local/bin/cgx_glfw

# Verify installation
if [ -x "/usr/local/bin/cgx" ] && [ -x "/usr/local/bin/cgx_glfw" ]; then
    echo ""
    echo "========================================================================"
    echo "  ✅ Installation Successful!"
    echo "========================================================================"
    echo ""
    echo "  You can now open any Terminal window and run:"
    echo "     cgx file.frd"
    echo "     cgx_glfw file.frd"
    echo ""
else
    echo ""
    echo "⚠️  Failed to create symlinks in /usr/local/bin."
    echo "   You can manually add this alias to your ~/.zshrc file:"
    echo "   alias cgx=\"'$BIN_PATH'\""
    echo ""
fi

read -p "Press [Enter] to close this window..."
