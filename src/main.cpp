#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "logger.h" 
std::vector<uint8_t> loadFile(const std::string& filename) {
    // std::ifstream: file input stream
    // std::ios::binary: disables text translation (CR/LF)
    // std::ios::ate: open at end to get file size directly
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file) {
        Logger::instance().log(LogLevel::ERROR,
                               "Failed to open input file: " + filename);
        return {};
    }

    // file.tellg() gives file size when opened with ios::ate
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);   // Move cursor back to start

    std::vector<uint8_t> buffer(size);

    // Read raw bytes directly into buffer
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        Logger::instance().log(LogLevel::ERROR,
                               "File read operation failed for: " + filename);
        return {};
    }

    Logger::instance().log(LogLevel::INFO,
                           "Successfully loaded file: " + filename +
                           " (bytes = " + std::to_string(size) + ")");

    return buffer;
}

// ----------------------------------------------------------------------------------
// main() — Application Entry Point
// Every C++ program begins execution here.
// ----------------------------------------------------------------------------------
int main(int argc, char** argv) {
    // Initialize logger output
    Logger::instance().setLogFile("app.log");
    Logger::instance().log(LogLevel::INFO, "Application started.");

    // --------------------------------------------------------------------------
    // Step 1: Validate command-line arguments
    // argc = argument count
    // argv = array of C-style char* strings
    // Example usage: ./app audio.wav
    // --------------------------------------------------------------------------
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <audio_file>\n";
        Logger::instance().log(LogLevel::WARNING,
                               "No input file provided. Exiting.");
        return 1;
    }

    std::string inputFile = argv[1];

    Logger::instance().log(LogLevel::INFO,
                           "Input file received: " + inputFile);

    // --------------------------------------------------------------------------
    // Step 2: Load input audio file into memory
    // This simulates how real decoders handle binary data.
    // --------------------------------------------------------------------------
    std::vector<uint8_t> audioData = loadFile(inputFile);

    if (audioData.empty()) {
        Logger::instance().log(LogLevel::CRITICAL,
                               "Audio data buffer is empty. Terminating app.");
        return 1;
    }

    // --------------------------------------------------------------------------
    // Step 3 (Future Work):
    // Pass buffer → decoder pipeline → PCM output
    //
    // For Stage 1, we only demonstrate the bootstrap step.
    //
    // Example future call:
    // WavDecoder decoder;
    // std::vector<int16_t> pcm = decoder.decode(audioData);
    // --------------------------------------------------------------------------
    Logger::instance().log(LogLevel::INFO,
                           "Decoder pipeline not implemented in Stage 1.");

    // --------------------------------------------------------------------------
    // Step 4: Clean exit
    // --------------------------------------------------------------------------
    Logger::instance().log(LogLevel::INFO,
                           "Application finished successfully.");

    return 0;
}