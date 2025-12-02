/**
 * player.cpp
 *
 * Implementation of Player class. This file explains:
 *  - How decoder -> audio output flow is established
 *  - How the decode thread converts int16 PCM -> float and writes to ring buffer
 *  - Thread lifecycle and synchronization using atomics
 *
 * Key libraries used:
 *  - FFmpegDecoder (decoding, multi-format)
 *  - AudioOutput (PortAudio wrapper with SPSC ring buffer)
 *  - Logger for debug/diagnostic messages
 */

#include "player.h"

// Concrete includes
#include "../decoder/ffmpeg_decoder.h"    // FFmpeg decoder interface
#include "../audio/audio_output.h"       // AudioOutput abstraction (PortAudio + ring buffer)
#include "../utils/logger.h"             // Logger (singleton)

// STL
#include <vector>
#include <chrono>
#include <algorithm>

Player::Player()
    : decoder_(nullptr),
      audioOut_(nullptr),
      playing_(false),
      stopRequested_(false)
{}

Player::~Player() {
    stop();
}

bool Player::load(const std::string& filepath) {
    // Stop any existing playback
    stop();

    Logger::instance().log(LogLevel::INFO, "Player: Loading file: " + filepath);

    // Create decoder and open file
    decoder_.reset(new FFmpegDecoder());
    if (!decoder_->open(filepath)) {
        Logger::instance().log(LogLevel::ERROR, "Player: FFmpegDecoder failed to open file");
        decoder_.reset();
        return false;
    }

    // Create audio output and initialize with decoder's parameters
    audioOut_.reset(new AudioOutput());

    // Use decoder's sample rate/channels to configure output. AudioOutput expects floats.
    int sr = decoder_->getSampleRate();
    int ch = decoder_->getChannels();

    if (!audioOut_->init(sr, ch, 512)) { // 512 frames per buffer is a common low-latency value
        Logger::instance().log(LogLevel::ERROR, "Player: AudioOutput init failed");
        audioOut_.reset();
        decoder_->close();
        decoder_.reset();
        return false;
    }

    currentFile_ = filepath;
    Logger::instance().log(LogLevel::INFO,
        "Player: Loaded successfully (sr=" + std::to_string(sr) +
        ", ch=" + std::to_string(ch) + ")");
    return true;
}

bool Player::play() {
    if (!decoder_ || !audioOut_) {
        Logger::instance().log(LogLevel::ERROR, "Player: No file loaded or audio output unavailable");
        return false;
    }

    if (playing_.load()) {
        Logger::instance().log(LogLevel::WARNING, "Player: Already playing");
        return true;
    }

    // Start audio device
    if (!audioOut_->start()) {
        Logger::instance().log(LogLevel::ERROR, "Player: Failed to start audio output");
        return false;
    }

    // Reset flags
    stopRequested_.store(false);
    playing_.store(true);

    // Launch decoder thread (producer)
    decoderThread_ = std::thread(&Player::decodeThreadFunc, this);

    Logger::instance().log(LogLevel::INFO, "Player: Playback started");
    return true;
}

void Player::stop() {
    // Request stop
    if (!playing_.load()) {
        // Not playing — still ensure resources cleaned
        if (decoder_) {
            decoder_->close();
            decoder_.reset();
        }
        if (audioOut_) {
            audioOut_->stop();
            audioOut_.reset();
        }
        return;
    }

    stopRequested_.store(true);

    // Wait for thread to finish
    if (decoderThread_.joinable()) {
        decoderThread_.join();
    }

    // Stop audio device
    if (audioOut_) {
        // wait for ring buffer to drain before stopping (best-effort)
        while (audioOut_->size() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (!playing_.load()) break;
        }
        audioOut_->stop();
    }

    // Close decoder
    if (decoder_) {
        decoder_->close();
        decoder_.reset();
    }

    playing_.store(false);
    stopRequested_.store(false);

    Logger::instance().log(LogLevel::INFO, "Player: Playback stopped and resources released");
}

// decodeThreadFunc:
// - Repeatedly calls decoder_->readSamples(...) to fetch up to framesPerRead frames
// - Converts int16_t PCM -> float in [-1.0, +1.0]
// - Calls audioOut_->write() to push frames into ring buffer
//
// Thread safety:
// - This is a producer thread (non-RT). audioOut_->write() is designed to be called from non-RT thread.
// - audioOut_'s callback (consumer) is running in PortAudio RT thread and is lock-free.
void Player::decodeThreadFunc() {
    const size_t framesPerRead = 1024; // frames to request per decode call (frame = channels samples)
    int channels = decoder_->getChannels();

    // Temporary buffers
    std::vector<int16_t> intBuf;
    std::vector<float> floatBuf;

    while (!stopRequested_.load()) {
        // decoder_->readSamples expects a sample count = frames * channels (implementation-defined).
        // Here we call decode(out_buffer) which returns number of samples (interleaved int16).
        int nSamples = decoder_->decode(intBuf);
        if (nSamples <= 0) {
            // EOF or error
            Logger::instance().log(LogLevel::INFO, "Player: Decoder returned 0 samples (EOF)");
            break;
        }

        // Convert int16 -> float normalized
        floatBuf.resize(nSamples);
        for (int i = 0; i < nSamples; ++i) {
            // 32768.0f ensures normalization in [-1.0, +1.0)
            floatBuf[i] = static_cast<float>(intBuf[i]) / 32768.0f;
        }

        // Now push frames into audioOut_. audioOut_->write expects frameCount (frames = samples / channels).
        size_t totalFrames = static_cast<size_t>(nSamples) / static_cast<size_t>(channels);
        size_t writtenFrames = 0;
        const float* dataPtr = floatBuf.data();

        // Keep trying until all frames written or stop requested
        while (writtenFrames < totalFrames && !stopRequested_.load()) {
            size_t canWrite = audioOut_->write(dataPtr + writtenFrames * channels, totalFrames - writtenFrames);
            if (canWrite == 0) {
                // Buffer full: wait briefly (non-RT wait); avoid busy spin.
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            writtenFrames += canWrite;
        }
    } // end decode loop

    // Signal end-of-stream: no more data will be written. We leave any remaining frames to drain in audioOut_.
    Logger::instance().log(LogLevel::INFO, "Player: Decoder thread exiting");
}
