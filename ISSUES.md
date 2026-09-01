# CalculiX GraphiX (GLFW Edition) — Issue Tracker

This document records confirmed issues, root causes, and planned fixes for upcoming development sessions.

---

## 📌 Issue #001: Mesh Element Edge Lines are Too Thin / Barely Visible on High-DPI Displays

### 1. Description
When displaying element edges (`view elem`, `plot e all`, `view edge`, `view sh`), the wireframe lines defining the mesh boundaries are rendered with a fixed single-pixel line width. On high-resolution displays (Apple Retina, 2K/4K Linux/Windows screens), these lines appear as faint, sub-pixel hairlines that are difficult to distinguish against shaded surfaces.

### 2. Suspected Root Cause
* In OpenGL rendering routines ([`dispLists.c`](cgx_2.23/src/dispLists.c), [`plotFunktions.c`](cgx_2.23/src/plotFunktions.c), [`extGL.c`](cgx_2.23/src/extGL.c)), `glLineWidth()` defaults to `1.0f` without multiplying by the framebuffer scale factor (`fb_scale = fb_width / window_width`).
* Anti-aliasing (`GL_LINE_SMOOTH`) is not consistently enabled during wireframe passes.

### 3. Proposed Fix
1. **Dynamic DPI Scaling**: Scale `glLineWidth()` by the active window `fb_scale` (e.g. `glLineWidth(1.5f * fb_scale)` or `2.0f * fb_scale`).
2. **Smooth Line Rasterization**: Enable `glEnable(GL_LINE_SMOOTH)` with proper alpha blending during wireframe element overlays.
3. **Menu Option / Setting**: Add a line width preference to `GUI Settings >` submenu or auto-scale with the active UI text size tier.

---

## 📌 Issue #002: Model Disappears / Viewport Lost on Window Maximize (Unrecoverable via `frame`)

### 1. Description
When maximizing the application window or performing a large instant resize, the 3D model disappears completely from the viewport. Executing the `frame` command (or pressing the `f` hotkey to re-center the model) fails to bring the geometry back into view.

### 2. Suspected Root Cause
* **Coordinate Discrepancy in GLFW Resize Callbacks**:
  In [`cgx_glut_glfw.c`](cgx_2.23/src/cgx_glut_glfw.c) and [`cgx.c`](cgx_2.23/src/cgx.c) (`reshapeFunc`), `glViewport(0, 0, width, height)` and the perspective/orthographic projection matrix calculations receive inconsistent units (framebuffer pixels vs. window screen points) during the rapid maximize event.
* **Aspect Ratio / Near-Far Plane Corruption**:
  An intermediate zero or near-zero dimension during the maximize transition can cause division by zero in aspect ratio calculations (`width / height`), resulting in `NaN` or `Inf` in the OpenGL projection/modelview matrix stack. Once `NaN` enters the matrix stack, `frame()` matrix computations continue to produce `NaN`, leaving the model invisible.

### 3. Proposed Fix
1. **Sanitize Dimensions**: In `glfw_window_size_callback` and `glfw_framebuffer_size_callback`, enforce `if (width <= 0) width = 1; if (height <= 0) height = 1;`.
2. **Synchronize Viewport with Framebuffer**: Always query `glfwGetFramebufferSize()` directly inside `reshapeFunc()` before setting `glViewport()` and rebuilding projection matrices.
3. **Defensive Matrix Reset in `frame()`**: In `frame()`, explicitly reset and sanitize the ModelView and Projection matrices (`glMatrixMode(GL_PROJECTION); glLoadIdentity(); ...`) to guarantee recovery from any previous invalid state.

---

## 📌 Issue #003: Secondary / Right-Click Mouse Drag Fails and Conflicts with Context Menu

### 1. Description
Attempting to pan or zoom the model by dragging with the secondary / right mouse button (or trackpad equivalent) does not work. Instead, pressing the button immediately intercepts the input and opens the cascading context menu over the viewport, preventing the drag action from reaching CGX's model navigation routines.

### 2. Suspected Root Cause
* In [`cgx_glut_glfw.c`](cgx_2.23/src/cgx_glut_glfw.c) (`glfw_mouse_button_callback`), `GLUT_RIGHT_BUTTON` on `GLUT_DOWN` unconditionally opens the cascade menu and immediately returns before dispatching the event to `win->mouse_func`:
  ```c
  if (glut_btn == GLUT_RIGHT_BUTTON && glut_state == GLUT_DOWN)
  {
    open_cascade_root(...);
    return;
  }
  ```
* Because mouse-down is swallowed, `motionFunc` never initiates viewport translation/zoom dragging for secondary button actions.

### 3. Proposed Fix
1. **Click vs Drag Discrimination**:
   - Record the mouse-down position on right-click without immediately opening the menu.
   - If the user moves the mouse past a drag threshold ($\Delta > 3\text{px}$), treat the gesture as a model drag and forward movement to `win->motion_func`.
   - If the user releases the button within the threshold without dragging, trigger `open_cascade_root()` on mouse-up (`GLUT_UP`).
2. **Support Middle-Click Drag / Modifiers**: Ensure standard 3-button mouse mappings (Left: Rotate, Middle: Pan, Right: Menu/Zoom, Shift+Left: Pan) function cleanly across macOS, Linux, and Windows.

---

## 📌 Issue #004: `frame` Zoom Centers Too Close and Zooms Out from Off-Center Origin

### 1. Description
Executing the `frame` command (or pressing `f`) often positions the camera excessively close to the geometry. Furthermore, zooming out moves the camera away from an off-center or distant anchor point rather than zooming out symmetrically from the geometric centroid / bounding box center of the structure.

### 2. Suspected Root Cause
* In `cgx.c` (`frame()`) and `extGL.c` (`moveModel()`):
  - In default perspective 3D projection, `ds` (zoom scale) is hardcoded to `0.5` without taking into account the field of view ($\theta = 45^\circ$), aspect ratio, and diagonal extents of the model bounding box.
  - When rotating and zooming, the camera orbits around a coordinate system offset rather than the true center of the active bounding box `(scale->x, scale->y, scale->z)`.
  - In `frame()`, `dtx=0; dty=0; dtz=0; ds=0.5;` assumes the model is normalized exactly between $[-1, 1]$ across all 3 axes, but asymmetric geometries have non-zero offsets that shift the apparent focal point away from the center of visual mass.

### 3. Proposed Fix
1. **Fit-to-View Bounding Box Extents**:
   - Compute the model bounding box diameter $D = \sqrt{\Delta x^2 + \Delta y^2 + \Delta z^2}$ and center $C = (x_{mid}, y_{mid}, z_{mid})$.
   - In perspective projection, calculate optimal camera distance $Z_{dist} = \frac{D / 2}{\tan(\text{FOV} / 2) \cdot \text{margin}}$ (with a comfortable $1.15\times$ margin) so the entire structure fits in view comfortably.
2. **Focal Center Alignment**: Ensure zoom and orbit transformations are anchored directly at $C$, so zooming in/out stays locked onto the model center.

---

## 📌 Issue #005: 3D Viewport / Frame Lost During Window Resizing (Perspective Aspect Ratio Desync)

### 1. Description
Resizing the application window interactively (e.g. stretching borders or changing window dimensions) causes the 3D model framing and camera view to become distorted, shift off-screen, or vanish. The effect appears particularly pronounced in perspective 3D projection mode.

### 2. Suspected Root Cause
* In `cgx.c` (`reshapeFunc`) and `cgx_glut_glfw.c`:
  - Interactive resize events trigger continuous aspect ratio and projection matrix recalculations. In perspective 3D mode (`perspectiveFlag = 1`), intermediate viewport dimensions during rapid drag-resizes can cause the aspect ratio to diverge or matrix state drift to accumulate.
  - The camera orbit distance and center are not properly preserved across window resize callbacks.

### 3. Proposed Fix
1. **Synchronized Resize Pipeline**: Ensure `reshapeFunc` recalculates the aspect ratio strictly from current framebuffer pixel dimensions (`fb_w / fb_h`).
2. **Stable Frustum Preservation**: In perspective projection mode, maintain the camera focal point and distance relative to the bounding box rather than resetting or drifting on resize events.
3. **Continuous Redisplay**: Ensure GLFW resize callbacks invoke clean projection updates followed by `glutPostRedisplay()`.

---

## 📌 Issue #006: Color Scale Bar and Numerical Legend Distort on Window Resize

### 1. Description
When changing the window size or aspect ratio, the colormap scale bar (`scala_tex`) and its associated numerical legend labels become misaligned, overlap with geometry or the bar, stretch unnaturally, or get pushed out of the visible screen area.

### 2. Suspected Root Cause
* In `extGL.c` (`scala_tex()`) and `cgx.c`:
  - The scale bar positions `(dx, dy, kb, kh)` are calculated using raw normalized device coordinate heuristics `(1-y)/2` rather than an isolated, DPI-aware 2D orthogonal HUD overlay.
  - As the aspect ratio changes, `glRasterPos2d` positions float unpredictably across the 3D viewport instead of staying pinned to screen corners.

### 3. Proposed Fix
1. **Isolated 2D HUD Projection**: Wrap `scala_tex()` in a dedicated 2D screen-space pixel projection (`glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, win_w, 0, win_h, -1, 1);`).
2. **Fixed Margin & Sizing**: Pin the color scale bar with fixed pixel dimensions (e.g., 20px wide, 260px tall, positioned at a comfortable margin from the left/top border) regardless of window aspect ratio.
3. **Crisp Typography Layout**: Render numerical values and legend titles using `stb_truetype` with consistent right-aligned baseline positioning.

---

## 📌 Issue #007: Shaded Results Toggle (`view sh`) Not Working & Default Startup State Should Be Shading OFF

### 1. Description
1. Attempting to toggle shaded results (via menu or `view sh` / `view nosh` command) does not properly switch the shading/lighting mode for FEA result scalar maps in the viewport.
2. The application currently defaults to having surface lighting/shading ON for result plots, which can darken or wash out pure colormap scalar data. The desired default on startup is **Shading OFF** (pure, unshaded high-contrast colormap display), allowing users to selectively enable 3D metallic/diffuse shading via the menu when needed.

### 2. Suspected Root Cause
* In `cgx.c` and `extGL.c` (`dispLists.c`):
  - `shadingFlag` / `lightFlag` state toggles do not force a complete display list regeneration (`updateDispLists()`) or OpenGL material/lighting state update for result texture maps.
  - Initialization sequence in `initModel()` sets `shadingFlag = 1` by default.

### 3. Proposed Fix
1. **Set Default Startup State**: Change initial setting in `initModel()` / `cgx.c` so `shadingFlag = 0` (Shade Results OFF by default).
2. **Robust Toggle Synchronization**: Ensure `view sh` / `view nosh` and GUI menu toggles update `shadingFlag`, call `updateDispLists()`, and cleanly enable/disable `GL_LIGHTING` on the active colormap shader pass.

---

*Created on 2026-08-29 during TetGen in-memory meshing modernization.*
