# project-x

Small C++ audio player/decoder project.

## Overview

This repository contains a simple audio player and WAV decoder implemented in C++ using CMake for building. The source is under `src/` with subfolders for `decoder`, `player`, and `utils`.

## Files of interest

- `CMakeLists.txt` - Top-level CMake file
- `src/main.cpp` - Program entry point
- `src/decoder/wav_decoder.*` - WAV file decoding
- `src/player/*` - Audio playback logic
- `src/utils/logger.*` - Logging utilities

## Prerequisites

- CMake (>= 3.10)
- A C++17-capable compiler (MSVC, MinGW-w64, or clang)
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
