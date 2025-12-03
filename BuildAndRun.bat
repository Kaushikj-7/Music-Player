@echo off
cd /d "%~dp0"
echo ==========================================
echo      Music Player - Build & Run
echo ==========================================

echo [1/2] Compiling in WSL...
wsl cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

wsl cmake --build build -j 4
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo [2/2] Launching Application...
wsl bash -c "export PULSE_SERVER=/mnt/wslg/PulseServer; export ALSA_CONFIG_PATH=\"$(wslpath -u '%~dp0asound.conf')\"; ./build/bin/music_player"
