#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H

#include "fractal.h" // contains all the headers
#include "audiocapture.h"

// define encoder struct to pass as a type
typedef struct 
{
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    AVFilterGraph *filter_graph;
} audio_filter;

typedef struct 
{
    AVCodec *codec;
    AVCodecContext *context;
    AVFrame *input_frame;
    AVFrame *output_frame;
    AVPacket packet;
} audio_encoder_t;

int check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt);
int select_sample_rate(AVCodec *codec);
int select_channel_layout(AVCodec *codec);

audio_filter *create_audio_filter(audio_capture_device *device, audio_encoder_t *encoder);
audio_encoder_t *create_audio_encoder(int bit_rate); 
void audio_encode_and_send(audio_capture_device *device, audio_encoder_t *encoder, audio_filter *filter, SocketContext context);

#endif // AUDIO_ENCODE_H
