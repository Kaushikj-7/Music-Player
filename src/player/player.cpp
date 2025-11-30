#include "player.h"
#include <iostream>
#include <chrono>

void MockPlayer::play(const WavData& data){
    const auto& samples=data.samples;
    uint32_t sampleRate = data.sampleRate;
    double frameTimeMs= 1000.0/sampleRate;

    std::cout << "Simulating playback..." << std::endl;
    std::cout << "Channels: " << data.channels << "\n";
    std::cout << "Sample Rate: " << sampleRate << " Hz\n";
    std::cout << "Bits per Sample: " << data.bitsPerSample << "\n";
    std::cout << "Total Samples: " << samples.size() << "\n";
     for (size_t i = 0; i < samples.size(); ++i) {
            std::this_thread::sleep_for(
            std::chrono::microseconds((int)(frameTimeMs * 1000))
        );
    }
    
    std::cout << "Playback simulation finished.\n";
}