# Third-Party Libraries, Licenses & Credits

This document provides a summary of all third-party libraries and components incorporated into or used by **CalculiX GraphiX (GLFW Edition)**.

---

## Summary Table

| Component / Library | Primary Author(s) | Purpose | License |
| :--- | :--- | :--- | :--- |
| **CalculiX GraphiX (CGX)** | Klaus Wittig | Core FEA Pre/Post-Processor Engine | [GNU GPL v2 or later](https://www.gnu.org/licenses/gpl-2.0.html) |
| **CalculiX CrunchiX (CCX)** | Dr. Guido Dhondt | Solver formats & references | [GNU GPL v2 or later](https://www.gnu.org/licenses/gpl-2.0.html) |
| **GLFW3** | Camilla Löwy & GLFW Contributors | Multi-platform windowing, OpenGL context & input | [zlib/libpng License](https://www.glfw.org/license.html) |
| **stb_image_write** | Sean Barrett | Lossless PNG image export | [Public Domain / MIT](https://github.com/nothings/stb) |
| **stb_truetype** | Sean Barrett | TrueType font rendering engine | [Public Domain / MIT](https://github.com/nothings/stb) |
| **msf_gif** | Miles Fogle | High-performance animated GIF encoder | [Public Domain / MIT](https://github.com/notnullnotvoid/msf_gif) |
| **TetGen** | Hang Si (WIAS Berlin) | 3D Quality Tetrahedral Mesh Generator | [Affero GPL v3](https://wias-berlin.de/software/index.jsp?id=TetGen&lang=1) |
| **libSNL** | Klaus Wittig | Surface & NURBS Geometry Library | [GNU GPL v2](http://www.calculix.de/) |
| **Iosevka Font** | Belleve Invis | Clean vector monospace typography | [SIL Open Font License 1.1](https://github.com/be5invis/Iosevka) |

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
- **License**: AGPL v3 / Research License. Full notice located in `cgx_2.23/src/tetgen/LICENSE`.

### 6. Iosevka Font
- **Author**: Belleve Invis
- **License**: SIL Open Font License, Version 1.1. Full notice located in `cgx_2.23/src/cgx_iosevka_font.c`.
