/*
 audio_output.cpp

 Implementation of AudioOutput using PortAudio.

 Key design goals:
  - PortAudio callback is minimal & real-time safe.
  - No locks inside callback. Uses atomic head/tail indices only.
  - Producer (decoder thread or main thread) converts samples to float and calls write().
  - The callback will output silence if buffer underruns to avoid popping.

 Build requirements:
  - Link with portaudio (libportaudio). Use pkg-config or system package manager:
    Ubuntu: sudo apt-get install libportaudio2 libportaudiocpp0 portaudio19-dev
    macOS: brew install portaudio
    Windows: install PortAudio and point CMake to the library.

  - CMake snippet to find portaudio is provided later.
*/

#include "audio_output.h"
#include "../utils/logger.h"   // use existing Logger
#include <portaudio.h>
#include <string>
#include <cstring>             // memcpy
#include <algorithm>           // std::min
#include <cmath>               // for next pow2 (if needed)

// -----------------------------
// Utility: nextPowerOfTwo
// -----------------------------
size_t AudioOutput::nextPowerOfTwo(size_t v) {
    if (v == 0) return 1;
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
    v |= v >> 32;
#endif
    return ++v;
}

// -----------------------------
// Constructor / Destructor
// -----------------------------
AudioOutput::AudioOutput()
    : capacityFrames_(0),
      channels_(0),
      head_(0),
      tail_(0),
      stream_(nullptr),
      sampleRate_(0),
      framesPerBuffer_(0),
      dummyMode_(false),
      volume_(1.0f)
{}

AudioOutput::~AudioOutput() {
    stop();
    Pa_Terminate(); // safe to call even if not initialized
}

// -----------------------------
// init()
//  - allocate ring buffer sized (in frames).
//  - open PortAudio (paInitialize) and configure stream parameters.
// -----------------------------
bool AudioOutput::init(int sampleRate, int channels, unsigned long framesPerBuffer) {
    sampleRate_ = sampleRate;
    channels_ = channels;
    framesPerBuffer_ = framesPerBuffer;

    // Choose ring buffer capacity (in frames). Use a medium buffer to allow some margin.
    // Example: 2 seconds worth of audio at 44100 = 88200 frames.
    size_t desiredFrames = static_cast<size_t>(sampleRate) * 2; // 2 seconds
    capacityFrames_ = nextPowerOfTwo(desiredFrames);

    // allocate interleaved buffer: capacityFrames * channels floats
    buffer_.assign(capacityFrames_ * channels_, 0.0f);

    head_.store(0);
    tail_.store(0);

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        Logger::instance().log(LogLevel::ERROR, std::string("PortAudio init failed: ") + Pa_GetErrorText(err));
        return false;
    }

    // Setup PortAudio stream parameters
    PaStreamParameters outParams;
    std::memset(&outParams, 0, sizeof(outParams));
    outParams.device = Pa_GetDefaultOutputDevice();
    if (outParams.device == paNoDevice) {
        Logger::instance().log(LogLevel::WARNING, "No default output device. Falling back to DUMMY mode (no sound).");
        dummyMode_ = true;
        return true;
    }
    outParams.channelCount = channels_;
    outParams.sampleFormat = paFloat32; // we write float samples
    
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(outParams.device);
    Logger::instance().log(LogLevel::INFO, std::string("Using Audio Device: ") + deviceInfo->name);
    outParams.suggestedLatency = deviceInfo->defaultHighOutputLatency;

    outParams.hostApiSpecificStreamInfo = nullptr;

    // Open stream with static callback lambda wrapper
    err = Pa_OpenStream(
        &stream_,
        nullptr,                    // no input
        &outParams,
        sampleRate_,
        framesPerBuffer_,
        paClipOff,
        // Callback function
        [](const void* inputBuffer, void* outputBuffer,
           unsigned long framesPerBuffer,
           const PaStreamCallbackTimeInfo* timeInfo,
           PaStreamCallbackFlags statusFlags,
           void* userData) -> int
        {
            // Cast user data to our AudioOutput instance
            AudioOutput* out = reinterpret_cast<AudioOutput*>(userData);
            float* outBuf = reinterpret_cast<float*>(outputBuffer);

            // Get indices in frames
            size_t tail = out->tail_.load(std::memory_order_acquire);
            size_t head = out->head_.load(std::memory_order_acquire);

            size_t availableFrames = (head - tail) & (out->capacityFrames_ - 1);

            // If not enough frames, we may underflow: output silence for missing frames
            size_t framesToRead = std::min<size_t>(framesPerBuffer, availableFrames);

            // Read framesToRead frames from ring buffer into outBuf
            float vol = out->volume_.load(std::memory_order_relaxed);
            for (size_t f = 0; f < framesToRead; ++f) {
                size_t idx = ((tail + f) & (out->capacityFrames_ - 1)) * out->channels_;
                // copy channels_
                for (int c = 0; c < out->channels_; ++c) {
                    float sample = out->buffer_[idx + c] * vol;
                    // Soft-clip / Clamp to [-1.0, 1.0] to allow volume boost without wrapping
                    if (sample > 1.0f) sample = 1.0f;
                    else if (sample < -1.0f) sample = -1.0f;
                    outBuf[f * out->channels_ + c] = sample;
                }
            }

            // Fill the rest with silence if underrun
            if (framesToRead < framesPerBuffer) {
                size_t start = framesToRead * out->channels_;
                size_t silenceSamples = (framesPerBuffer - framesToRead) * out->channels_;
                std::memset(outBuf + start, 0, silenceSamples * sizeof(float));
            }

            // Advance tail atomically by framesToRead
            out->tail_.store((tail + framesToRead) & (out->capacityFrames_ - 1), std::memory_order_release);

            // Continue streaming
            return paContinue;
        },
        this // userData
    );

    if (err != paNoError) {
        Logger::instance().log(LogLevel::ERROR, std::string("Pa_OpenStream failed: ") + Pa_GetErrorText(err));
        return false;
    }

    Logger::instance().log(LogLevel::INFO, "AudioOutput initialized");
    return true;
}

// -----------------------------
// start() / stop()
// -----------------------------
bool AudioOutput::start() {
    if (dummyMode_) {
        Logger::instance().log(LogLevel::INFO, "AudioOutput started (DUMMY mode)");
        return true;
    }
    if (!stream_) {
        Logger::instance().log(LogLevel::ERROR, "Stream not opened");
        return false;
    }
    PaError err = Pa_StartStream(stream_);
    if (err != paNoError) {
        Logger::instance().log(LogLevel::ERROR, std::string("Pa_StartStream failed: ") + Pa_GetErrorText(err));
        return false;
    }
    Logger::instance().log(LogLevel::INFO, "AudioOutput started");
    return true;
}

void AudioOutput::stop() {
    if (dummyMode_) {
        Logger::instance().log(LogLevel::INFO, "AudioOutput stopped (DUMMY mode)");
        return;
    }
    if (stream_) {
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        Logger::instance().log(LogLevel::INFO, "AudioOutput stopped");
    }
}

// -----------------------------
// write() -> producer API
//   - write up to frameCount frames; returns how many frames were written.
//   - Must be called from a non-real-time thread (e.g. decoder thread).
// -----------------------------
size_t AudioOutput::write(const float* frames, size_t frameCount) {
    if (dummyMode_) {
        return frameCount;
    }

    // Calculate current head/tail and free capacity in frames
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_acquire);

    // capacityFrames_ is power-of-two
    size_t freeFrames = (tail - head - 1) & (capacityFrames_ - 1); // leave one frame free to distinguish full/empty

    size_t toWrite = std::min(frameCount, freeFrames);

    // Write frames into ring buffer
    for (size_t f = 0; f < toWrite; ++f) {
        size_t idx = ((head + f) & (capacityFrames_ - 1)) * channels_;
        // copy channel samples
        for (int c = 0; c < channels_; ++c) {
            buffer_[idx + c] = frames[f * channels_ + c];
        }
    }

    // Publish new head index
    head_.store((head + toWrite) & (capacityFrames_ - 1), std::memory_order_release);

    return toWrite;
}

size_t AudioOutput::available() const {
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_acquire);
    return (tail - head - 1) & (capacityFrames_ - 1);
}

size_t AudioOutput::size() const {
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_acquire);
    return (head - tail) & (capacityFrames_ - 1);
}

void AudioOutput::setVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    volume_.store(volume, std::memory_order_relaxed);
}

float AudioOutput::getVolume() const {
    return volume_.load(std::memory_order_relaxed);
}
