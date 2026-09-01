#!/usr/bin/env bash
# ==============================================================================
# CalculiX GraphiX (GLFW Edition) — Universal Uninstaller
#
# 1-Liner Usage:
#   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/uninstall.sh)"
#
# Local Usage:
#   ./uninstall.sh
# ==============================================================================

set -e

# Color codes
BOLD="\033[1m"
GREEN="\033[0;32m"
BLUE="\033[0;34m"
YELLOW="\033[1;33m"
RED="\033[0;31m"
NC="\033[0m" # No Color

# Helper for interactive reading
prompt_read() {
    local prompt_msg="$1"
    local default_val="$2"
    local var_name="$3"
    local input_val=""

    if [ -n "$NON_INTERACTIVE" ]; then
        eval "$var_name=\"$default_val\""
        return 0
    fi

    if [ -t 0 ]; then
        read -r -p "$prompt_msg" input_val
    elif [ -c /dev/tty ]; then
        read -r -p "$prompt_msg" input_val < /dev/tty
    else
        input_val="$default_val"
    fi

    input_val="${input_val:-$default_val}"
    eval "$var_name=\"$input_val\""
}

echo -e "${BOLD}${BLUE}=====================================================${NC}"
echo -e "${BOLD}${BLUE}   CalculiX GraphiX (GLFW Edition) — Uninstaller     ${NC}"
echo -e "${BOLD}${BLUE}=====================================================${NC}"

# Target install directory
DEFAULT_INSTALL_DIR="${HOME}/.local/bin"
INSTALL_DIR="${CGX_INSTALL_DIR:-$DEFAULT_INSTALL_DIR}"
SOURCE_DIR="${HOME}/.cgx"
GLOBAL_BIN="/usr/local/bin/cgx_glfw"

NON_INTERACTIVE=""
KEEP_SOURCE=""
CUSTOM_DIR_SPECIFIED=""

# CLI Argument parsing
while [[ $# -gt 0 ]]; do
    case "$1" in
        -y|--yes)
            NON_INTERACTIVE="1"
            shift
            ;;
        --keep-source)
            KEEP_SOURCE="1"
            shift
            ;;
        --prefix|--bin-dir|--dir)
            if [ -n "$2" ]; then
                INSTALL_DIR="$2"
                INSTALL_DIR="${INSTALL_DIR/#\~/$HOME}"
                CUSTOM_DIR_SPECIFIED="1"
                shift 2
            else
                echo -e "${RED}Error: $1 requires a directory argument.${NC}"
                exit 1
            fi
            ;;
        --help|-h)
            echo "Usage: uninstall.sh [OPTION]"
            echo "Options:"
            echo "  -y, --yes           Non-interactive mode (uninstall without prompting)"
            echo "  --keep-source       Keep cached source build directory (~/.cgx)"
            echo "  --prefix, --dir     Specify custom binary installation directory (Default: ~/.local/bin)"
            echo "  --help, -h          Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

TARGET_BIN="${INSTALL_DIR}/cgx_glfw"

# Detect installed components
FOUND_ITEMS=0
echo -e "\n${BOLD}--> Detecting installed components...${NC}"

if [ -f "${TARGET_BIN}" ] || [ -L "${TARGET_BIN}" ]; then
    echo -e "  [Found] Binary: ${BOLD}${TARGET_BIN}${NC}"
    FOUND_ITEMS=$((FOUND_ITEMS + 1))
fi

if [ -f "${GLOBAL_BIN}" ] || [ -L "${GLOBAL_BIN}" ]; then
    echo -e "  [Found] Global Link: ${BOLD}${GLOBAL_BIN}${NC}"
    FOUND_ITEMS=$((FOUND_ITEMS + 1))
fi

if [ -d "${SOURCE_DIR}" ]; then
    echo -e "  [Found] Build Source Cache: ${BOLD}${SOURCE_DIR}${NC}"
    FOUND_ITEMS=$((FOUND_ITEMS + 1))
fi

if [ "$FOUND_ITEMS" -eq 0 ]; then
    echo -e "${YELLOW}No installed CGX GLFW components found in standard locations.${NC}"
    if [ -z "$NON_INTERACTIVE" ]; then
        prompt_read "Check a custom directory for 'cgx_glfw'? [y/N] " "N" CHECK_CUSTOM
        if [[ "$CHECK_CUSTOM" =~ ^[Yy]$ ]]; then
            prompt_read "Enter custom directory path: " "${DEFAULT_INSTALL_DIR}" CUSTOM_PATH
            CUSTOM_PATH="${CUSTOM_PATH/#\~/$HOME}"
            TARGET_BIN="${CUSTOM_PATH}/cgx_glfw"
            if [ -f "${TARGET_BIN}" ] || [ -L "${TARGET_BIN}" ]; then
                echo -e "  [Found] Binary: ${BOLD}${TARGET_BIN}${NC}"
                FOUND_ITEMS=1
            else
                echo -e "${RED}Binary not found at ${TARGET_BIN}. Exiting.${NC}"
                exit 0
            fi
        else
            exit 0
        fi
    else
        exit 0
    fi
fi

# Confirmation prompt
if [ -z "$NON_INTERACTIVE" ]; then
    echo ""
    prompt_read "Are you sure you want to uninstall CalculiX GraphiX (GLFW)? [y/N] " "N" CONFIRM
    if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}Uninstall cancelled.${NC}"
        exit 0
    fi
fi

# 1. Remove binary
if [ -f "${TARGET_BIN}" ] || [ -L "${TARGET_BIN}" ]; then
    rm -f "${TARGET_BIN}"
    echo -e "${GREEN}[OK] Removed binary: ${TARGET_BIN}${NC}"
fi

# 2. Remove /usr/local/bin symlink
if [ -f "${GLOBAL_BIN}" ] || [ -L "${GLOBAL_BIN}" ]; then
    if [ -w "/usr/local/bin" ]; then
        rm -f "${GLOBAL_BIN}"
        echo -e "${GREEN}[OK] Removed global link: ${GLOBAL_BIN}${NC}"
    else
        echo "Requesting sudo to remove ${GLOBAL_BIN}..."
        sudo rm -f "${GLOBAL_BIN}" 2>/dev/null || true
        echo -e "${GREEN}[OK] Removed global link: ${GLOBAL_BIN}${NC}"
    fi
fi

# 3. Clean source cache directory
if [ -d "${SOURCE_DIR}" ]; then
    REMOVE_SRC="Y"
    if [ -z "$NON_INTERACTIVE" ]; then
        echo ""
        prompt_read "Remove source repository build cache (~/.cgx)? [Y/n] " "Y" REMOVE_SRC
    elif [ -n "$KEEP_SOURCE" ]; then
        REMOVE_SRC="N"
    fi

    if [[ "$REMOVE_SRC" =~ ^[Yy]$ ]]; then
        rm -rf "${SOURCE_DIR}"
        echo -e "${GREEN}[OK] Removed build source cache: ${SOURCE_DIR}${NC}"
    else
        echo -e "${YELLOW}Preserved build source cache at: ${SOURCE_DIR}${NC}"
    fi
fi

# 4. Clean PATH entries in shell config
clean_shell_path() {
    local profile="$1"
    if [ -f "$profile" ] && grep -q "# CalculiX GraphiX PATH" "$profile"; then
        sed -i.cgxbak '/# CalculiX GraphiX PATH/d' "$profile" 2>/dev/null || true
        sed -i.cgxbak "/export PATH=.*\.local\/bin:\$PATH.*/d" "$profile" 2>/dev/null || true
        rm -f "${profile}.cgxbak"
        echo -e "${GREEN}[OK] Cleaned PATH entry from ${profile}${NC}"
    fi
}

clean_shell_path "${HOME}/.bashrc"
clean_shell_path "${HOME}/.zshrc"
clean_shell_path "${HOME}/.bash_profile"
clean_shell_path "${HOME}/.profile"

echo -e "\n${BOLD}${GREEN}=====================================================${NC}"
echo -e "${BOLD}${GREEN}   Uninstall Complete!                            ${NC}"
echo -e "${BOLD}${GREEN}=====================================================${NC}"
