#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavdevice/avdevice.h"
#include "ffmpeg/libavfilter/avfilter.h"
#include "ffmpeg/libavfilter/buffersink.h"
#include "ffmpeg/libavfilter/buffersrc.h"
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavutil/avutil.h"
#include "ffmpeg/libswresample/swresample.h"
#include "ffmpeg/libswscale/swscale.h"

// define decoder struct to pass as a type
typedef struct {
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    AVFormatContext *pFormatCtx;
    AVFrame *pFrame;
    AVPacket packet;
    SwrContext *pSwrContext;
    uint8_t *out_buffer;
} audio_decoder_t;

audio_decoder_t *create_audio_decoder(int sample_rate);

int init_av_frame(audio_decoder_t *decoder);

void audio_decoder_packet_readout(audio_decoder_t *decoder, uint8_t *data,
                                  int len);

int audio_decoder_decode_packet(audio_decoder_t *decoder,
                                AVPacket *encoded_packet);

void destroy_audio_decoder(audio_decoder_t *decoder);

#endif  // DECODE_H
