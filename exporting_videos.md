# Exporting Hardcopies, Screenshots & Movies in CGX (GLFW Edition)

In CalculiX GraphiX (GLFW Edition), screenshot captures and movie recordings are written directly from the OpenGL framebuffer to PNG, MP4, GIF, and OGV formats without requiring ImageMagick.

---

## 1. Screenshots & Hardcopy (`hcpy`)

Screenshots are saved directly in **PNG** format without external tools or dependencies.

### Command Usage:
```text
hcpy                # Saves hcpy_1.png, hcpy_2.png, ...
hcpy <filename>     # Saves <filename>.png
hcpy clean          # Deletes session hcpy_*.png files
```

### GUI Menu:
- Right-click or open main menu -> **`Hardcopy -> Save PNG Screenshot`**.

---

## 2. Movie & Video Recording (`movie`)

CGX supports video recording to **MP4 (H.264)**, **OGV (Theora)**, **WebM (VP9)**, and **animated GIFs**.

### Supported Formats:
| Format | Extension | Requirements | Description |
| :--- | :--- | :--- | :--- |
| **MP4** | `.mp4` | `ffmpeg` | Universal H.264 video format |
| **GIF** | `.gif` | None | Built-in pure-C animated GIF encoder |
| **OGV** | `.ogv` | `ffmpeg` | Ogg Theora video format |
| **WebM** | `.webm` | `ffmpeg` | WebM VP9 video format |

### Command Usage:
```text
movie cycle [name.mp4|name.gif] # Records 1 full oscillation/sequence cycle matching on-screen timing
movie start [filename.mp4]      # Starts continuous recording (defaults to movie.mp4)
movie start my_anim.gif         # Starts continuous recording of animated GIF
movie frames <N> [name.mp4]     # Records N frames and stops automatically
movie delay <sec>               # Sets frame delay / framerate (e.g. 0.04 for 25 fps)
movie stop                      # Stops active recording and saves output
movie clean                     # Deletes default movie.* files
```

### 1-Cycle Recording (`movie cycle`):
- **Harmonic Vibration (`anim real`)**: Uses the configured number of angular oscillation steps ($N = \text{anim\_steps}$) and calculates the framerate to match the on-screen animation period ($\text{FPS} = \frac{\text{anim\_steps}}{\text{time\_per\_period}}$).
- **Sequence Animation (`anim seq`)**: Records all $N$ time steps in the active dataset sequence.
- **Phase Sync**: Resets the animation index to 0 at start and automatically saves the output when frame $N$ is reached.

### GUI Menu:
- **`Hardcopy -> Save PNG Screenshot`**: Saves current view to PNG.
- **`Hardcopy -> Record 1 Cycle (MP4)`**: Records 1 full period to `movie_cycle.mp4`.
- **`Hardcopy -> Record 1 Cycle (GIF)`**: Records 1 full period to `movie_cycle.gif`.
- **`Hardcopy -> Record Continuous MP4 Video`**: Streams to `movie.mp4` until stopped.
- **`Hardcopy -> Record Continuous GIF Movie`**: Streams to `movie.gif` until stopped.
- **`Hardcopy -> Stop Recording`**: Finalizes the active recording.

---

## 3. 🛠️ Installing `ffmpeg` for MP4/OGV/WebM Video Recording

While **PNG screenshots** and **animated GIFs** work out-of-the-box with **zero external dependencies**, generating `.mp4` / `.ogv` / `.webm` videos uses `ffmpeg` for ultra-fast, real-time GPU/CPU encoding without temporary files.

### macOS
Install via [Homebrew](https://brew.sh):
```bash
brew install ffmpeg
```

### Linux (Ubuntu, Debian, Fedora, Arch)
```bash
# Ubuntu / Debian
sudo apt update && sudo apt install -y ffmpeg

# Fedora / RHEL
sudo dnf install -y ffmpeg

# Arch Linux / Manjaro
sudo pacman -S --needed ffmpeg
```

### Windows (10 & 11)
Windows users have multiple quick options:
1. **Microsoft Package Manager (winget)**:
   ```powershell
   winget install Gyan.FFmpeg
   ```
2. **Chocolatey / Scoop**:
   ```powershell
   choco install ffmpeg
   # or
   scoop install ffmpeg
   ```
3. **Standalone Binary**:
   - Download static `ffmpeg.exe` from [gyan.dev/ffmpeg/builds](https://www.gyan.dev/ffmpeg/builds/) or [ffmpeg.org](https://ffmpeg.org).
   - Drop `ffmpeg.exe` either into your system `PATH` or directly in the same folder as `cgx_glfw.exe`.
