/**
 * ffmpeg_decoder.cpp
 *
 * Implementation of FFmpegDecoder declared in ffmpeg_decoder.h.
 *
 * Important design choices:
 *  - Output format: interleaved S16 (int16_t). This is compact and matches many
 *    audio playback backends. Converting to float can be done later in the Player.
 *  - decode(...) returns number of int16 samples appended. Player will convert
 *    those to floats and push to AudioOutput.
 *
 * Threading note:
 *  - This decoder is not thread-safe by itself. Player uses it from a single
 *    decoder thread. If you need multiple decoders concurrently, instantiate
 *    separate FFmpegDecoder objects.
 *
 * Build/link: Requires linking with:
 *    - lavformat, lavcodec, lavutil, swresample
 *
 * Example:
 *    FFmpegDecoder dec;
 *    dec.open("song.mp3");
 *    std::vector<int16_t> out;
 *    while (dec.decode(out) > 0) { /* push to audio */ }
 *    dec.close();
 */

#include "ffmpeg_decoder.h"
#include "../utils/logger.h"   // logger singleton for diagnostic messages

#include <cstdlib>   // for EXIT_*
#include <cstring>   // memcpy
#include <algorithm> // std::min

// Helper macro to convert libav error code to human-readable string.
// av_err2str is a macro generating a temp buffer; use av_strerror instead.
static std::string ffmpegErrStr(int errnum) {
    char buf[256];
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

// ---------------- Constructor / Destructor ----------------
FFmpegDecoder::FFmpegDecoder()
    : fmt_ctx_(nullptr),
      codec_ctx_(nullptr),
      swr_ctx_(nullptr),
      packet_(nullptr),
      frame_(nullptr),
      audio_stream_index_(-1),
      out_sample_rate_(0),
      out_channels_(0),
      out_sample_fmt_(AV_SAMPLE_FMT_S16),
      out_channel_layout_(0),
      eof_(false)
{
    // In modern FFmpeg builds av_register_all is deprecated, but calling it
    // is harmless on many installs. We won't rely on it.
    // av_register_all();
}

FFmpegDecoder::~FFmpegDecoder() {
    cleanup();
}

// ---------------- open() ----------------
bool FFmpegDecoder::open(const std::string& filepath) {
    cleanup();

    // Open input file and allocate format context
    int ret = avformat_open_input(&fmt_ctx_, filepath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avformat_open_input failed: " + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    // Retrieve stream information
    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avformat_find_stream_info failed: " + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    // Find the first audio stream
    audio_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index_ < 0) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: No audio stream found");
        cleanup();
        return false;
    }

    // Get codec parameters for the audio stream
    AVCodecParameters* codecpar = fmt_ctx_->streams[audio_stream_index_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: Unsupported codec");
        cleanup();
        return false;
    }

    // Allocate codec context
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avcodec_alloc_context3 failed");
        cleanup();
        return false;
    }

    // Copy codec parameters into codec context
    ret = avcodec_parameters_to_context(codec_ctx_, codecpar);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avcodec_parameters_to_context failed: " + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    // Open codec
    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avcodec_open2 failed: " + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    // Determine output parameters based on codec context (we will resample to these)
    out_sample_rate_ = codec_ctx_->sample_rate > 0 ? codec_ctx_->sample_rate : 44100;
    out_channels_ = codec_ctx_->channels > 0 ? codec_ctx_->channels : 2;

    // Set default channel layout (required for swr)
    if (codec_ctx_->channel_layout == 0) {
        out_channel_layout_ = av_get_default_channel_layout(out_channels_);
    } else {
        out_channel_layout_ = codec_ctx_->channel_layout;
    }

    // Allocate packet and frame
    packet_ = av_packet_alloc();
    frame_ = av_frame_alloc();
    if (!packet_ || !frame_) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: packet/frame allocation failed");
        cleanup();
        return false;
    }

    // Initialize resampler (convert from codec's sample_fmt/sample_rate/channel_layout to out_sample_fmt_/out_sample_rate_/out_channel_layout_)
    if (!initResampler()) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: initResampler failed");
        cleanup();
        return false;
    }

    eof_ = false;

    std::string msg = std::string("FFmpegDecoder: Opened successfully. SR=") + std::to_string(out_sample_rate_) + std::string(" CH=") + std::to_string(out_channels_);
    Logger::instance().log(LogLevel::INFO, msg);
    return true;
}

// ---------------- initResampler ----------------
bool FFmpegDecoder::initResampler() {
    // Input layout / format from codec context
    uint64_t in_ch_layout = codec_ctx_->channel_layout;
    if (in_ch_layout == 0) {
        in_ch_layout = av_get_default_channel_layout(codec_ctx_->channels);
    }

    // Output layout: use same layout as input (preserve stereo/mono)
    uint64_t out_ch_layout = in_ch_layout;

    // Allocate SwrContext
    swr_ctx_ = swr_alloc_set_opts(
        nullptr,
        out_ch_layout,
        out_sample_fmt_,           // target format: S16
        out_sample_rate_,          // target sample rate
        in_ch_layout,
        codec_ctx_->sample_fmt,    // source sample format
        codec_ctx_->sample_rate,   // source sample rate
        0, nullptr
    );

    if (!swr_ctx_) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: swr_alloc_set_opts returned nullptr");
        return false;
    }

    int ret = swr_init(swr_ctx_);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: swr_init failed: " + ffmpegErrStr(ret));
        swr_free(&swr_ctx_);
        return false;
    }

    return true;
}

// ---------------- decode() ----------------
// Append decoded interleaved S16 samples to out_buffer and return number appended.
// Implementation approach:
//  - Loop: if packet available, send to decoder (avcodec_send_packet).
//  - Then repeatedly call avcodec_receive_frame to get decoded AVFrame(s).
//  - Convert each decoded frame via swr_convert to S16 interleaved, append to out_buffer.
//  - Continue until we've produced some output or reached EOF and flushed decoder.
// Note: We do not enforce a strict maximum per call; the caller (Player) converts and writes to audio ring buffer.
int FFmpegDecoder::decode(std::vector<int16_t>& out_buffer) {
    if (!fmt_ctx_ || !codec_ctx_ || !swr_ctx_ || !packet_ || !frame_) {
        return 0;
    }

    int totalSamplesAppended = 0;

    // Loop until we appended some samples or reach EOF and no more frames.
    while (true) {
        // Try receiving frames while decoder has them
        while (true) {
            int ret = avcodec_receive_frame(codec_ctx_, frame_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break; // need to feed more packets or EOF flushed
            } else if (ret < 0) {
                Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avcodec_receive_frame failed: " + ffmpegErrStr(ret));
                return totalSamplesAppended;
            }

            // At this point, frame_ contains decoded audio in codec's sample format.
            // Convert frame_ -> S16 interleaved using swr_convert.

            // Estimate maximum number of output samples per channel.
            int max_out_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx_, codec_ctx_->sample_rate) + frame_->nb_samples,
                out_sample_rate_, codec_ctx_->sample_rate, AV_ROUND_UP);

            // Allocate converted buffer: single pointer with interleaved data
            uint8_t** converted = nullptr;
            int converted_linesize = 0;
            int ret_alloc = av_samples_alloc_array_and_samples(
                &converted,
                &converted_linesize,
                out_channels_,
                max_out_samples,
                out_sample_fmt_,
                0
            );
            if (ret_alloc < 0) {
                Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: av_samples_alloc_array_and_samples failed: " + ffmpegErrStr(ret_alloc));
                av_frame_unref(frame_);
                return totalSamplesAppended;
            }

            // Perform conversion
            int out_samples = swr_convert(
                swr_ctx_,
                converted,
                max_out_samples,
                const_cast<const uint8_t**>(frame_->extended_data),
                frame_->nb_samples
            );

            if (out_samples < 0) {
                Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: swr_convert failed: " + ffmpegErrStr(out_samples));
                av_freep(&converted[0]);
                av_freep(&converted);
                av_frame_unref(frame_);
                return totalSamplesAppended;
            }

            // Number of interleaved int16 samples = out_samples * out_channels_
            int totalConvertedSamples = out_samples * out_channels_;
            int16_t* samples16 = reinterpret_cast<int16_t*>(converted[0]);

            // Append to out_buffer
            size_t prevSize = out_buffer.size();
            out_buffer.insert(out_buffer.end(), samples16, samples16 + totalConvertedSamples);
            totalSamplesAppended += totalConvertedSamples;

            // Free converted buffer
            av_freep(&converted[0]);
            av_freep(&converted);

            // Unref frame to prepare for next receive
            av_frame_unref(frame_);
        } // end receive-frame loop

        // If we already appended some samples, return to let producer push them
        if (totalSamplesAppended > 0) {
            return totalSamplesAppended;
        }

        // If EOF previously detected and no samples produced, return 0
        if (eof_) {
            return 0;
        }

        // Read next packet from format context
        int ret = av_read_frame(fmt_ctx_, packet_);
        if (ret < 0) {
            // EOF or error - signal EOF and flush decoder by sending null packet
            eof_ = true;
            av_packet_unref(packet_);
            avcodec_send_packet(codec_ctx_, nullptr); // flush
            // Loop will attempt avcodec_receive_frame again to flush frames
            continue;
        }

        // Only send audio stream packets to the decoder
        if (packet_->stream_index == audio_stream_index_) {
            int sendRet = avcodec_send_packet(codec_ctx_, packet_);
            if (sendRet < 0) {
                Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avcodec_send_packet failed: " + ffmpegErrStr(sendRet));
                av_packet_unref(packet_);
                return totalSamplesAppended;
            }
        }
        // Free packet data
        av_packet_unref(packet_);

        // Loop to receive frames (back to top)
    } // end outer while
}

// ---------------- close() & cleanup ----------------
void FFmpegDecoder::close() {
    cleanup();
}

void FFmpegDecoder::cleanup() {
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }
    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }
    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
    }
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
    }
    audio_stream_index_ = -1;
    eof_ = false;
}
