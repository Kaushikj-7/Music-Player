@echo off
cd /d "%~dp0"

:: Ensure the binary is executable
wsl chmod +x ./build_release/bin/music_player

:: Copy config to temp to avoid space-in-path issues with ALSA
wsl cp ./asound.conf /tmp/asound.conf

if "%~1"=="" goto NoArgs

:WithArgs
echo Opening file: %~1
wsl bash -c "export PULSE_SERVER=/mnt/wslg/PulseServer; export ALSA_CONFIG_PATH=/tmp/asound.conf; ./build_release/bin/music_player \"$(wslpath -u '%~1')\""
if %ERRORLEVEL% NEQ 0 pause
goto End

:NoArgs
echo Starting Music Player...
wsl bash -c "export PULSE_SERVER=/mnt/wslg/PulseServer; export ALSA_CONFIG_PATH=/tmp/asound.conf; ./build_release/bin/music_player"
if %ERRORLEVEL% NEQ 0 pause
goto End

:End
