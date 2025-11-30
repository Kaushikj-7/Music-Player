#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <fstream>
struct WavData{
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    std::vector<int16_t> samples;
    uint32_t data_size;

};

class WavDecoder{
    public: 
     static WavData load(const std::string& path);

};

