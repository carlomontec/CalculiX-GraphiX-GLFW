#!/usr/bin/env bash
# ==============================================================================
# CalculiX GraphiX (GLFW Edition) — Universal 1-Liner Installer
#
# 1-Liner Usage:
#   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/install.sh)"
#
# Supports:
#   - macOS (Apple Silicon arm64 & Intel x86_64)
#   - Linux (x86_64 & ARM64)
#   - Windows (MSYS2 / MinGW64 / Git Bash x86_64)
# ==============================================================================

set -e

REPO="carlomontec/CalculiX-GraphiX-GLFW"
GITHUB_REPO_URL="https://github.com/${REPO}.git"
DEFAULT_RELEASE_URL="https://github.com/${REPO}/releases/latest/download"
RELEASE_URL="${CGX_DOWNLOAD_BASE_URL:-$DEFAULT_RELEASE_URL}"

# Color codes
BOLD="\033[1m"
GREEN="\033[0;32m"
BLUE="\033[0;34m"
YELLOW="\033[1;33m"
RED="\033[0;31m"
NC="\033[0m" # No Color

# Helper for interactive reading (handles stdin pipe gracefully)
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
echo -e "${BOLD}${BLUE}   CalculiX GraphiX (GLFW Edition) — Installer       ${NC}"
echo -e "${BOLD}${BLUE}=====================================================${NC}"

# Target install directory (~/.local/bin default, strictly in user space)
DEFAULT_INSTALL_DIR="${HOME}/.local/bin"
INSTALL_DIR="${CGX_INSTALL_DIR:-$DEFAULT_INSTALL_DIR}"

# Detect OS and Architecture
OS_RAW="$(uname -s)"
ARCH="$(uname -m)"
BIN_EXT=""
IS_WINDOWS=0

if [[ "${OS_RAW}" =~ ^MINGW ]] || [[ "${OS_RAW}" =~ ^MSYS ]] || [[ "${OS_RAW}" =~ ^CYGWIN ]] || [ "${OS_RAW}" = "Windows_NT" ]; then
    OS="Windows"
    IS_WINDOWS=1
    BIN_EXT=".exe"
elif [ "${OS_RAW}" = "Darwin" ]; then
    OS="Darwin"
elif [ "${OS_RAW}" = "Linux" ]; then
    OS="Linux"
else
    OS="${OS_RAW}"
fi

echo -e "Detected Platform: ${BOLD}${GREEN}${OS} (${ARCH})${NC}"

# Map to GitHub Release asset name
BINARY_NAME=""
if [ "${OS}" = "Darwin" ]; then
    if [ "${ARCH}" = "arm64" ]; then
        BINARY_NAME="cgx_glfw-macos-arm64"
    else
        echo -e "${YELLOW}Notice: macOS Intel (${ARCH}) pre-built binaries are not provided. Falling back to local compilation...${NC}"
        BINARY_NAME=""
    fi
elif [ "${OS}" = "Linux" ]; then
    if [ "${ARCH}" = "x86_64" ]; then
        BINARY_NAME="cgx_glfw-linux-x86_64"
    elif [ "${ARCH}" = "aarch64" ] || [ "${ARCH}" = "arm64" ]; then
        BINARY_NAME="cgx_glfw-linux-arm64"
    else
        echo -e "${RED}Error: Linux '${ARCH}' architecture is not supported yet (future work).${NC}"
        exit 1
    fi
elif [ "${OS}" = "Windows" ]; then
    if [ "${ARCH}" = "x86_64" ]; then
        BINARY_NAME="cgx_glfw-windows-x86_64.exe"
    else
        echo -e "${YELLOW}Notice: Windows 32-bit pre-built binary is not provided. Falling back to local compilation...${NC}"
        BINARY_NAME=""
    fi
else
    echo -e "${RED}Error: Unsupported operating system '${OS_RAW}'.${NC}"
    exit 1
fi

# Function: Ensure Homebrew on macOS
ensure_homebrew_macos() {
    if command -v brew &>/dev/null; then
        return 0
    fi

    # Check common paths
    if [ -x "/opt/homebrew/bin/brew" ]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
        return 0
    elif [ -x "/usr/local/bin/brew" ]; then
        eval "$(/usr/local/bin/brew shellenv)"
        return 0
    fi

    echo -e "\n${BOLD}${YELLOW}--> Homebrew package manager was not found on your Mac.${NC}"
    echo -e "Homebrew (https://brew.sh) is the standard, free open-source package manager for macOS."
    echo -e "It safely provides the GLFW windowing libraries and graphics tools needed by modern apps.\n"

    prompt_read "Would you like to install Homebrew now automatically? [Y/n] " "Y" INSTALL_BREW

    if [[ "$INSTALL_BREW" =~ ^[Yy]$ ]]; then
        echo -e "\n${BOLD}--> Running official Homebrew installer...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

        if [ -x "/opt/homebrew/bin/brew" ]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        elif [ -x "/usr/local/bin/brew" ]; then
            eval "$(/usr/local/bin/brew shellenv)"
        fi

        if command -v brew &>/dev/null; then
            echo -e "${GREEN}[OK] Homebrew successfully installed!${NC}"
        else
            echo -e "${RED}Error: Homebrew was installed but 'brew' is not yet in PATH.${NC}"
            exit 1
        fi
    else
        echo -e "\n${YELLOW}Homebrew is required for GLFW on macOS. Install it at https://brew.sh and re-run this script.${NC}"
        exit 1
    fi
}

# Function: Install Runtime Dependencies
install_runtime_deps() {
    echo -e "\n${BOLD}--> Checking runtime dependencies...${NC}"
    if [ "${OS}" = "Darwin" ]; then
        ensure_homebrew_macos
        if brew list glfw &>/dev/null; then
            echo -e "${GREEN}[OK] GLFW runtime is already installed.${NC}"
        else
            echo "Installing glfw via Homebrew..."
            brew install glfw
        fi
        if command -v ffmpeg &>/dev/null; then
            echo -e "${GREEN}[OK] ffmpeg video tools available for MP4 recording.${NC}"
        else
            echo -e "${BLUE}Tip: Install ffmpeg via 'brew install ffmpeg' to enable MP4 video recording.${NC}"
        fi
    elif [ "${OS}" = "Linux" ]; then
        if ldconfig -p 2>/dev/null | grep -q "libglfw\.so" || [ -f /lib64/libglfw.so.3 ] || [ -f /usr/lib/libglfw.so.3 ] || [ -f /usr/lib/x86_64-linux-gnu/libglfw.so.3 ] || [ -f /home/linuxbrew/.linuxbrew/lib/libglfw.so ]; then
            echo -e "${GREEN}[OK] GLFW runtime library is already installed.${NC}"
            if command -v ffmpeg &>/dev/null; then
                echo -e "${GREEN}[OK] ffmpeg video tools available for MP4 recording.${NC}"
            else
                echo -e "${BLUE}Tip: Install ffmpeg (sudo apt install ffmpeg) to enable MP4 video recording.${NC}"
            fi
            return 0
        fi

        echo "Installing missing runtime libraries (requires sudo)..."
        if command -v apt-get &>/dev/null; then
            sudo apt-get update -y && sudo apt-get install -y libglfw3 libglu1-mesa ffmpeg
        elif command -v dnf &>/dev/null; then
            sudo dnf install -y glfw mesa-libGLU ffmpeg
        elif command -v pacman &>/dev/null; then
            sudo pacman -S --needed --noconfirm glfw-x11 mesa glu ffmpeg
        else
            echo -e "${YELLOW}Please ensure libglfw3 and OpenGL/Mesa runtime libraries are installed.${NC}"
        fi
    elif [ "${OS}" = "Windows" ]; then
        echo -e "${GREEN}[OK] Windows runtime libraries configured.${NC}"
        if command -v ffmpeg &>/dev/null; then
            echo -e "${GREEN}[OK] ffmpeg video tools available for MP4 recording.${NC}"
        else
            echo -e "${BLUE}Tip: Install ffmpeg for Windows (winget install Gyan.FFmpeg) to enable MP4 video recording.${NC}"
        fi
    fi
}

# Function: Install Build Toolchain Dependencies
install_build_deps() {
    echo -e "\n${BOLD}--> Checking build dependencies...${NC}"
    if [ "${OS}" = "Darwin" ]; then
        ensure_homebrew_macos
        if ! command -v cmake &>/dev/null; then
            brew install cmake
        fi
        if ! brew list glfw &>/dev/null; then
            brew install glfw
        fi
        echo -e "${GREEN}[OK] macOS build tools are ready.${NC}"
    elif [ "${OS}" = "Linux" ]; then
        HAS_COMPILER=0
        HAS_GLFW_DEV=0
        HAS_CMAKE=0
        
        if command -v gcc &>/dev/null && command -v g++ &>/dev/null; then
            HAS_COMPILER=1
        fi

        if command -v cmake &>/dev/null; then
            HAS_CMAKE=1
        fi

        if pkg-config --exists glfw3 2>/dev/null || [ -f /usr/include/GLFW/glfw3.h ] || [ -f /home/linuxbrew/.linuxbrew/include/GLFW/glfw3.h ]; then
            HAS_GLFW_DEV=1
        fi

        if [ "$HAS_COMPILER" -eq 1 ] && [ "$HAS_GLFW_DEV" -eq 1 ] && [ "$HAS_CMAKE" -eq 1 ]; then
            echo -e "${GREEN}[OK] Compilers, CMake, and GLFW development headers are ready.${NC}"
            return 0
        fi

        echo "Installing missing build tools (requires sudo)..."
        if command -v apt-get &>/dev/null; then
            sudo apt-get update -y && sudo apt-get install -y cmake build-essential libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev git
        elif command -v dnf &>/dev/null; then
            sudo dnf install -y cmake gcc-c++ make glfw-devel mesa-libGL-devel mesa-libGLU-devel git
        elif command -v pacman &>/dev/null; then
            sudo pacman -S --needed --noconfirm cmake base-devel glfw-x11 mesa glu git
        fi
    elif [ "${OS}" = "Windows" ]; then
        HAS_COMPILER=0
        HAS_CMAKE=0
        if command -v gcc &>/dev/null && command -v g++ &>/dev/null; then
            HAS_COMPILER=1
        fi
        if command -v cmake &>/dev/null; then
            HAS_CMAKE=1
        fi

        if [ "$HAS_COMPILER" -eq 1 ] && [ "$HAS_CMAKE" -eq 1 ]; then
            echo -e "${GREEN}[OK] Windows MinGW-w64 compiler and CMake are ready.${NC}"
            return 0
        fi

        if command -v pacman &>/dev/null; then
            echo "Installing MinGW-w64 toolchain and GLFW via pacman..."
            pacman -S --needed --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-cmake make git
        else
            echo -e "${YELLOW}Notice: pacman package manager not found in current environment.${NC}"
            if [ "$HAS_COMPILER" -eq 0 ] || [ "$HAS_CMAKE" -eq 0 ]; then
                echo -e "${RED}Error: MinGW-w64 GCC and CMake are required to build on Windows.${NC}"
                echo -e "Please install MSYS2 (https://www.msys2.org/) or ensure gcc and cmake are in PATH."
                exit 1
            fi
        fi
    fi
}

# Function: Create User-level Symlink / Alias (cgx for stable, cgx_dev for --head)
create_user_alias() {
    local target_bin="$1"
    
    if [ "$USE_HEAD" = "1" ]; then
        local alias_bin="${INSTALL_DIR}/cgx_dev${BIN_EXT}"
        if [ "$IS_WINDOWS" -eq 1 ]; then
            cp -f "${target_bin}" "${alias_bin}" 2>/dev/null || true
        else
            ln -sf "cgx_glfw${BIN_EXT}" "${alias_bin}" 2>/dev/null || ln -sf "${target_bin}" "${alias_bin}" 2>/dev/null || true
        fi
        echo -e "${GREEN}[OK] Development alias created: ${alias_bin} -> cgx_glfw${BIN_EXT}${NC}"
    else
        local alias_bin="${INSTALL_DIR}/cgx${BIN_EXT}"
        if [ "$IS_WINDOWS" -eq 1 ]; then
            cp -f "${target_bin}" "${alias_bin}" 2>/dev/null || true
        else
            ln -sf "cgx_glfw${BIN_EXT}" "${alias_bin}" 2>/dev/null || ln -sf "${target_bin}" "${alias_bin}" 2>/dev/null || true
        fi
        echo -e "${GREEN}[OK] User alias created: ${alias_bin} -> cgx_glfw${BIN_EXT}${NC}"
    fi
}

# Function: Fast Install via Pre-built Binary
do_fast_install() {
    if [ -z "${BINARY_NAME}" ]; then
        echo -e "${YELLOW}No pre-built binary for ${OS} ${ARCH}. Switching to build from source.${NC}"
        do_build_install
        return
    fi

    install_runtime_deps

    DOWNLOAD_URL="${RELEASE_URL}/${BINARY_NAME}"
    TARGET_BIN="${INSTALL_DIR}/cgx_glfw${BIN_EXT}"

    echo -e "\n${BOLD}--> Fetching pre-compiled binary from:${NC}"
    echo "    ${DOWNLOAD_URL}"
    
    if curl -fSL --progress-bar -o "${TARGET_BIN}" "${DOWNLOAD_URL}"; then
        chmod +x "${TARGET_BIN}"
        if [ "${OS}" = "Darwin" ]; then
            xattr -d com.apple.quarantine "${TARGET_BIN}" 2>/dev/null || true
        fi
        create_user_alias "${TARGET_BIN}"
        echo -e "${GREEN}[OK] Successfully installed: ${TARGET_BIN}${NC}"
    else
        echo -e "${YELLOW}Notice: Pre-compiled binary not found on GitHub Releases yet (${DOWNLOAD_URL}).${NC}"
        echo -e "${BOLD}--> Automatically falling back to local compilation...${NC}"
        do_build_install
        return
    fi
}

# Function: Compile from Source
do_build_install() {
    SRC_ROOT=""
    IS_LOCAL_CLONE=0

    # Check if run inside existing cloned repo
    if [ -f "$(pwd)/CMakeLists.txt" ] || [ -f "$(pwd)/cgx_2.23/src/Makefile.glfw" ]; then
        SRC_ROOT="$(pwd)"
        IS_LOCAL_CLONE=1
        echo -e "\n${BOLD}--> Using existing local repository: ${SRC_ROOT}${NC}"
    elif [ -f "$(dirname "$0")/CMakeLists.txt" ] || [ -f "$(dirname "$0")/cgx_2.23/src/Makefile.glfw" ]; then
        SRC_ROOT="$(cd "$(dirname "$0")" && pwd)"
        IS_LOCAL_CLONE=1
        echo -e "\n${BOLD}--> Using script repository directory: ${SRC_ROOT}${NC}"
    else
        # 1-Liner mode: Prompt version if interactive
        if [ -z "$USE_HEAD" ] && [ -z "$NON_INTERACTIVE" ]; then
            echo -e "\nChoose version to build from source:"
            echo -e "  ${BOLD}1) Stable Release Tag${NC} (Recommended for production) ${YELLOW}[Default]${NC}"
            echo -e "  ${BOLD}2) Latest Development Commit${NC} (--head / --nightly -> alias 'cgx_dev')"
            echo ""
            prompt_read "Select [1/2] (Default: 1): " "1" VERSION_CHOICE
            if [ "$VERSION_CHOICE" = "2" ]; then
                USE_HEAD="1"
            fi
        fi

        # 1-Liner mode: Clone repo into ~/.cgx/CalculiX-GraphiX-GLFW
        SOURCE_DIR="${HOME}/.cgx"
        mkdir -p "${SOURCE_DIR}"
        SRC_ROOT="${SOURCE_DIR}/CalculiX-GraphiX-GLFW"
        
        echo -e "\n${BOLD}--> Fetching CalculiX-GraphiX-GLFW into ${SRC_ROOT}...${NC}"
        if [ -d "${SRC_ROOT}/.git" ]; then
            git -C "${SRC_ROOT}" fetch --all --tags --prune --quiet 2>/dev/null || true
        else
            git clone "${GITHUB_REPO_URL}" "${SRC_ROOT}"
        fi
    fi

    # Only switch branches/tags in 1-liner mode cache, NEVER in a developer's working directory
    if [ "$IS_LOCAL_CLONE" -eq 0 ] && [ -d "${SRC_ROOT}/.git" ]; then
        if [ -n "$USE_HEAD" ]; then
            echo -e "--> Checking out latest '${BOLD}main${NC}' branch (--head / --nightly)..."
            git -C "${SRC_ROOT}" fetch --all --quiet 2>/dev/null || true
            git -C "${SRC_ROOT}" checkout -f main --quiet 2>/dev/null || git -C "${SRC_ROOT}" checkout -f master --quiet 2>/dev/null || true
            git -C "${SRC_ROOT}" pull --ff-only origin main --quiet 2>/dev/null || git -C "${SRC_ROOT}" reset --hard origin/main --quiet 2>/dev/null || true
        else
            LATEST_TAG=$(git -C "${SRC_ROOT}" tag -l "v*glfw*" --sort=-v:refname 2>/dev/null | head -n 1)
            if [ -z "$LATEST_TAG" ]; then
                LATEST_TAG=$(git -C "${SRC_ROOT}" tag --sort=-v:refname 2>/dev/null | head -n 1)
            fi

            if [ -n "$LATEST_TAG" ] && git -C "${SRC_ROOT}" cat-file -e "${LATEST_TAG}:CMakeLists.txt" 2>/dev/null; then
                echo -e "--> Checking out latest release tag: ${BOLD}${LATEST_TAG}${NC}..."
                git -C "${SRC_ROOT}" checkout -f "${LATEST_TAG}" --quiet 2>/dev/null || true
            else
                echo -e "--> Checking out latest '${BOLD}main${NC}' branch..."
                git -C "${SRC_ROOT}" checkout -f main --quiet 2>/dev/null || git -C "${SRC_ROOT}" checkout -f master --quiet 2>/dev/null || true
                git -C "${SRC_ROOT}" pull --ff-only origin main --quiet 2>/dev/null || git -C "${SRC_ROOT}" reset --hard origin/main --quiet 2>/dev/null || true
            fi
        fi
    fi

    install_build_deps

    echo -e "\n${BOLD}--> Building CGX (GLFW Edition)...${NC}"
    
    NPROC=4
    if [ -n "$NUMBER_OF_PROCESSORS" ]; then
        NPROC="$NUMBER_OF_PROCESSORS"
    elif command -v nproc &>/dev/null; then
        NPROC=$(nproc)
    elif [ "${OS}" = "Darwin" ] && command -v sysctl &>/dev/null; then
        NPROC=$(sysctl -n hw.ncpu)
    fi

    if command -v cmake &>/dev/null && [ -f "${SRC_ROOT}/CMakeLists.txt" ]; then
        echo "Configuring and building with CMake..."
        if [ "${OS}" = "Darwin" ]; then
            cmake -S "${SRC_ROOT}" -B "${SRC_ROOT}/build" -DCMAKE_BUILD_TYPE=Release -DSTATIC_GLFW=ON
        elif [ "${OS}" = "Windows" ] && command -v ninja &>/dev/null; then
            cmake -S "${SRC_ROOT}" -B "${SRC_ROOT}/build" -G Ninja -DCMAKE_BUILD_TYPE=Release
        else
            cmake -S "${SRC_ROOT}" -B "${SRC_ROOT}/build" -DCMAKE_BUILD_TYPE=Release
        fi
        cmake --build "${SRC_ROOT}/build" -j"${NPROC}"
    else
        echo "Building with Makefile.glfw..."
        cd "${SRC_ROOT}/cgx_2.23/src"
        make -f Makefile.glfw clean 2>/dev/null || true
        make -f Makefile.glfw OPT="-O3 -march=native" -j"${NPROC}"
    fi

    # Locate and copy output binary to target installation directory
    PRODUCED_BIN=""
    if [ -f "${SRC_ROOT}/bin/cgx_glfw${BIN_EXT}" ]; then
        PRODUCED_BIN="${SRC_ROOT}/bin/cgx_glfw${BIN_EXT}"
    elif [ -f "${SRC_ROOT}/bin/cgx_glfw" ]; then
        PRODUCED_BIN="${SRC_ROOT}/bin/cgx_glfw"
    elif [ -f "${SRC_ROOT}/build/bin/cgx_glfw${BIN_EXT}" ]; then
        PRODUCED_BIN="${SRC_ROOT}/build/bin/cgx_glfw${BIN_EXT}"
    elif [ -f "${SRC_ROOT}/build/bin/cgx_glfw" ]; then
        PRODUCED_BIN="${SRC_ROOT}/build/bin/cgx_glfw"
    fi

    if [ -n "${PRODUCED_BIN}" ] && [ -f "${PRODUCED_BIN}" ]; then
        TARGET_DEST="${INSTALL_DIR}/cgx_glfw${BIN_EXT}"
        cp -f "${PRODUCED_BIN}" "${TARGET_DEST}"
        chmod +x "${TARGET_DEST}"
        if [ "${OS}" = "Darwin" ]; then
            xattr -d com.apple.quarantine "${TARGET_DEST}" 2>/dev/null || true
        fi
        create_user_alias "${TARGET_DEST}"
        echo -e "${GREEN}[OK] Successfully installed: ${TARGET_DEST}${NC}"
    else
        echo -e "${RED}Error: Build failed. Binary not found at ${SRC_ROOT}/bin/cgx_glfw${BIN_EXT}.${NC}"
        exit 1
    fi
}

# Function: Configure PATH in Shell Profile (User Level, No Sudo Required)
ensure_path_configured() {
    # Check if INSTALL_DIR is already in PATH
    if [[ ":$PATH:" == *":$INSTALL_DIR:"* ]]; then
        return 0
    fi

    echo -e "\n${BOLD}--> Configuring PATH environment...${NC}"
    
    SHELL_PROFILE=""
    CURRENT_SHELL="$(basename "${SHELL:-bash}")"

    if [ "$CURRENT_SHELL" = "zsh" ]; then
        SHELL_PROFILE="${HOME}/.zshrc"
    elif [ "$CURRENT_SHELL" = "bash" ]; then
        if [ "${OS}" = "Darwin" ]; then
            SHELL_PROFILE="${HOME}/.bash_profile"
        else
            SHELL_PROFILE="${HOME}/.bashrc"
        fi
    else
        SHELL_PROFILE="${HOME}/.profile"
    fi

    EXPORT_CMD="export PATH=\"${INSTALL_DIR}:\$PATH\""

    if [ -f "$SHELL_PROFILE" ] && grep -q "${INSTALL_DIR}" "$SHELL_PROFILE"; then
        echo -e "${GREEN}[OK] PATH already configured in ${SHELL_PROFILE}${NC}"
    else
        echo "" >> "$SHELL_PROFILE"
        echo "# CalculiX GraphiX PATH" >> "$SHELL_PROFILE"
        echo "$EXPORT_CMD" >> "$SHELL_PROFILE"
        echo -e "${GREEN}[OK] Added ${INSTALL_DIR} to ${SHELL_PROFILE}${NC}"
    fi

    export PATH="${INSTALL_DIR}:${PATH}"
}

# Function: Finalize Installation Notice
finalize_installation() {
    ensure_path_configured

    echo -e "\n${BOLD}${GREEN}=====================================================${NC}"
    if [ "$USE_HEAD" = "1" ]; then
        echo -e "${BOLD}${GREEN}   Installation Complete! [Bleeding-Edge Development]${NC}"
    else
        echo -e "${BOLD}${GREEN}   Installation Complete! [Stable Release]          ${NC}"
    fi
    echo -e "${BOLD}${GREEN}=====================================================${NC}"

    if [ "$USE_HEAD" = "1" ]; then
        echo -e "You can now run this development build from your terminal:"
        echo -e "    ${BOLD}cgx_dev <model.frd>${NC}"
        echo -e "    (or: ${BOLD}cgx_glfw <model.frd>${NC})"
    else
        echo -e "You can now run CGX from your terminal using either command:"
        echo -e "    ${BOLD}cgx <model.frd>${NC}"
        echo -e "    ${BOLD}cgx_glfw <model.frd>${NC}"
    fi

    echo -e "\n${BOLD}Features:${NC}"
    echo -e "  - ${GREEN}PNG Screenshots:${NC} 'hcpy' (zero external dependencies)"
    echo -e "  - ${GREEN}Animated GIFs:${NC}   'movie start my.gif' (zero external dependencies)"
    echo -e "  - ${GREEN}MP4 Video:${NC}       'movie start my.mp4' (requires ffmpeg)"
    echo -e "  See ${BOLD}exporting_videos.md${NC} for complete details."
    if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
        if [ "$USE_HEAD" = "1" ]; then
            echo -e "\n${YELLOW}Note: To use 'cgx_dev' immediately in this current shell, run:${NC}"
        else
            echo -e "\n${YELLOW}Note: To use 'cgx' immediately in this current shell, run:${NC}"
        fi
        echo -e "    export PATH=\"${INSTALL_DIR}:\$PATH\""
    fi
    echo ""
}

# CLI Argument parsing
CHOICE=""
NON_INTERACTIVE=""
USE_HEAD=""
CUSTOM_DIR_SPECIFIED=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --binary|--fast|-b)
            CHOICE="1"
            shift
            ;;
        --build|--source|-s)
            CHOICE="2"
            shift
            ;;
        --nightly|--head)
            CHOICE="2"
            USE_HEAD="1"
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
        -y|--yes)
            NON_INTERACTIVE="1"
            shift
            ;;
        --help|-h)
            echo "Usage: install.sh [OPTION]"
            echo "Options:"
            echo "  --binary,  -b       Fast install: Download pre-built binary & install runtime libs"
            echo "  --build,   -s       Power install: Compile locally with native CPU optimizations (latest release tag)"
            echo "  --nightly, --head   Build bleeding-edge from latest commit on 'main' branch (alias 'cgx_dev')"
            echo "  --prefix,  --dir    Specify custom binary installation directory (Default: ~/.local/bin)"
            echo "  -y,        --yes    Non-interactive mode (use defaults without prompting)"
            echo "  --help,    -h       Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

# Prompt for install directory if interactive and not specified via CLI
if [ -z "$NON_INTERACTIVE" ] && [ -z "$CUSTOM_DIR_SPECIFIED" ]; then
    echo -e "\nChoose install destination:"
    prompt_read "Directory for binaries [Default: ${DEFAULT_INSTALL_DIR}]: " "${DEFAULT_INSTALL_DIR}" USER_DIR
    USER_DIR="${USER_DIR/#\~/$HOME}"
    INSTALL_DIR="${USER_DIR:-$DEFAULT_INSTALL_DIR}"
fi
mkdir -p "${INSTALL_DIR}"

# If no CLI flag passed, prompt interactively
if [ -z "$CHOICE" ]; then
    echo -e "\nChoose installation method:"
    echo -e "  ${BOLD}1) Fast Install${NC} (Download pre-built binary + install runtime libs) ${YELLOW}[Default]${NC}"
    echo -e "  ${BOLD}2) Build from Source${NC} (Compile locally with -march=native)"
    echo ""
    prompt_read "Select [1/2] (Default: 1): " "1" CHOICE
fi

case "$CHOICE" in
    1)
        do_fast_install
        finalize_installation
        ;;
    2)
        do_build_install
        finalize_installation
        ;;
    *)
        echo -e "${RED}Invalid selection. Exiting.${NC}"
        exit 1
        ;;
esac
