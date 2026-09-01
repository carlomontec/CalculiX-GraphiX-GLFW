# CalculiX GraphiX (GLFW Edition) — Installation Guide

This guide covers all methods to install or compile **CalculiX GraphiX (GLFW Edition)** on macOS, Linux, and Windows.

---

## 1. Quick Start (Universal 1-Liner)

For **macOS (Apple Silicon)** and **Linux (x86_64 & ARM64)**:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/install.sh)"
```

The script offers two modes:
1. **Pre-built binary**: Downloads the latest release binary for your platform.
2. **Build from source**: Installs required build tools and compiles locally via CMake.

### Unattended Flags:
```bash
./install.sh --binary   # Fast pre-built install
./install.sh --build    # Local compilation from latest release tag
./install.sh --head     # Local compilation from latest 'main' branch
```

The binary is installed into `~/.local/bin/cgx_glfw` and added to your `PATH`.

---

## 2. Pre-Compiled Binaries

Pre-compiled standalone binaries are available on the [**Releases Page**](https://github.com/carlomontec/CalculiX-GraphiX-GLFW/releases):

* **Windows**: Download `cgx_glfw-windows-x86_64.exe` (statically linked, standalone).
  ```powershell
  # PowerShell 1-Liner:
  Invoke-WebRequest -Uri "https://github.com/carlomontec/CalculiX-GraphiX-GLFW/releases/latest/download/cgx_glfw-windows-x86_64.exe" -OutFile "cgx_glfw.exe"
  ```
* **macOS (Apple Silicon / arm64)**: Download `cgx_glfw-macos-arm64`.
* **Linux (x86_64)**: Download `cgx_glfw-linux-x86_64`.
* **Linux (ARM64 / aarch64)**: Download `cgx_glfw-linux-arm64`.

On macOS/Linux, make the binary executable before running:
```bash
chmod +x cgx_glfw-*
xattr -d com.apple.quarantine cgx_glfw-macos-* 2>/dev/null || true # macOS only
./cgx_glfw-*
```

---

## 3. Build from Source with CMake (Recommended)

### Prerequisites

#### macOS
```bash
brew install cmake glfw
```

#### Linux
* **Ubuntu / Debian / Linux Mint**:
  ```bash
  sudo apt-get update && sudo apt-get install -y cmake build-essential libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev git
  ```
* **Fedora / RHEL / Rocky**:
  ```bash
  sudo dnf install -y cmake gcc-c++ make glfw-devel mesa-libGL-devel mesa-libGLU-devel git
  ```
* **Arch / Manjaro / CachyOS**:
  ```bash
  sudo pacman -S --needed cmake base-devel glfw-x11 mesa glu git
  ```

#### Windows (MSYS2 MinGW-w64)
From the MSYS2 UCRT64 or MINGW64 shell:
```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-glfw git
```

---

### Build Steps

1. **Clone the repository**:
   ```bash
   git clone https://github.com/carlomontec/CalculiX-GraphiX-GLFW.git
   cd CalculiX-GraphiX-GLFW
   ```

2. **Configure and Build**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ```

   > **Note on Static Linking & Platform Status**:
   > * **macOS / Linux**: Fully supported and verified on small, medium, and large multi-dataset FEA models.
   > * **macOS**: `STATIC_GLFW=ON` is enabled by default, creating a standalone binary with zero Homebrew runtime dependency.
   > * **Windows (MinGW)**: Builds a standalone `.exe` statically linked with GLFW and GCC runtime. Standard models, meshing, and interactive commands are fully operational. Large multi-dataset modal models (e.g. 100k+ elements with 10+ modes) are currently under investigation for Windows-specific heap/stream handling.

3. **Run**:
   ```bash
   ./bin/cgx_glfw test/beam_modal.frd
   ```

4. **Optional: Install to system**:
   ```bash
   cmake --install build --prefix ~/.local
   ```

---

## 4. Build with Classic Makefile (Alternative)

If you prefer building without CMake:

### macOS / Linux
```bash
cd cgx_2.23/src
make -f Makefile.glfw -j
../../bin/cgx_glfw
```

### Windows (MSYS2 MinGW-w64)
```bash
cd cgx_2.23/src
make -f Makefile.glfw.win -j
../../bin/cgx_glfw.exe
```

---

## 5. Verification

To verify that the installation and graphics backend are working correctly:

```bash
cgx_glfw test/beam_modal.frd
```

You should see the 3D beam model with dark mode background, 3D perspective projection, and the bottom interactive command bar.

---

## 6. Uninstallation

To remove `cgx_glfw` from your system:

```bash
# Remove installed binary
rm -f ~/.local/bin/cgx_glfw /usr/local/bin/cgx_glfw

# Optionally remove cached source repository (if using 1-liner)
rm -rf ~/.cgx
```
