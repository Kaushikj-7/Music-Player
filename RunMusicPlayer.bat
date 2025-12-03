@echo off
cd /d "%~dp0"

:: Ensure the binary is executable
wsl chmod +x ./build/bin/music_player

:: Check if a file was dragged onto the script or passed as an argument
if "%~1"=="" (
    :: No arguments, just run the player
    wsl bash -c "export PULSE_SERVER=/mnt/wslg/PulseServer; export ALSA_CONFIG_PATH=\"$(wslpath -u '%~dp0asound.conf')\"; ./build/bin/music_player"
) else (
    :: Argument provided (e.g. "Open With"), convert path to WSL format and run
    wsl bash -c "export PULSE_SERVER=/mnt/wslg/PulseServer; export ALSA_CONFIG_PATH=\"$(wslpath -u '%~dp0asound.conf')\"; ./build/bin/music_player \"$(wslpath -u '%~1')\""
)
