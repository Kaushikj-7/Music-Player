#include "wav_decoder.h"
WavData WavDecoder::load(const std::string& path){
    std::ifstream file(path,std::ios::binary);
    if(!file.is_open()){
        throw std::runtime_error("Could not open WAV file: " );
    }
    auto read_u32 = [&](uint32_t& out){
        file.read(reinterpret_cast<char*>(&out),4);
    };
    auto read_u16 = [&](uint16_t& out){
        file.read(reinterpret_cast<char*>(&out),2);
    };

    char riff[4];
    file.read(riff, 4);
    std::string(riff, 4)
    if(std::string(riff, 4) != "RIFF"){
        throw std::runtime_error("Not a RIFF file");
    }
    file.ignore(4);
    
    char fmt[4];
    file.read(fmt, 4);
    if (std::string(fmt, 4) != "fmt ")
        throw std::runtime_error("Missing fmt chunk");

    
    uint32_t fmtSize;
    read_u32(fmtSize);
    uint16_t audioFormat;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    read_u16(audioFormat);
    read_u16(channels);
    read_u32(sampleRate);
    read_u32(byteRate);
    read_u16(blockAlign);
    read_u16(bitsPerSample);

    if (audioFormat != 1)
    throw std::runtime_error("Only PCM supported");
    
    char dataHeader[4];
    file.read(dataHeader, 4);
    while(std::string(dataHeader,4)!="data"){
        uint32_t chunkSize;
        read_u32(chunkSize);
        file.ignore(chunkSize);
        file.read(dataHeader, 4);
    }


    uint32_t dataSize;
    read_u32(dataSize);

    std::vector<int16_t> samples(dataSize / 2);
    file.read(reinterpret_cast<char*>(samples.data()), dataSize);

    return WavData{
        channels,
        sampleRate,
        bitsPerSample,
        samples,
        dataSize
    };
}


