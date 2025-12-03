#pragma once
/*
 audio_output.h

 Purpose:
   - Provide a lightweight real-time audio output abstraction using PortAudio.
   - Provide an SPSC lock-free ring buffer for real-time-safe audio streaming.
   - Expose a simple API:
       bool init(sampleRate, channels, framesPerBuffer)
       bool start()
       void stop()
       size_t write(const float* frames, size_t frameCount)  // producer API
       size_t available() const                              // how many frames free
       size_t size() const                                   // how many frames used
   - PortAudio callback pulls frames from ring buffer and writes them to device.

 Why:
   - Real audio playback requires a callback-driven API to minimize latency.
   - The SPSC ring buffer ensures the callback is lock-free and deterministic.

 Notes:
   - We store audio as interleaved float32 samples (common practice for mixing).
   - Producer expected to convert int16_t -> float in producer thread before write().
*/

#include <vector>
#include <atomic>
#include <cstdint>
#include <cstddef>   // for size_t

// Forward declare PortAudio types to avoid including portaudio.h in header.
// We will include portaudio.h in the .cpp implementation file.
struct PaStreamParameters;
// typedef struct PaStream PaStream; // Removed because PaStream is void in some versions

class AudioOutput {
public:
    AudioOutput();
    ~AudioOutput();

    // Initialize audio output
    // sampleRate: e.g., 44100
    // channels: 1 (mono) or 2 (stereo)
    // framesPerBuffer: portaudio buffer size (0 = default / system-chosen)
    bool init(int sampleRate, int channels, unsigned long framesPerBuffer = 0);

    // Start audio stream (returns true on success)
    bool start();

    // Stop playback (blocks until callback stops)
    void stop();

    // Set volume (0.0 to 1.0)
    void setVolume(float volume);
    float getVolume() const;

    // Producer API: write interleaved float frames into the ring buffer.
    // frameCount = number of frames (a frame contains `channels` samples).
    // Returns number of frames actually written (may be less if buffer is full).
    size_t write(const float* frames, size_t frameCount);

    // Query how many frames currently available to write (free capacity)
    size_t available() const;

    // Query how many frames currently available to read (used)
    size_t size() const;

private:
    // Internal: power-of-two ring buffer for interleaved float samples.
    // The ring buffer size is in *frames* (not samples). Internally we store
    // frames * channels floats.
    std::vector<float> buffer_;        // contiguous storage
    size_t capacityFrames_;            // capacity in frames (power of two)
    int channels_;                     // channels (1 or 2)
    std::atomic<size_t> head_;         // write index in frames
    std::atomic<size_t> tail_;         // read index in frames

    // PortAudio stream handle (implementation includes portaudio.h)
    void* stream_;
    int sampleRate_;
    unsigned long framesPerBuffer_;
    bool dummyMode_;
    std::atomic<float> volume_;

    // Internal helper to ensure capacity is power-of-two
    static size_t nextPowerOfTwo(size_t v);
};
