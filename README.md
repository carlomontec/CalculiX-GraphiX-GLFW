# CalculiX GraphiX (GLFW Edition)

> **A Modernized multi-platform edition of CalculiX GraphiX.**

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](COPYING)
[![Platform: macOS | Linux & Windows (Soon)](https://img.shields.io/badge/Platform-macOS%20%7C%20Linux%20%26%20Windows%20(Soon)-brightgreen.svg)](#build-instructions)
[![Dependencies: 100% GLUT-Free](https://img.shields.io/badge/Dependencies-100%25%20GLUT--Free%20%26%20X11--Free-success.svg)](#highlights--new-features)
[![Graphics: GLFW3 + Modern OpenGL](https://img.shields.io/badge/Graphics-GLFW3%20%2B%20stb__truetype-orange.svg)](#highlights--new-features)
[![ParaView: Native VTU/PVD](https://img.shields.io/badge/ParaView-Native%20VTU%2FPVD-purple.svg)](#native-paraview-vtupvd-exporter)

![CalculiX GraphiX GLFW Edition](img/cgx_glfw.png)

---

## About This Project

This project is an **academic exercise for learning AI agentic programming** by **Carlo Monjaraz-Tec**, based on the great work of the original authors and contributors of **CalculiX GraphiX (CGX)**, originally led by **Klaus Wittig** (https://www.dhondt.de/ See section _CalculiX GraphiX_).

The objective is to explore agent-assisted refactoring by modernizing CGX's windowing and rendering layer (introducing GLFW3, smooth vector typography via `stb_truetype`, an in-window command bar, dark mode, and 3D perspective projection) and VTU export, while keeping all core finite element mechanics, meshing routines (`libSNL`), file parsers, and solver workflows completely intact.

---

## Highlights & New Features

* **100% GLUT-Free & X11-Free**: Free of legacy X11, GLX, and raw GLUT dependencies. Windowing and input events are routed through a modern, native **GLFW3** layer across macOS (Apple Silicon / Intel Cocoa), Linux (Wayland / X11), and Windows.
* **Anti-Aliased Vector Typography**: Integrated [`stb_truetype`](https://github.com/nothings/stb) vector font engine with dynamic 3-tier font sizing (`Small`, `Medium`, `Big`).
* **Signature Dark Mode by Default**: Modern dark slate aesthetic (`#0D121A`) with high-contrast text and colormaps.
* **True 3D Perspective Projection**: Perspective projection active by default with center-anchored geometry framing (`frame`).
* **Interactive Command Bar with Fuzzy Suggestions**: In-window command line bar with history navigation (<kbd>↑</kbd>/<kbd>↓</kbd>) and intelligent Levenshtein typo suggestions (e.g. `fram` $\rightarrow$ `frame`).
* **Modern CAD / Blender Navigation Modifiers**: Smooth orbit, <kbd>Shift</kbd>/<kbd>Ctrl</kbd>/<kbd>Cmd</kbd> + Left Drag to Pan, and <kbd>Alt</kbd>/<kbd>Option</kbd> + Left Drag to Zoom.
* **Retina & High-DPI Auto-Scaling**: Wireframes, text, and node points dynamically scale to your monitor's pixel ratio.
* **Native Base64 Binary VTU/PVD Exporter**: 1-click export of complex 1D, 2D, and 3D meshes and transient results to ParaView (`send all vtu all`).

> 📖 **Deep Dive**: For full details on all graphical enhancements, rendering options, and shortcuts, see [**GUI Extra Features**](GUI_EXTRA_FEATURES.md).

---

## Quick Start (Universal 1-Liner)

Install and run **CalculiX GraphiX (GLFW Edition)** on **macOS (Apple Silicon / Intel)** or **Linux (Ubuntu, Debian, RHEL, Rocky, Fedora, Arch)** with a single command:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/install.sh)"
```

### Installation Options:
When prompted, you can choose:
* **Option 1: Fast Install (Default)**: Automatically checks runtime libraries and downloads the matching pre-compiled release binary (**macOS arm64** or **Linux x86_64**). Ready in 2 seconds!
* **Option 2: Build from Source**: Automatically installs development toolchains and compiles the latest stable release locally with `-O3 -march=native` for maximum hardware performance on your CPU.

> **Unattended / CI Usage**:
> ```bash
> ./install.sh --binary   # Fast pre-built install
> ./install.sh --build    # Local CPU-optimized compilation (latest release tag)
> ./install.sh --head     # Bleeding-edge compilation from 'main' branch
> ```

---

## Platform Support & Roadmap

* **macOS (Apple Silicon & Intel)**: Fully supported via Cocoa + GLFW3.
* **Linux (Wayland & X11)**: Fully supported across Ubuntu, Debian, RHEL, Rocky, Fedora, Arch.
* **Windows (Native)**: **On the way!** Native Windows support and standalone `.exe` installers are currently in active development and will arrive in an upcoming release.

---

## Manual Build Instructions

If you prefer to build manually from the cloned repository:

### macOS

1. **Install GLFW**:
   ```bash
   brew install glfw
   ```

2. **Compile**:
   ```bash
   cd cgx_2.23/src
   make -f Makefile.glfw -j$(sysctl -n hw.ncpu)
   ```

3. **Launch**:
   ```bash
   ../../bin/cgx_glfw
   ```

---

### Linux (Ubuntu, Debian, Fedora, RHEL, Arch)

1. **Install Prerequisites**:
   * **Ubuntu / Debian / Linux Mint**:
     ```bash
     sudo apt-get update && sudo apt-get install -y build-essential libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
     ```
   * **Fedora / RHEL / Rocky**:
     ```bash
     sudo dnf install -y gcc-c++ make glfw-devel mesa-libGL-devel mesa-libGLU-devel
     ```
   * **Arch Linux / Manjaro**:
     ```bash
     sudo pacman -S --needed base-devel glfw-x11 mesa glu
     ```

2. **Compile**:
   ```bash
   cd cgx_2.23/src
   make -f Makefile.glfw -j$(nproc)
   ```

3. **Launch**:
   ```bash
   ../../bin/cgx_glfw
   ```

---

## Controls & Mouse Gestures

| Gesture / Shortcut | Action |
| :--- | :--- |
| **Left Click + Drag** | **Rotate / Orbit 3D Model** (smooth trackball rotation around model center) |
| **Shift + Left Drag** or **Ctrl/Cmd + Drag** | **Pan / Translate** model horizontally & vertically |
| **Alt / Option + Left Drag** | **Smooth Continuous Zoom** |
| **Scroll Wheel** or **Trackpad Pinch** | **Step Zoom** (unbounded zoom range) |
| **Right Click + Drag** | **Pan** (drag threshold $\ge 4\text{px}$) |
| **Right Click** (tap & release) | **Open Multi-Level Cascading Context Menu** |

---

## Popular Commands (Type in Bottom Bar)

| Command | Action |
| :--- | :--- |
| `frame` | Auto-fit and center the 3D model in the viewport |
| `view persp` | Switch to realistic 3D Perspective Projection |
| `view ortho` | Switch to classic Orthographic parallel view |
| `view dark` | Switch viewport to modern Dark Mode (`#0D121A`) |
| `view light` | Switch viewport to classic Light Mode (White) |
| `ds <step> e <comp>` | Load and display specific result dataset (e.g. `ds 4 e 4`) |
| `plot fv all` | Plot filled contour values on entire model |
| `plot f all` | Plot filled surfaces |
| `plot e all` | Plot element wireframe mesh |
| `cmap <palette>` | Change colormap (`coolwarm`, `turbo`, `viridis`, `inferno`, `jet`, `classic`) |
| `anim real` | Start real-time modal or transient animation |
| `send all vtu all` | Export entire model and all time-steps to ParaView `.vtu` & `.pvd` |

---

## Native ParaView VTU/PVD Exporter

Export your CalculiX models directly to ParaView with full result fields:

```text
send <set> vtu [all] [ascii]
```

* **Default Base64 Binary Encoding**: Ultra-compact and fast to load.
* **1-Click Time-History Playback**: Running `send all vtu all` creates individual time-step `.vtu` files and automatically links them into a master ParaView Collection file (`.pvd`).
* **Full Element Support**: Converts TET4, TET10, HEX8, HEX20, WEDGE6, WEDGE15, PYR5, PYR13, TR3, TR6, QUAD4, QUAD8, BEAM2, BEAM3 to canonical VTK cells.

---

## License & Attribution

This project is free and open-source software distributed under the **GNU General Public License Version 2 (GPL-2.0 or later)**, strictly adhering to the original CalculiX licensing.

### Original Authors & Copyright:
* **CalculiX GraphiX (CGX)** is created and copyrighted by **Klaus Wittig** (`klaus.h.wittig@t-online.de`).
* **CalculiX CrunchiX (CCX)** is created and copyrighted by **Dr. Guido Dhondt** (`dhondt@t-online.de`).
* Official CalculiX website: [http://www.calculix.de](http://www.calculix.de) / [https://www.dhondt.de](https://www.dhondt.de)

### Project Maintainer & AI Pairing:
* Modernized by **Carlo Monjaraz-Tec** ([@carlomontec](https://github.com/carlomontec)) in collaboration with **Antigravity (AGY)** as an open-source academic exploration of agent-assisted scientific software modernization.
* **Disclaimer**: This software is provided "AS IS", without warranty of any kind, express or implied. The authors assume no liability for errors, bugs, or damages.
