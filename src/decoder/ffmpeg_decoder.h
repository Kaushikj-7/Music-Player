#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

/**
 * ffmpeg_decoder.h
 *
 * FFmpeg-based decoder that exposes a simple C++ API used by Player.
 *
 * Public methods:
 *   - bool open(const std::string& filepath)
 *   - int decode(std::vector<int16_t>& out_buffer)
 *   - void close()
 *   - int getSampleRate() const
 *   - int getChannels() const
 *
 * Behavior:
 *   - Decoded and resampled audio is returned as interleaved signed 16-bit PCM
 *     samples in host endianness (int16_t).
 *   - decode(...) appends samples to the provided vector and returns number of
 *     samples appended (not frames). If 0 is returned, that indicates EOF or
 *     no samples available.
 *
 * Implementation notes (in .cpp):
 *   - Uses libavformat/libavcodec to demux and decode packets.
 *   - Uses libswresample (swr_convert) to convert to AV_SAMPLE_FMT_S16 interleaved,
 *     and to the desired sample rate / channel layout.
 *   - The output sample rate and channels are chosen to be the codec's native
 *     values by default (but you can change that part in code if you want a
 *     fixed output).
 */

#include <string>
#include <vector>
#include <cstdint>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

class FFmpegDecoder {
public:
    FFmpegDecoder();
    ~FFmpegDecoder();

    // Open the media file. Returns true on success.
    bool open(const std::string& filepath);

    // Decode some audio and append interleaved int16 samples to out_buffer.
    // Returns number of int16 samples appended. 0 -> EOF or no more data.
    int decode(std::vector<int16_t>& out_buffer);

    // Close and free resources.
    void close();

    // Output parameters (after open)
    int getSampleRate() const { return out_sample_rate_; }
    int getChannels() const { return out_channels_; }

private:
    // Initialize (allocate) resampler based on codecCtx_.
    bool initResampler();

    // Internal cleanup helper
    void cleanup();

private:
    AVFormatContext* fmt_ctx_;   // format context (container)
    AVCodecContext* codec_ctx_;  // codec context (decoder)
    SwrContext* swr_ctx_;        // resampler context
    AVPacket* packet_;           // packet container
    AVFrame* frame_;             // decoded frame

    int audio_stream_index_;     // index of audio stream in container

    // Output format parameters (we resample to these)
    int out_sample_rate_;        // e.g., 44100
    int out_channels_;           // e.g., 2
    AVSampleFormat out_sample_fmt_; // AV_SAMPLE_FMT_S16
    uint64_t out_channel_layout_;   // channel layout mask

    bool eof_;                   // end-of-file reached flag
};

#endif // FFMPEG_DECODER_H
