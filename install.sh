#!/usr/bin/env bash
# ==============================================================================
# CalculiX GraphiX (GLFW Edition) — Universal 1-Liner Installer
#
# 1-Liner Usage:
#   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/install.sh)"
#
# Supports:
#   - macOS (Apple Silicon arm64 pre-built; Intel builds from source)
#   - Linux (x86_64 & ARM64)
#   - Windows (MSYS2 UCRT64 / MinGW64 / Git Bash x86_64)
# ==============================================================================

set -e

REPO="carlomontec/CalculiX-GraphiX-GLFW"
GITHUB_REPO_URL="https://github.com/${REPO}.git"
DEFAULT_RELEASE_URL="https://github.com/${REPO}/releases/latest/download"
RELEASE_URL=""

# Color codes
BOLD="\033[1m"
GREEN="\033[0;32m"
BLUE="\033[0;34m"
YELLOW="\033[1;33m"
RED="\033[0;31m"
NC="\033[0m" # No Color

# Helper for interactive reading (handles stdin pipe and non-TTY safely)
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
        read -r -p "$prompt_msg" input_val || true
    elif [ -f "$0" ]; then
        printf "%s" "$prompt_msg"
        IFS= read -r input_val || input_val="$default_val"
    elif (true < /dev/tty) 2>/dev/null; then
        read -r -p "$prompt_msg" input_val < /dev/tty || true
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
# Arg1: 1 (default) exits on failure/decline; 0 returns non-zero for optional tools
ensure_homebrew_macos() {
    local required="${1:-1}"

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
        if ! /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"; then
            echo -e "${RED}Error: Homebrew installation did not complete.${NC}"
            [ "$required" -eq 1 ] && exit 1 || return 1
        fi

        if [ -x "/opt/homebrew/bin/brew" ]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        elif [ -x "/usr/local/bin/brew" ]; then
            eval "$(/usr/local/bin/brew shellenv)"
        fi

        if command -v brew &>/dev/null; then
            echo -e "${GREEN}[OK] Homebrew successfully installed!${NC}"
            return 0
        else
            echo -e "${RED}Error: Homebrew was installed but 'brew' is not yet in PATH.${NC}"
            [ "$required" -eq 1 ] && exit 1 || return 1
        fi
    else
        if [ "$required" -eq 1 ]; then
            echo -e "\n${YELLOW}Homebrew is required for building on macOS. Install it at https://brew.sh and re-run this script.${NC}"
            exit 1
        fi
        echo -e "${YELLOW}Skipped Homebrew installation.${NC}"
        return 1
    fi
}

# Function: Install Gnuplot via the platform's package manager (non-fatal on error)
install_gnuplot_pkg() {
    echo -e "\n${BOLD}--> Installing gnuplot...${NC}"
    if [ "${OS}" = "Darwin" ]; then
        if ensure_homebrew_macos 0; then
            brew install gnuplot || true
        fi
    elif [ "${OS}" = "Linux" ]; then
        if command -v brew &>/dev/null; then
            brew install gnuplot || true
        elif command -v apt-get &>/dev/null; then
            (sudo apt-get update -y && sudo apt-get install -y gnuplot) || true
        elif command -v dnf &>/dev/null; then
            sudo dnf install -y gnuplot || true
        elif command -v pacman &>/dev/null; then
            sudo pacman -S --needed --noconfirm gnuplot || true
        elif command -v zypper &>/dev/null; then
            sudo zypper install -y gnuplot || true
        else
            echo -e "${YELLOW}Notice: No supported package manager found for automatic gnuplot installation.${NC}"
        fi
    elif [ "${OS}" = "Windows" ]; then
        if command -v pacman &>/dev/null; then
            local pkg_prefix="${MINGW_PACKAGE_PREFIX:-mingw-w64-ucrt-x86_64}"
            pacman -S --needed --noconfirm "${pkg_prefix}-gnuplot" || true
        elif command -v winget &>/dev/null; then
            winget install Gnuplot.Gnuplot || true
        else
            echo -e "${YELLOW}Notice: Neither pacman (MSYS2) nor winget was found for automatic gnuplot installation.${NC}"
        fi
    fi

    if command -v gnuplot &>/dev/null; then
        echo -e "${GREEN}[OK] gnuplot installed and ready for 2D engineering plots.${NC}"
        return 0
    fi
    return 1
}

print_gnuplot_tip() {
    if [ "${OS}" = "Darwin" ]; then
        echo -e "${BLUE}Tip: Install gnuplot ('brew install gnuplot') if you want 2D plot image rendering.${NC}"
    elif [ "${OS}" = "Linux" ]; then
        echo -e "${BLUE}Tip: Install gnuplot ('sudo apt install gnuplot' or 'sudo pacman -S gnuplot') for 2D plots.${NC}"
    elif [ "${OS}" = "Windows" ]; then
        echo -e "${BLUE}Tip: Install gnuplot for Windows (pacman -S mingw-w64-ucrt-x86_64-gnuplot or winget install Gnuplot.Gnuplot) for 2D plots.${NC}"
    fi
}

# Function: Check Optional Runtime Tools (offers interactive Gnuplot setup)
install_runtime_deps() {
    echo -e "\n${BOLD}--> Checking optional runtime tools...${NC}"
    if [ "${OS}" = "Darwin" ]; then
        echo -e "${GREEN}[OK] Standalone macOS binary embeds GLFW (no Homebrew runtime dependency).${NC}"
    elif [ "${OS}" = "Linux" ]; then
        echo -e "${GREEN}[OK] Standalone Linux binary embeds GLFW 3.4 (runtime X11/Wayland support).${NC}"
        if command -v ldconfig &>/dev/null && ! ldconfig -p 2>/dev/null | grep -q "libGL\.so\.1"; then
            echo -e "${YELLOW}Tip: Ensure OpenGL/Mesa runtime libraries (libgl1 / libglu1-mesa) are installed.${NC}"
        fi
    elif [ "${OS}" = "Windows" ]; then
        echo -e "${GREEN}[OK] Windows standalone runtime configured.${NC}"
    fi

    if command -v gnuplot &>/dev/null; then
        echo -e "${GREEN}[OK] gnuplot is available for 2D engineering plots.${NC}"
    elif [ -z "$NON_INTERACTIVE" ]; then
        prompt_read "Gnuplot was not found (recommended for 2D engineering plots). Install Gnuplot now? [Y/n]: " "Y" INSTALL_GNUPLOT
        if [[ "$INSTALL_GNUPLOT" =~ ^[Yy]$ ]]; then
            if ! install_gnuplot_pkg; then
                echo -e "${YELLOW}Notice: Could not install gnuplot automatically; continuing without it.${NC}"
                print_gnuplot_tip
            fi
        else
            print_gnuplot_tip
        fi
    else
        print_gnuplot_tip
    fi

    if command -v ffmpeg &>/dev/null; then
        echo -e "${GREEN}[OK] ffmpeg video tools available for MP4 recording.${NC}"
    elif [ "${OS}" = "Darwin" ]; then
        echo -e "${BLUE}Tip: Install ffmpeg ('brew install ffmpeg') to enable MP4 video recording.${NC}"
    elif [ "${OS}" = "Linux" ]; then
        echo -e "${BLUE}Tip: Install ffmpeg ('sudo apt install ffmpeg') to enable MP4 video recording.${NC}"
    elif [ "${OS}" = "Windows" ]; then
        echo -e "${BLUE}Tip: Install ffmpeg for Windows (winget install Gyan.FFmpeg) to enable MP4 video recording.${NC}"
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
            PKG_PREFIX="${MINGW_PACKAGE_PREFIX:-mingw-w64-ucrt-x86_64}"
            echo "Installing MinGW-w64 toolchain and GLFW via pacman (${PKG_PREFIX})..."
            pacman -S --needed --noconfirm "${PKG_PREFIX}-gcc" "${PKG_PREFIX}-glfw" "${PKG_PREFIX}-cmake" "${PKG_PREFIX}-ninja" make git
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

# Function: Compute lowercase SHA-256 hex digest of a file
compute_sha256() {
    local file_path="$1"
    if command -v sha256sum &>/dev/null; then
        sha256sum "${file_path}" | awk '{print tolower($1)}'
    elif command -v shasum &>/dev/null; then
        shasum -a 256 "${file_path}" | awk '{print tolower($1)}'
    elif command -v openssl &>/dev/null; then
        openssl dgst -sha256 "${file_path}" | awk '{print tolower($NF)}'
    else
        echo ""
    fi
}

# Function: Create User-level Symlink / Alias (cgx for stable, cgx_dev for --head)
create_user_alias() {
    local target_bin="$1"
    local alias_name="cgx"
    local label="User alias"

    if [ "$USE_HEAD" = "1" ]; then
        alias_name="cgx_dev"
        label="Development alias"
    fi

    local alias_bin="${INSTALL_DIR}/${alias_name}${BIN_EXT}"
    rm -f "${alias_bin}"

    if [ "$IS_WINDOWS" -eq 1 ]; then
        cp -f "${target_bin}" "${alias_bin}" 2>/dev/null || true
    else
        ln -sf "cgx_glfw${BIN_EXT}" "${alias_bin}" 2>/dev/null || ln -sf "${target_bin}" "${alias_bin}" 2>/dev/null || true
    fi
    echo -e "${GREEN}[OK] ${label} created: ${alias_bin} -> cgx_glfw${BIN_EXT}${NC}"
}

# Function: Stage, sanity-check, and atomically replace target binary
install_binary_safely() {
    local src_file="$1"
    local dest_bin="$2"
    local dest_dir
    dest_dir="$(dirname "${dest_bin}")"
    local staged_bin="${dest_bin}.tmp.$$"

    mkdir -p "${dest_dir}"
    cp -f "${src_file}" "${staged_bin}"
    chmod +x "${staged_bin}"
    if [ "${OS}" = "Darwin" ]; then
        xattr -d com.apple.quarantine "${staged_bin}" 2>/dev/null || true
    fi

    # Smoke test staged executable on the host before replacing destination
    if ! "${staged_bin}" --version >/dev/null 2>&1; then
        echo -e "${YELLOW}Notice: Could not verify '--version' on staged binary (${staged_bin}).${NC}"
    fi

    # Unlink destination first to avoid ETXTBSY ('Text file busy') or read-only permission errors
    rm -f "${dest_bin}"
    if ! mv -f "${staged_bin}" "${dest_bin}"; then
        rm -f "${staged_bin}"
        echo -e "${RED}Error: Failed to install binary into ${dest_bin}.${NC}"
        exit 1
    fi
}

# Function: Detect existing installation and ask user before overwriting
check_existing_installation() {
    local existing_bin="${INSTALL_DIR}/cgx_glfw${BIN_EXT}"
    local existing_alias="${INSTALL_DIR}/cgx${BIN_EXT}"
    local existing_dev_alias="${INSTALL_DIR}/cgx_dev${BIN_EXT}"
    local found_items=0

    for item in "${existing_bin}" "${existing_alias}" "${existing_dev_alias}"; do
        if [ -f "${item}" ] || [ -L "${item}" ]; then
            found_items=$((found_items + 1))
        fi
    done

    if [ "$found_items" -eq 0 ]; then
        return 0
    fi

    echo -e "\n${BOLD}${YELLOW}--> Existing installation detected in ${INSTALL_DIR}:${NC}"

    if [ -f "${existing_bin}" ] || [ -L "${existing_bin}" ]; then
        local ver_info=""
        local sha_info=""
        if [ -x "${existing_bin}" ]; then
            ver_info="$("${existing_bin}" --version 2>/dev/null | grep -m1 "Version" || true)"
        fi
        local full_hash
        full_hash="$(compute_sha256 "${existing_bin}" 2>/dev/null || true)"
        if [ -n "${full_hash}" ]; then
            sha_info="sha256: ${full_hash:0:12}"
        fi

        local meta=""
        if [ -n "${ver_info}" ] && [ -n "${sha_info}" ]; then
            meta=" (${ver_info}, ${sha_info})"
        elif [ -n "${ver_info}" ]; then
            meta=" (${ver_info})"
        elif [ -n "${sha_info}" ]; then
            meta=" (${sha_info})"
        fi
        echo -e "  - Binary: ${BOLD}${existing_bin}${NC}${meta}"
    fi

    for alias_item in "${existing_alias}" "${existing_dev_alias}"; do
        if [ -L "${alias_item}" ]; then
            local link_target
            link_target="$(readlink "${alias_item}" 2>/dev/null || echo "unknown")"
            echo -e "  - Alias:  ${BOLD}${alias_item}${NC} -> ${link_target}"
        elif [ -f "${alias_item}" ]; then
            echo -e "  - Alias:  ${BOLD}${alias_item}${NC}"
        fi
    done

    if [ "$USE_HEAD" = "1" ] && { [ -f "${existing_alias}" ] || [ -L "${existing_alias}" ]; }; then
        echo -e "  ${YELLOW}Note: Stable alias '${existing_alias}' points to cgx_glfw${BIN_EXT} and will follow the development build.${NC}"
    fi

    if [ -n "$FORCE_OVERWRITE" ] || [ -n "$NON_INTERACTIVE" ]; then
        echo -e "--> Replacing existing installation (--force / non-interactive mode)..."
        return 0
    fi

    echo ""
    prompt_read "Replace existing installation, cancel, or uninstall? [Y/n/u] (Y=replace, n=cancel, u=uninstall): " "Y" REPLACE_CHOICE
    if [[ "$REPLACE_CHOICE" =~ ^[Uu]$ ]]; then
        do_uninstall
        exit 0
    elif [[ ! "$REPLACE_CHOICE" =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}Installation cancelled. Existing binaries were kept untouched.${NC}"
        exit 0
    fi
}

# Function: Verify downloaded binary against SHA256SUMS.txt
verify_sha256() {
    local file_path="$1"
    local asset_name="$2"
    local sums_file
    sums_file="$(mktemp)"

    if ! curl -fsSL -o "${sums_file}" "${RELEASE_URL}/SHA256SUMS.txt" 2>/dev/null; then
        rm -f "${sums_file}"
        echo -e "${YELLOW}Notice: SHA256SUMS.txt not found at release URL; skipping checksum verification.${NC}"
        return 0
    fi

    local expected_hash
    expected_hash=$(awk -v target="$asset_name" '{
        fname = $2
        sub(/^\.\//, "", fname)
        sub(/^\*/, "", fname)
        if (fname == target) { print tolower($1); exit }
    }' "${sums_file}")
    rm -f "${sums_file}"

    if [ -z "${expected_hash}" ]; then
        echo -e "${YELLOW}Notice: '${asset_name}' not listed in SHA256SUMS.txt; skipping checksum verification.${NC}"
        return 0
    fi

    local actual_hash
    actual_hash="$(compute_sha256 "${file_path}")"
    if [ -z "${actual_hash}" ]; then
        echo -e "${YELLOW}Notice: No SHA-256 tool found; skipping checksum verification.${NC}"
        return 0
    fi

    if [ "${expected_hash}" != "${actual_hash}" ]; then
        echo -e "${RED}Error: SHA-256 checksum mismatch for ${asset_name}!${NC}"
        echo "  Expected: ${expected_hash}"
        echo "  Actual:   ${actual_hash}"
        rm -f "${file_path}"
        exit 1
    fi
    echo -e "${GREEN}[OK] SHA-256 checksum verified for ${asset_name}.${NC}"
}

# Function: Fast Install via Pre-built Binary
do_fast_install() {
    if [ -z "${BINARY_NAME}" ]; then
        echo -e "${YELLOW}No pre-built binary for ${OS} ${ARCH}. Switching to build from source.${NC}"
        do_build_install
        return
    fi

    DOWNLOAD_URL="${RELEASE_URL}/${BINARY_NAME}"
    TARGET_BIN="${INSTALL_DIR}/cgx_glfw${BIN_EXT}"
    TMP_BIN="$(mktemp)"

    echo -e "\n${BOLD}--> Fetching pre-compiled binary from:${NC}"
    echo "    ${DOWNLOAD_URL}"
    
    if curl -fSL --progress-bar -o "${TMP_BIN}" "${DOWNLOAD_URL}"; then
        verify_sha256 "${TMP_BIN}" "${BINARY_NAME}"
        install_binary_safely "${TMP_BIN}" "${TARGET_BIN}"
        rm -f "${TMP_BIN}"
        create_user_alias "${TARGET_BIN}"
        echo -e "${GREEN}[OK] Successfully installed: ${TARGET_BIN}${NC}"
    else
        rm -f "${TMP_BIN}"
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
        elif [ -n "$TARGET_TAG" ]; then
            echo -e "--> Checking out requested release tag: ${BOLD}${TARGET_TAG}${NC}..."
            if ! git -C "${SRC_ROOT}" checkout -f "${TARGET_TAG}" --quiet 2>/dev/null; then
                echo -e "${RED}Error: Release tag '${TARGET_TAG}' not found in repository.${NC}"
                exit 1
            fi
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
        install_binary_safely "${PRODUCED_BIN}" "${TARGET_DEST}"
        create_user_alias "${TARGET_DEST}"
        echo -e "${GREEN}[OK] Successfully installed: ${TARGET_DEST}${NC}"
    else
        echo -e "${RED}Error: Build failed. Binary not found at ${SRC_ROOT}/bin/cgx_glfw${BIN_EXT}.${NC}"
        exit 1
    fi
}

# Function: Configure PATH in Shell Profile (User Level, No Sudo Required)
ensure_path_configured() {
    PATH_WAS_MISSING=0
    # Check if INSTALL_DIR is already in PATH
    if [[ ":$PATH:" == *":$INSTALL_DIR:"* ]]; then
        return 0
    fi
    PATH_WAS_MISSING=1

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
    install_runtime_deps
    ensure_path_configured

    local expected_alias="cgx"
    if [ "$USE_HEAD" = "1" ]; then
        expected_alias="cgx_dev"
    fi

    local resolved_bin
    resolved_bin="$(command -v "${expected_alias}" 2>/dev/null || true)"
    if [ -n "${resolved_bin}" ] && \
       [ "${resolved_bin}" != "${INSTALL_DIR}/${expected_alias}${BIN_EXT}" ] && \
       [ "${resolved_bin}" != "${INSTALL_DIR}/${expected_alias}" ]; then
        echo -e "\n${YELLOW}Warning: Another '${expected_alias}' executable was found earlier in PATH: ${resolved_bin}${NC}"
        echo -e "To prioritize the newly installed binary, ensure '${INSTALL_DIR}' comes first in PATH:"
        echo -e "    export PATH=\"${INSTALL_DIR}:\$PATH\""
    fi

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
    echo -e "  Run ${BOLD}cgx --help${NC} for the full command reference."
    if [ "${PATH_WAS_MISSING:-0}" -eq 1 ]; then
        if [ "$USE_HEAD" = "1" ]; then
            echo -e "\n${YELLOW}Note: To use 'cgx_dev' immediately in this current shell, run:${NC}"
        else
            echo -e "\n${YELLOW}Note: To use 'cgx' immediately in this current shell, run:${NC}"
        fi
        echo -e "    export PATH=\"${INSTALL_DIR}:\$PATH\""
    fi
    echo ""
}

# Function: Remove CalculiX GraphiX PATH entry from a shell profile
clean_shell_path() {
    local profile="$1"
    if [ -f "$profile" ] && grep -q "# CalculiX GraphiX PATH" "$profile"; then
        local tmp_profile="${profile}.cgxbak"
        awk -v target_line="export PATH=\"${INSTALL_DIR}:\$PATH\"" '
            $0 == "# CalculiX GraphiX PATH" { skip_next=1; next }
            skip_next && ($0 == target_line || $0 ~ /^export PATH=".*\.local\/bin:\$PATH"$/) { skip_next=0; next }
            { skip_next=0; print }
        ' "$profile" > "$tmp_profile" && mv -f "$tmp_profile" "$profile"
        echo -e "${GREEN}[OK] Cleaned PATH entry from ${profile}${NC}"
    fi
}

# Function: Uninstall CGX GLFW binaries, aliases, build cache, and PATH entry (zero sudo)
do_uninstall() {
    local target_bin="${INSTALL_DIR}/cgx_glfw${BIN_EXT}"
    local target_alias="${INSTALL_DIR}/cgx${BIN_EXT}"
    local target_dev_alias="${INSTALL_DIR}/cgx_dev${BIN_EXT}"
    local source_dir="${HOME}/.cgx"
    local global_bin="/usr/local/bin/cgx_glfw"
    local global_alias="/usr/local/bin/cgx"
    local global_dev_alias="/usr/local/bin/cgx_dev"

    local found_items=0
    echo -e "\n${BOLD}--> Detecting installed components...${NC}"

    if [ -f "${target_bin}" ] || [ -L "${target_bin}" ]; then
        echo -e "  [Found] Binary: ${BOLD}${target_bin}${NC}"
        found_items=$((found_items + 1))
    fi

    if [ -f "${target_alias}" ] || [ -L "${target_alias}" ]; then
        echo -e "  [Found] User Alias (Stable): ${BOLD}${target_alias}${NC}"
        found_items=$((found_items + 1))
    fi

    if [ -f "${target_dev_alias}" ] || [ -L "${target_dev_alias}" ]; then
        echo -e "  [Found] User Alias (Dev): ${BOLD}${target_dev_alias}${NC}"
        found_items=$((found_items + 1))
    fi

    if [ -w "/usr/local/bin" ] && { [ -f "${global_bin}" ] || [ -L "${global_bin}" ]; }; then
        echo -e "  [Found] Global Link: ${BOLD}${global_bin}${NC}"
        found_items=$((found_items + 1))
    fi

    if [ -d "${source_dir}" ]; then
        echo -e "  [Found] Build Source Cache: ${BOLD}${source_dir}${NC}"
        found_items=$((found_items + 1))
    fi

    if [ "$found_items" -eq 0 ]; then
        echo -e "${YELLOW}No installed CGX GLFW components found in ${INSTALL_DIR} (or ${source_dir}).${NC}"
        clean_shell_path "${HOME}/.bashrc"
        clean_shell_path "${HOME}/.zshrc"
        clean_shell_path "${HOME}/.bash_profile"
        clean_shell_path "${HOME}/.profile"
        exit 0
    fi

    if [ -z "$NON_INTERACTIVE" ] && [ -z "$FORCE_OVERWRITE" ]; then
        echo ""
        prompt_read "Are you sure you want to uninstall CalculiX GraphiX (GLFW)? [y/N]: " "N" CONFIRM_UNINSTALL
        if [[ ! "$CONFIRM_UNINSTALL" =~ ^[Yy]$ ]]; then
            echo -e "${YELLOW}Uninstall cancelled.${NC}"
            exit 0
        fi
    fi

    if [ -f "${target_bin}" ] || [ -L "${target_bin}" ]; then
        rm -f "${target_bin}"
        echo -e "${GREEN}[OK] Removed binary: ${target_bin}${NC}"
    fi

    if [ -f "${target_alias}" ] || [ -L "${target_alias}" ]; then
        rm -f "${target_alias}"
        echo -e "${GREEN}[OK] Removed user alias: ${target_alias}${NC}"
    fi

    if [ -f "${target_dev_alias}" ] || [ -L "${target_dev_alias}" ]; then
        rm -f "${target_dev_alias}"
        echo -e "${GREEN}[OK] Removed dev alias: ${target_dev_alias}${NC}"
    fi

    if [ -w "/usr/local/bin" ]; then
        rm -f "${global_bin}" "${global_alias}" "${global_dev_alias}" 2>/dev/null || true
    fi

    if [ -d "${source_dir}" ]; then
        local remove_src="Y"
        if [ -n "$KEEP_SOURCE" ]; then
            remove_src="N"
        elif [ -z "$NON_INTERACTIVE" ] && [ -z "$FORCE_OVERWRITE" ]; then
            echo ""
            prompt_read "Remove source repository build cache (${source_dir})? [Y/n]: " "Y" remove_src
        fi

        if [[ "$remove_src" =~ ^[Yy]$ ]]; then
            rm -rf "${source_dir}"
            echo -e "${GREEN}[OK] Removed build source cache: ${source_dir}${NC}"
        else
            echo -e "${YELLOW}Preserved build source cache at: ${source_dir}${NC}"
        fi
    fi

    clean_shell_path "${HOME}/.bashrc"
    clean_shell_path "${HOME}/.zshrc"
    clean_shell_path "${HOME}/.bash_profile"
    clean_shell_path "${HOME}/.profile"

    echo -e "\n${BOLD}${GREEN}=====================================================${NC}"
    echo -e "${BOLD}${GREEN}   Uninstall Complete! (Zero Sudo / User Space)     ${NC}"
    echo -e "${BOLD}${GREEN}=====================================================${NC}"
}

# CLI Argument parsing
CHOICE=""
NON_INTERACTIVE=""
USE_HEAD=""
FORCE_OVERWRITE=""
KEEP_SOURCE=""
TARGET_TAG=""
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
            CHOICE="3"
            USE_HEAD="1"
            shift
            ;;
        --uninstall|-u)
            CHOICE="4"
            shift
            ;;
        --keep-source)
            KEEP_SOURCE="1"
            shift
            ;;
        --force|-f)
            FORCE_OVERWRITE="1"
            shift
            ;;
        --tag)
            if [ -n "$2" ]; then
                TARGET_TAG="$2"
                shift 2
            else
                echo -e "${RED}Error: $1 requires a tag argument (e.g. v2.23-glfw-0.10.0).${NC}"
                exit 1
            fi
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
            echo "  --binary,    -b     Fast install: Download pre-built binary (latest release tag)"
            echo "  --build,     -s     Power install: Compile locally with native CPU optimizations (latest release tag)"
            echo "  --nightly,   --head Build bleeding-edge from latest commit on 'main' branch (alias 'cgx_dev')"
            echo "  --uninstall, -u     Uninstall CGX GLFW binaries, aliases, build cache, and PATH entry"
            echo "  --keep-source       Keep cached source build directory (~/.cgx) when uninstalling"
            echo "  --tag <TAG>         Install or build a specific release tag (e.g. v2.23-glfw-0.10.0)"
            echo "  --force,     -f     Overwrite or uninstall without prompting"
            echo "  --prefix,    --dir  Specify custom binary installation directory (Default: ~/.local/bin)"
            echo "  -y,          --yes  Non-interactive mode (use defaults without prompting)"
            echo "  --help,      -h     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

# Resolve release download URL after parsing --tag and environment overrides
if [ -n "${CGX_DOWNLOAD_BASE_URL}" ]; then
    RELEASE_URL="${CGX_DOWNLOAD_BASE_URL}"
elif [ -n "${TARGET_TAG}" ]; then
    RELEASE_URL="https://github.com/${REPO}/releases/download/${TARGET_TAG}"
else
    RELEASE_URL="${DEFAULT_RELEASE_URL}"
fi

# Prompt for install directory if interactive and not specified via CLI or env
if [ -z "$NON_INTERACTIVE" ] && [ -z "$CUSTOM_DIR_SPECIFIED" ] && [ -z "${CGX_INSTALL_DIR:-}" ]; then
    echo -e "\nChoose install destination:"
    prompt_read "Directory for binaries [Default: ${DEFAULT_INSTALL_DIR}]: " "${DEFAULT_INSTALL_DIR}" USER_DIR
    USER_DIR="${USER_DIR/#\~/$HOME}"
    INSTALL_DIR="${USER_DIR:-$DEFAULT_INSTALL_DIR}"
fi

# If no CLI flag passed, prompt interactively with all options upfront
if [ -z "$CHOICE" ]; then
    echo -e "\nChoose action:"
    echo -e "  ${BOLD}1) Fast Install${NC} (Download pre-built release binary) ${YELLOW}[Default]${NC}"
    echo -e "  ${BOLD}2) Build from Source (Stable Release)${NC} (Compile latest release tag with -march=native)"
    echo -e "  ${BOLD}3) Build from Source (Latest Development)${NC} (Compile latest 'main' commit -> alias 'cgx_dev')"
    echo -e "  ${BOLD}4) Uninstall${NC} (Remove installed binaries, aliases, build cache, and PATH entry)"
    echo ""
    prompt_read "Select [1/2/3/4] (Default: 1): " "1" CHOICE
fi

case "$CHOICE" in
    1) ;;
    2) ;;
    3) USE_HEAD="1" ;;
    4)
        do_uninstall
        exit 0
        ;;
    *)
        echo -e "${RED}Invalid selection. Exiting.${NC}"
        exit 1
        ;;
esac

# Check if an existing installation is present and ask before overwriting
check_existing_installation
mkdir -p "${INSTALL_DIR}"

case "$CHOICE" in
    1)
        do_fast_install
        finalize_installation
        ;;
    2|3)
        do_build_install
        finalize_installation
        ;;
esac
