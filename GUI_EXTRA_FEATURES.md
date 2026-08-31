# CalculiX GraphiX (GLFW Edition) — GUI Extra Features

This document provides a comprehensive guide to the modernized UI/UX enhancements, navigation shortcuts, typography engine, and visual rendering upgrades introduced in **CalculiX GraphiX (GLFW Edition)** compared to CalculiX GraphiX.

---

## 1. 🖥️ Modern Windowing & Cross-Platform Engine
* **100% GLUT-Free & X11-Free**: Replaces legacy X11, GLX, and raw GLUT dependencies with native **GLFW3**.
* **Universal Compatibility**: Runs natively on **macOS** (Apple Silicon / Intel Cocoa), **Linux** (Wayland / X11), and **Windows**.
* **High-DPI / Retina Awareness**: Dynamically queries the framebuffer scale (`fb_scale`) so text, wireframe lines, and node dots render crisp, sharp, and physically proportional across 1x monitors, 4K/5K displays, and Apple Liquid Retina screens.

---

## 2. 🎮 Navigation & Keyboard Modifiers

Navigate and inspect complex 3D FEA models smoothly using standard CAD / Blender mouse and keyboard gestures:

| Shortcut / Gesture | Action | Description |
| :--- | :--- | :--- |
| **Left Click + Drag** | **Rotate / Orbit** | Smooth trackball rotation around the model center |
| **Shift + Left Drag** | **Pan** | Translate the model horizontally and vertically |
| **Ctrl + Left Drag** (or **Cmd + Drag**) | **Pan** | Standard CAD / Mac modifier pan |
| **Alt / Option + Left Drag** | **Zoom** | Smooth continuous dynamic zoom |
| **Ctrl/Cmd + Shift + Left Drag** | **Zoom** | Alternative modifier zoom |
| **Scroll Wheel / Trackpad Pinch** | **Zoom** | Continuous stepped zoom |
| **Right Click + Drag** | **Pan** | Quick right-drag navigation (drag threshold $\ge 4\text{px}$) |
| **Right Click Tap** (release without drag) | **Context Menu** | Opens the cascading context menu at the cursor |

---

## 3. ⌨️ Interactive Command Bar & Smart Fuzzy Suggestions

Located at the bottom of the viewport for rapid command execution without needing to switch between terminal windows:

* **In-Window Prompt**: Click the bottom bar or start typing to enter CGX commands (`frame`, `ds 1 e 4`, `plot f all`, `view sh`, `help`).
* **Command History**: Press <kbd>↑</kbd> and <kbd>↓</kbd> arrow keys to browse through previously entered commands.
* **Smart Fuzzy Matcher**: If you make a typo, an integrated Levenshtein distance matcher automatically suggests the closest valid CGX command:
  ```text
  cgx> fram
   [ERROR] Unknown command 'fram'. Did you mean 'frame'? (Type 'help' for commands)
  
  cgx> zooom
   [ERROR] Unknown command 'zooom'. Did you mean 'zoom'? (Type 'help' for commands)
  ```
* **Visual Status Feedback**: Unrecognized commands and hints display in warm amber text directly inside the bottom command bar for 6 seconds before returning to the placeholder.
* **Toggle Visibility**: Hide or show the bar via **`GUI Settings >`** $\rightarrow$ **`Toggle Command Line Bar`** or command `menu 5`.

---

## 4. 🎨 Modernized Visuals & Rendering Defaults

* **Signature Dark Mode by Default**:
  - Starts in dark slate aesthetic (`#0D121A`) with high-contrast text and mesh lines.
  - Toggle between Dark and Light mode via right-click **`GUI Settings >`** $\rightarrow$ **`Toggle Dark Mode`** or `view bg`.
* **True 3D Perspective Projection**:
  - Perspective projection is active by default on launch (`perspectiveFlag = 1`).
  - Toggle between 3D Perspective and Orthographic parallel view via **`GUI Settings >`** $\rightarrow$ **`Toggle Perspective 3D`** or commands `view persp` / `view ortho`.
* **Stationary 2D HUD Overlays**:
  - The **Color Scale Bar** (`scala_tex`) and **Viewport Ruler** (`drawRuler`) are encapsulated in dedicated 2D orthographic projection passes with full OpenGL state isolation (`glPushAttrib`), completely unaffected by model rotation or aspect ratio resizing.
* **Refined Wireframes & High-DPI Nodes**:
  - Element wireframe lines render with anti-aliased smoothing (`GL_LINE_SMOOTH`).
  - Node and geometry points render as 50% larger, anti-aliased round dots (`GL_POINT_SMOOTH`) for clear visibility on Retina screens.

---

## 5. 🖋️ Anti-Aliased Vector Typography

* Built-in vector typography engine powered by [`stb_truetype`](https://github.com/nothings/stb) rendering embedded **Iosevka SS05** font atlas.
* **3-Tier Dynamic Font Sizing**:
  - **Small (14pt)** — Compact for dense datasets.
  - **Medium (20pt / Default)** — Comfortable modern reading.
  - **Big (32pt)** — Large scale for 4K / presentation viewports.
  - Switch anytime via right-click **`GUI Settings >`** $\rightarrow$ **`Text Size >`**.

---

## 6. 🚀 Modern GPU Pipeline (VBO + GLSL Shaders)

CalculiX GraphiX includes an optimized **Modern GPU Pipeline** designed to eliminate CPU bottlenecks on massive multi-element models:

* **1 Single Draw Call (`glDrawArrays`)**: Geometry and field results are uploaded into dedicated Vertex Buffer Objects (VBOs) in GPU VRAM, rendering the entire 3D model in a single instruction.
* **Blinn-Phong Metallic Surface Shading**: Realistic specular glints ($\alpha = 96.0$, strength $= 0.38$) provide tactile 3D surface depth on CAD/FEA geometries.
* **Exact Quadratic Element Accuracy**: Supports full-fidelity sub-triangle decomposition for high-order quadratic elements (**Quad8**, **Tri6**, **Quad4**).
* **Dual-Pipeline Runtime Switch**: Toggle between the Modern GPU Pipeline and Klaus Wittig's legacy display lists anytime via **`GUI Settings >`** $\rightarrow$ **`[Experimental] Modern GPU Pipeline`**.

> 📖 **Architecture Deep Dive**: For complete technical specifications of the buffer layouts and shader code, see [**MODERN_3D_PIPELINE.md**](MODERN_3D_PIPELINE.md).

---

## 7. 📊 ParaView-Style Logarithmic Color Scale

Visualize physical quantities spanning multiple orders of magnitude (e.g. plastic strains, crack-tip stress concentrations, acoustic fields):

* **1-Click Menu Toggle**: Right-click $\rightarrow$ **`Viewing >`** $\rightarrow$ **`Toggle Logarithmic Scale`**.
* **Automatic Positive Range Floor**: If data contains zeros or negative numbers ($v \le 0$), the engine automatically establishes a positive floor ($\max(v_{\text{pos,min}}, v_{\max} \times 10^{-4})$), maps non-positive values to the bottom color band, and displays informative terminal diagnostics.
* **Base-10 Power Labels (`10^x`)**: The 2D color scale bar dynamically renders decade ticks in clear mathematical power notation (`10^1`, `10^2`, `10^3`, `10^-2`).

---

## 8. 🌈 Perceptual Scientific Colormaps

* **`cubehelix (reversed)` (Default)**:
  - Developed by **D. A. Green (2011)** (*Cavendish Laboratory, Cambridge*), Cubehelix is designed with strictly monotonic perceived lightness.
  - **Black & White Print Safe**: When printed or photocopied in grayscale, the gradient converts to a perfectly monotonic linear brightness profile.
  - **Colorblind Friendly**: Eliminates artificial color boundaries common in classic Jet/Rainbow palettes.
* **Colormap Palette Suite**:
  - `cubehelix (reversed)` (Default)
  - `cubehelix` (Standard)
  - `viridis`
  - `turbo`
  - `inferno`
  - `coolwarm`
  - `jet`
  - `classic`
  - `gray`
  - Switch anytime via **`Viewing >`** $\rightarrow$ **`Colormap >`** or the command bar (`cmap <name>`).
