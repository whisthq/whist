#ifndef FFMPEG_ENCODE_H
#define FFMPEG_ENCODE_H

#include <fractal/core/fractal.h>

/**
 * @brief   Encode type.
 * @details Type of encoding used for video encoding.
 */
typedef enum FFmpegEncodeType { SOFTWARE_ENCODE = 0, NVENC_ENCODE = 1, QSV_ENCODE = 2 } FFmpegEncodeType;

typedef struct FFmpegEncoder {
    // FFmpeg members to encode and scale video
    const AVCodec* codec;
    AVCodecContext* context;
    AVFilterGraph* filter_graph;
    AVFilterContext* filter_graph_source;
    AVFilterContext* filter_graph_sink;
    AVBufferRef* hw_device_ctx;
    int frames_since_last_iframe;

    // frame metadata + data
    int in_width, in_height;
    int out_width, out_height;
    int gop_size;
    bool is_iframe;
    void* sw_frame_buffer;
    int encoded_frame_size;  /// <size of encoded frame in bytes

    AVFrame* hw_frame;
    AVFrame* sw_frame;
    AVFrame* filtered_frame;

    FFmpegEncodeType type;
    CodecType codec_type;
} FFmpegEncoder;


FFmpegEncoder* create_ffmpeg_encoder(int in_width, int in_height, int out_width, int out_height, int bitrate, CodecType codec_type);
int ffmpeg_encoder_frame_intake(FFmpegEncoder* encoder, void* rgb_pixels, int pitch);
int ffmpeg_encoder_receive_packet(FFmpegEncoder *encoder, AVPacket *packet);
int ffmpeg_encoder_send_frame(FFmpegEncoder *encoder);
void ffmpeg_set_iframe(FFmpegEncoder* encoder);
void ffmpeg_unset_iframe(FFmpegEncoder* encoder);
void destroy_ffmpeg_encoder(FFmpegEncoder* encoder);
#endif
