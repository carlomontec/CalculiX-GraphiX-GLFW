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

Pre-compiled standalone binaries, installers, and `SHA256SUMS.txt` checksums are published on the [**Releases Page**](https://github.com/carlomontec/CalculiX-GraphiX-GLFW/releases):

* **macOS (Apple Silicon / `arm64`)**:
  * **DMG Installer**: `CalculiX-GraphiX-GLFW-macOS-arm64.dmg` — drag `CalculiX GraphiX GLFW.app` to `Applications`, and optionally run `Install CLI Tool.command` to add user-space `cgx` / `cgx_glfw` links in `~/.local/bin`.
  * **Standalone CLI binary**: `cgx_glfw-macos-arm64` (statically links GLFW; zero Homebrew runtime dependency).
* **Windows (`x86_64`)**:
  * **Windows Installer**: `CalculiX-GraphiX-GLFW-Setup-windows-x86_64.exe` — installs `cgx_glfw.exe` and `cgx.exe`, registers `.frd`/`.fbl`/`.inp` file associations, and optionally adds the install folder to `PATH`.
  * **Standalone CLI binary**: `cgx_glfw-windows-x86_64.exe` (statically linked against GLFW and C/C++ runtimes; zero MSYS2 DLL dependencies).
  ```powershell
  # PowerShell 1-Liner (binary + PATH setup):
  irm https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/install.ps1 | iex
  ```
  For unattended or customized runs:
  - Local script flags: `.\install.ps1 -Mode binary|build|head|uninstall [-InstallDir <dir>] [-Tag <tag>] [-Force] [-NonInteractive]`
  - 1-liner environment variables: `$env:CGX_MODE` (`auto` | `binary` | `build` | `head` | `uninstall`), `$env:CGX_INSTALL_DIR`, `$env:CGX_TAG`, `$env:CGX_FORCE=1`, `$env:CGX_NON_INTERACTIVE=1`, `$env:CGX_KEEP_SOURCE=1`, `$env:CGX_SKIP_PATH=1`
* **Linux (`x86_64` & `arm64` / `aarch64`)**:
  * **Desktop Bundle**: `cgx_glfw-linux-x86_64.tar.gz` or `cgx_glfw-linux-arm64.tar.gz` — extract and run `./install-desktop.sh` to install `cgx_glfw` / `cgx` into `~/.local/bin` and `.desktop`/MIME associations into `~/.local/share` without `sudo`.
  * **Standalone CLI binary**: `cgx_glfw-linux-x86_64` or `cgx_glfw-linux-arm64` — built on Ubuntu 22.04 (`glibc 2.35`) with static GLFW 3.4 (runtime `dlopen` for X11 and Wayland) and static C++ runtime.

On macOS/Linux, make standalone binaries executable before running:
```bash
chmod +x cgx_glfw-*
./cgx_glfw-*
```

Verify any downloaded file against `SHA256SUMS.txt`:
```bash
shasum -a 256 -c SHA256SUMS.txt --ignore-missing   # macOS
sha256sum -c SHA256SUMS.txt --ignore-missing       # Linux
```
```powershell
Get-FileHash .\cgx_glfw-windows-x86_64.exe -Algorithm SHA256   # Windows
```

### Security Notice (Unsigned Binaries)

Release binaries and installers are not signed with commercial Apple Developer ID or Windows Authenticode certificates.

* **macOS Gatekeeper**: Downloaded `.dmg`/`.app`/binary files carry the `com.apple.quarantine` attribute and may trigger *"cannot be opened because Apple cannot check it for malicious software"* or *"is damaged and can't be opened"*. Clear the quarantine attribute or allow execution via **System Settings → Privacy & Security → Open Anyway**:
  ```bash
  xattr -cr "/Applications/CalculiX GraphiX GLFW.app"
  xattr -d com.apple.quarantine cgx_glfw-macos-arm64 2>/dev/null || true
  ```
* **Windows SmartScreen**: Microsoft Defender SmartScreen may display *"Windows protected your PC"* when running an unsigned `.exe` or `Setup.exe`. Click **More info → Run anyway** after verifying the SHA-256 hash against `SHA256SUMS.txt`.

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
   > * **Windows (MinGW UCRT64)**: Builds a standalone `.exe` statically linked against `libglfw3.a` and the GCC/C++ runtime, with 64-bit FRD file offsets (`readfrd.c`) and QUAD8 VBO bounds fixes.

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

### Windows (MSYS2 MinGW-w64 UCRT64)
```bash
cd cgx_2.23/src
make -f Makefile.glfw -j
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

### Linux and macOS

Run the universal uninstaller or remove binaries manually:

```bash
# Universal 1-liner uninstaller
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/uninstall.sh)"

# Or remove installed binary and optional source cache manually
rm -f ~/.local/bin/cgx_glfw ~/.local/bin/cgx ~/.local/bin/cgx_dev /usr/local/bin/cgx_glfw
rm -rf ~/.cgx
```

### Windows

* **PowerShell 1-Liner**:
  ```powershell
  irm https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/uninstall.ps1 | iex
  ```
  You can also choose option **`[4] Uninstall`** in `install.ps1` (or run `.\install.ps1 -Mode uninstall`).
* **Windows Setup (`.exe` installer)**:
  Re-run `CalculiX-GraphiX-GLFW-Setup-windows-x86_64.exe` and choose **Uninstall**, or uninstall **CalculiX GraphiX GLFW** from Windows **Settings → Apps → Installed apps** (removes binaries, shortcuts, `.frd`/`.fbl`/`.inp` file associations, and `PATH` entry).
