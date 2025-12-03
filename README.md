# 🎵 Modern C++ Music Player

A sleek, high-performance music player built with C++17, featuring a modern dark UI, volume boosting, and variable playback speed. Designed for efficiency and audio clarity.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20WSL-lightgrey.svg)
![C++](https://img.shields.io/badge/std-c%2B%2B17-blue.svg)

## ✨ Features

- **Modern Dark UI**: Inspired by popular media players, featuring a semi-transparent, distraction-free interface.
- **Volume Boost**: Software-driven volume amplification up to **200%** with soft-clipping protection to prevent distortion on laptop speakers.
- **Variable Playback Speed**: Real-time speed adjustment (0.75x, 1.0x, 1.5x, 2.0x) without pitch alteration.
- **Format Support**: Plays MP3, WAV, FLAC, OGG, and more (powered by FFmpeg).
- **Playlist Management**: Automatically scans the current directory for audio files.
- **Gapless Looping**: Seamless track looping for continuous playback.

## 🛠️ Tech Stack

- **Language**: C++17
- **GUI**: [Dear ImGui](https://github.com/ocornut/imgui) + [SDL2](https://www.libsdl.org/) + OpenGL 3
- **Decoding**: [FFmpeg](https://ffmpeg.org/) (libavcodec, libavformat, libswresample)
- **Audio Output**: [PortAudio](http://www.portaudio.com/) (Low-latency, callback-driven)
- **Build System**: CMake

## 📦 Prerequisites

Ensure you have the following development libraries installed.

**Ubuntu / Debian / WSL:**

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake \
    libsdl2-dev libgl1-mesa-dev \
    libavcodec-dev libavformat-dev libavutil-dev libswresample-dev \
    portaudio19-dev
```

## 🚀 Building & Installation

### 1. Clone the Repository

```bash
git clone https://github.com/Kaushikj-7/Music-Player.git
cd Music-Player
```

### 2. Build from Source

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### 3. Run

```bash
./bin/music_player
# Or load a specific file:
./bin/music_player song.mp3
```

## 📦 Creating a Portable Release

To create a standalone, optimized distribution package (tarball):

```bash
./package_release.sh
```

This will generate `music_player_linux_x64.tar.gz` containing the binary and necessary scripts.

## 🎮 Controls

| Control           | Action                              |
| :---------------- | :---------------------------------- |
| **Play / Pause**  | Toggle playback                     |
| **Stop**          | Stop playback and reset cursor      |
| **Volume Slider** | Adjust volume (0% - 200%)           |
| **Speed Buttons** | Change playback rate (0.75x - 2.0x) |
| **Loop Checkbox** | Repeat current track indefinitely   |
| **Playlist**      | Click any file to play immediately  |

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

_Developed by Kaushik J._

- Git (for cloning and pushing)

## Build (recommended)

Open PowerShell and run (from the project root):

```powershell
mkdir build; cd build
cmake ..
cmake --build . --config Release
```

Notes:

- On Windows with Visual Studio installed, CMake will select the Visual Studio generator by default.
- To use MinGW (MSYS2/MinGW-w64), run `cmake .. -G "MinGW Makefiles"` and then `cmake --build .`.

## Run

After building, the executable should be in the `build` folder (or under a config subfolder like `Release`). Run the produced binary from the command line, for example:

```powershell
.\path\to\executable.exe
```

## Contributing

Feel free to open issues or submit pull requests. Keep changes focused and add small, testable commits.

## License

Add your license here (e.g., MIT). If you don't want to add a license, mention that the repository has no license yet.

---

_This README was added by an automated assistant. Update it as needed._
