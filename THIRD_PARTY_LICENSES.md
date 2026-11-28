# Third-Party Libraries, Licenses & Credits

This document provides a summary of all third-party libraries and components incorporated into or used by **CalculiX GraphiX (GLFW Edition)**.

---

## Summary Table

| Component / Library | Primary Author(s) | Purpose | License |
| :--- | :--- | :--- | :--- |
| **CalculiX GraphiX (CGX)** | Klaus Wittig | Core FEA pre/post-processor. This project is a modified version of CGX 2.23, see [NOTICE.md](NOTICE.md) | [GNU GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) ([LICENSE](LICENSE)) |
| **CalculiX CrunchiX (CCX)** | Dr. Guido Dhondt | Solver file formats (CCX itself is not bundled) | [GNU GPL v2 or later](https://www.gnu.org/licenses/gpl-2.0.html) |
| **GLFW3** | Camilla Löwy & GLFW Contributors | Multi-platform windowing, OpenGL context & input | [zlib/libpng License](https://www.glfw.org/license.html) |
| **stb_image_write** | Sean Barrett | Lossless PNG image export | [Public Domain / MIT](https://github.com/nothings/stb) |
| **stb_truetype** | Sean Barrett | TrueType font rendering engine | [Public Domain / MIT](https://github.com/nothings/stb) |
| **msf_gif** | Miles Fogle | High-performance animated GIF encoder | [Public Domain / MIT](https://github.com/notnullnotvoid/msf_gif) |
| **TetGen** | Hang Si (WIAS Berlin) | 3D Quality Tetrahedral Mesh Generator | [Affero GPL v3](https://wias-berlin.de/software/index.jsp?id=TetGen&lang=1) |
| **libSNL** | Scott A.E. Lanham | Surface & NURBS geometry library (part of the original CGX 2.23 distribution) | GNU GPL v2 or later ([libSNL/license.txt](libSNL/license.txt)) |
| **GLUT 3.5** | Mark J. Kilgard | Part of the original CGX 2.23 distribution; not compiled in the GLFW build | Freely distributable, not public domain ([glut-3.5/NOTICE](glut-3.5/NOTICE)) |
| **Iosevka Font** | Renzhi Li (Belleve Invis) | Monospace UI font | [SIL Open Font License 1.1](font/OFL.txt) |

---

## Detailed License Notices

### 1. stb_image_write.h
- **Author**: Sean Barrett (nothings)
- **License**: Public Domain (Unlicense) / MIT License
- **Notice**:
```text
This software is available under 2 licenses -- choose whichever you prefer:
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

### 2. msf_gif.h
- **Author**: Miles Fogle (notnullnotvoid)
- **License**: Public Domain (Unlicense) / MIT License
- **Notice**:
```text
Copyright (c) 2025 Miles Fogle
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

### 3. stb_truetype.h
- **Author**: Sean Barrett (nothings)
- **License**: Public Domain (Unlicense) / MIT License
- **Notice**: Full license notice embedded within `cgx_2.23/src/stb_truetype.h`.

### 4. GLFW
- **Authors**: Camilla Löwy and GLFW contributors
- **License**: zlib/libpng License
- **Notice**:
```text
Copyright (c) 2002-2006 Marcus Geelnard
Copyright (c) 2006-2019 Camilla Löwy <elmindreda@glfw.org>
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
```

### 5. TetGen
- **Author**: Hang Si (Weierstrass Institute for Applied Analysis and Stochastics)
- **Website**: https://wias-berlin.de/software/index.jsp?id=TetGen&lang=1
- **License**: The bundled `cgx_2.23/src/tetgen/tetgen.h` (TetGen 1.6.0) states: "It may be copied, modified, and redistributed for non-commercial use. Please consult the file LICENSE for the detailed copyright notices." That LICENSE file is not yet included in this repository. See the TetGen website for the current license terms.

### 6. Iosevka Font
- **Author**: Renzhi Li (aka. Belleve Invis)
- **Copyright**: Copyright 2015-2026, Renzhi Li (aka. Belleve Invis, belleve@typeof.net).
- **License**: SIL Open Font License, Version 1.1. Full text in [`font/OFL.txt`](font/OFL.txt).
- **Files**: the unmodified font file `font/IosevkaSS05-Medium.ttf` (Iosevka SS05 Medium, version 34.8.0), and the same font embedded as a byte array in `cgx_2.23/src/cgx_iosevka_font.c`. The font is bundled with the software, not sold on its own, as the OFL permits.
