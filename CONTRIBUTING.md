# Contributing

Bug reports, fixes and forks are welcome. This file explains how to build, test and submit changes.

## Background

This repository is a modified version of **CalculiX GraphiX (CGX) 2.23** by Klaus Wittig
(see [NOTICE.md](NOTICE.md)). It is not an official CalculiX release. Please report problems
with this edition here, not to the CalculiX authors.

## Building

Install the prerequisites for your platform as described in [INSTALL.md](INSTALL.md)
(GLFW 3, CMake, a C/C++ compiler and OpenGL headers). Then build with either:

```bash
# CMake (used by CI)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

```bash
# Standalone Makefile
make -C cgx_2.23/src -f Makefile.glfw -j4
```

Both write the binary to `bin/cgx_glfw`. Quick check:

```bash
./bin/cgx_glfw test/beam_modal.frd
```

## Testing

Run the headless integration tests before opening a pull request:

```bash
python3 tests/run_tests.py
```

or, after a CMake build:

```bash
ctest --test-dir build --output-on-failure
```

GitHub Actions (`.github/workflows/ci.yml`) builds and tests on macOS arm64, Linux x86_64,
Linux ARM64 and Windows (MinGW-w64) for every pull request. In your own fork you can start
the workflow on any branch:

```bash
gh workflow run ci.yml --ref <branch>
```

## Code guidelines

- **Compatibility:** keep CGX batch commands, `.fbl` / `.fbd` scripts and the `.frd`, `.inp`,
  `.stl` and `.step` readers backward compatible. Existing scripts must keep working.
- **Portability:** core CGX code must not use Cocoa, Win32 or X11 APIs directly. Windowing,
  input and menus go through the GLFW layer (`cgx_glut_glfw.c` / `cgx_glut_glfw.h`) and
  standard OpenGL.
- **OpenGL state:** 2D overlays (colour bar, ruler, command bar, menus, HUD) must save and
  restore OpenGL state with `glPushAttrib(GL_ALL_ATTRIB_BITS)` / `glPopAttrib()`, and must
  not leave a changed projection matrix, depth test or blending mode behind.
- **Original code:** keep the copyright and license headers of existing files. When you change
  an original CGX file, keep its structure and naming unless there is a clear reason not to.
- **New files:** use the GNU GPL version 2 header from the existing source files with your own
  copyright line.
- **Memory safety:** check array bounds when walking element and face node lists. QUAD8 faces
  have only `nod[0..7]`. Use 64-bit file offsets for large result files.

## Pull requests

- Base pull requests on `main` and keep each one focused on a single change.
- Describe what changed, why, and how you tested it, including which platforms you checked.
- Reference the issue number if there is one (`Fixes #NN`).
- Do not commit build output, solver results or private model files. `.gitignore` covers the
  usual cases.

## License

By contributing, you agree that your contribution is distributed under the GNU General Public
License, version 2, the same license as CalculiX GraphiX (see [LICENSE](LICENSE)).
