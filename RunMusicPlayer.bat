@echo off
cd /d "%~dp0"
wsl chmod +x ./build/bin/music_player
wsl ./build/bin/music_player %*
