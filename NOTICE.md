# Notice

## Origin

This repository contains a modified version of **CalculiX GraphiX (CGX) 2.23**,
the pre- and post-processor of the CalculiX package.

- CalculiX GraphiX: Copyright (C) 1996 Klaus Wittig.
- Original source: the CGX 2.23 release, distributed at [dhondt.de](https://www.dhondt.de) and [calculix.de](https://www.calculix.de).
- The unmodified release was imported in commit `4aaa4df`, "Import CalculiX GraphiX (CGX) 2.23 release from dhondt.de".

This project is not an official CalculiX release. It is not affiliated with or endorsed
by Klaus Wittig, Guido Dhondt or the CalculiX project. Problems found in this edition
should be reported to this repository, not to the CalculiX authors.

## License

CalculiX GraphiX is licensed under the **GNU General Public License, version 2**
("version 2 of the License", as stated in the original source files). The full text is in
[`LICENSE`](LICENSE), identical to [`cgx_2.23/COPYING`](cgx_2.23/COPYING) from the original release.

The modifications in this repository are distributed under the same license, GNU GPL version 2.
They do not change the license of the original work. The copyright notices in the original
source files are kept unchanged.

## Modifications

Modifications since the original release: Copyright (C) 2026 Carlo Monjaraz-Tec.

The main changes are:

- Windowing and input through GLFW3 instead of X11/GLX/GLUT (`cgx_glut_glfw.c`, `cgx_glut_glfw.h`).
- TrueType text rendering with `stb_truetype`.
- VBO/GLSL surface rendering, colour maps, HUD overlays and the model tree.
- VTK XML (`.vtu` / `.pvd`) export.
- Embedded TetGen tetrahedral meshing.
- PNG/GIF/video capture, 2D plotting through Gnuplot, and build, packaging and test infrastructure.

The complete list of changed files, with dates, is in the Git history of this repository:
`git log --stat 4aaa4df..`

## Bundled third-party components

Other bundled components keep their own licenses. See [`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md).
