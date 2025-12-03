#define __STDC_CONSTANT_MACROS
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "ffmpeg_decoder.h"
#include "../utils/logger.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

static std::string ffmpegErrStr(int errnum) {
    char buf[256];
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

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
}

FFmpegDecoder::~FFmpegDecoder() {
    cleanup();
}

bool FFmpegDecoder::open(const std::string& filepath) {
    cleanup();

    int ret = avformat_open_input(&fmt_ctx_, filepath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: avformat_open_input failed: ") + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: avformat_find_stream_info failed: ") + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    audio_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index_ < 0) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: No audio stream found");
        cleanup();
        return false;
    }

    AVCodecParameters* codecpar = fmt_ctx_->streams[audio_stream_index_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: Unsupported codec");
        cleanup();
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: avcodec_alloc_context3 failed");
        cleanup();
        return false;
    }

    ret = avcodec_parameters_to_context(codec_ctx_, codecpar);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: avcodec_parameters_to_context failed: ") + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: avcodec_open2 failed: ") + ffmpegErrStr(ret));
        cleanup();
        return false;
    }

    out_sample_rate_ = codec_ctx_->sample_rate > 0 ? codec_ctx_->sample_rate : 44100;
    out_channels_ = codec_ctx_->channels > 0 ? codec_ctx_->channels : 2;

    if (codec_ctx_->channel_layout == 0) {
        out_channel_layout_ = av_get_default_channel_layout(out_channels_);
    } else {
        out_channel_layout_ = codec_ctx_->channel_layout;
    }

    packet_ = av_packet_alloc();
    frame_ = av_frame_alloc();
    if (!packet_ || !frame_) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: packet/frame allocation failed");
        cleanup();
        return false;
    }

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

bool FFmpegDecoder::initResampler() {
    uint64_t in_ch_layout = codec_ctx_->channel_layout;
    if (in_ch_layout == 0) {
        in_ch_layout = av_get_default_channel_layout(codec_ctx_->channels);
    }

    uint64_t out_ch_layout = in_ch_layout;

    swr_ctx_ = swr_alloc_set_opts(
        nullptr,
        out_ch_layout,
        static_cast<AVSampleFormat>(out_sample_fmt_),
        out_sample_rate_,
        in_ch_layout,
        codec_ctx_->sample_fmt,
        codec_ctx_->sample_rate,
        0, nullptr
    );

    if (!swr_ctx_) {
        Logger::instance().log(LogLevel::ERROR, "FFmpegDecoder: swr_alloc_set_opts returned nullptr");
        return false;
    }

    int ret = swr_init(swr_ctx_);
    if (ret < 0) {
        Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: swr_init failed: ") + ffmpegErrStr(ret));
        swr_free(&swr_ctx_);
        return false;
    }

    return true;
}

int FFmpegDecoder::decode(std::vector<int16_t>& out_buffer) {
    if (!fmt_ctx_ || !codec_ctx_ || !swr_ctx_ || !packet_ || !frame_) {
        return 0;
    }

    int totalSamplesAppended = 0;

    while (true) {
        // Logger::instance().log(LogLevel::INFO, "FFmpegDecoder: Entering decode loop");
        while (true) {
            int ret = avcodec_receive_frame(codec_ctx_, frame_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                // Logger::instance().log(LogLevel::INFO, "FFmpegDecoder: avcodec_receive_frame returned EAGAIN/EOF");
                break;
            } else if (ret < 0) {
                Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: avcodec_receive_frame failed: ") + ffmpegErrStr(ret));
                return totalSamplesAppended;
            }
            // Logger::instance().log(LogLevel::INFO, "FFmpegDecoder: avcodec_receive_frame got frame");

            int max_out_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx_, codec_ctx_->sample_rate) + frame_->nb_samples,
                out_sample_rate_, codec_ctx_->sample_rate, AV_ROUND_UP);

            uint8_t** converted = nullptr;
            int converted_linesize = 0;
            int ret_alloc = av_samples_alloc_array_and_samples(
                &converted,
                &converted_linesize,
                out_channels_,
                max_out_samples,
                static_cast<AVSampleFormat>(out_sample_fmt_),
                0
            );
            if (ret_alloc < 0) {
                Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: av_samples_alloc_array_and_samples failed: ") + ffmpegErrStr(ret_alloc));
                av_frame_unref(frame_);
                return totalSamplesAppended;
            }

            int out_samples = swr_convert(
                swr_ctx_,
                converted,
                max_out_samples,
                const_cast<const uint8_t**>(frame_->extended_data),
                frame_->nb_samples
            );

            if (out_samples < 0) {
                Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: swr_convert failed: ") + ffmpegErrStr(out_samples));
                av_freep(&converted[0]);
                av_freep(&converted);
                av_frame_unref(frame_);
                return totalSamplesAppended;
            }

            int totalConvertedSamples = out_samples * out_channels_;
            int16_t* samples16 = reinterpret_cast<int16_t*>(converted[0]);

            out_buffer.insert(out_buffer.end(), samples16, samples16 + totalConvertedSamples);
            totalSamplesAppended += totalConvertedSamples;

            av_freep(&converted[0]);
            av_freep(&converted);

            av_frame_unref(frame_);
        }

        if (totalSamplesAppended > 0) {
            return totalSamplesAppended;
        }

        if (eof_) {
            Logger::instance().log(LogLevel::INFO, "FFmpegDecoder: EOF reached");
            return 0;
        }

        int ret = av_read_frame(fmt_ctx_, packet_);
        if (ret < 0) {
            Logger::instance().log(LogLevel::INFO, std::string("FFmpegDecoder: av_read_frame failed (EOF?): ") + ffmpegErrStr(ret));
            eof_ = true;
            av_packet_unref(packet_);
            avcodec_send_packet(codec_ctx_, nullptr);
            continue;
        }

        if (packet_->stream_index == audio_stream_index_) {
            int sendRet = avcodec_send_packet(codec_ctx_, packet_);
            if (sendRet < 0) {
                Logger::instance().log(LogLevel::ERROR, std::string("FFmpegDecoder: avcodec_send_packet failed: ") + ffmpegErrStr(sendRet));
                av_packet_unref(packet_);
                return totalSamplesAppended;
            }
        } else {
             // Logger::instance().log(LogLevel::INFO, "FFmpegDecoder: Ignored packet from other stream");
        }
        av_packet_unref(packet_);
    }
}

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
}

