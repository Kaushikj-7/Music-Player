#pragma once
/**
 * player.h
 *
 * High-level Player class that ties decoder -> audio output -> playback control.
 *
 * Responsibilities:
 *  - Load an audio file using FFmpegDecoder
 *  - Initialize audio output (AudioOutput) with decoder's sample rate/channels
 *  - Launch a background decoding thread that pushes decoded PCM into AudioOutput
 *  - Provide play/stop/pause lifecycle APIs
 *
 * Design notes:
 *  - Decoder runs on a non-RT thread (producer).
 *  - AudioOutput::write() is lock-free and real-time safe (consumer is PortAudio callback).
 *  - PCM format used internally: interleaved float32 ([-1.0,1.0]), converted from int16_t return of decoder.
 *
 * Usage:
 *   Player player;
 *   player.load("song.mp3");
 *   player.play();
 *   player.stop();
 */

#include <string>
#include <thread>
#include <atomic>
#include <memory>

// Forward declarations of modules (include concrete headers in .cpp)
class AudioOutput;         // audio/audio_output.h
class FFmpegDecoder;       // decoder/ffmpeg_decoder.h

class Player {
public:
    Player();
    ~Player();

    // Load a file (returns true on success)
    bool load(const std::string& filepath);

    // Start playback -- spawns decoder thread and starts audio device
    bool play();

    // Stop playback (blocks until thread stops and audio device stops)
    void stop();

    // Pause playback
    void pause();

    // Resume playback
    void resume();

    // Set volume (0.0 - 1.0)
    void setVolume(float volume);

    // Set playback speed (0.5x - 2.0x)
    void setSpeed(float speed);
    float getSpeed() const { return speed_; }

    // Query playback state
    bool isPlaying() const { return playing_.load(); }
    bool isPaused() const { return paused_.load(); }
    bool isFinished() const { return finished_.load(); }

private:
    // Thread function executed by decoder thread
    void decodeThreadFunc();

private:
    // Owned components
    std::unique_ptr<FFmpegDecoder> decoder_;  // ownership of decoder instance
    std::unique_ptr<AudioOutput> audioOut_;   // ownership of audio output

    // Control flags
    std::atomic<bool> playing_;               // true while playback is active
    std::atomic<bool> paused_;                // true while playback is paused
    std::atomic<bool> stopRequested_;         // set to request stop
    std::atomic<bool> finished_;              // true when playback finished naturally

    // Worker thread that decodes and pushes audio
    std::thread decoderThread_;

    // File path currently loaded (for logging)
    std::string currentFile_;
    float speed_ = 1.0f;
};
