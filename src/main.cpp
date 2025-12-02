/**
 * main.cpp
 *
 * Demonstrates a simple CLI: load a file, play it, wait for Enter to stop.
 * This binds together the Player class and system init.
 */

#include "player/player.h"
#include "utils/logger.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    Logger::instance().setLogFile("music_player.log");
    Logger::instance().log(LogLevel::INFO, "App started");

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audio-file>\n";
        return 1;
    }

    std::string path = argv[1];

    Player player;
    if (!player.load(path)) {
        Logger::instance().log(LogLevel::ERROR, "Main: Failed to load file: " + path);
        return 1;
    }

    if (!player.play()) {
        Logger::instance().log(LogLevel::ERROR, "Main: Failed to start playback");
        return 1;
    }

    std::cout << "Playing. Press ENTER to stop...\n";
    std::string dummy;
    std::getline(std::cin, dummy);

    player.stop();
    Logger::instance().log(LogLevel::INFO, "App exiting");
    return 0;
}
