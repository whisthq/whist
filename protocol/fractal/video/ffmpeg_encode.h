#ifndef FFMPEG_ENCODE_H
#define FFMPEG_ENCODE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ffmpeg_encode.h
 * @brief This file contains the code to create and destroy FFmpeg encoders and use
 *        them to encode captured screens.
============================
Usage
============================
Video is encoded to H264 via either a hardware encoder (currently, we use NVidia GPUs, so we use
NVENC) or a software encoder if hardware encoding fails. H265 is also supported but not currently
used. For encoders, create an H264 encoder via create_ffmpeg_encoder, and use it to encode frames
via ffmpeg_encoder_send_frame. Retrieve encoded packets using ffmpeg_encoder_receive_packet. When
finished, destroy the encoder using destroy_ffmpeg_encoder.
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>

/*
============================
Custom Types
============================
*/
/**
 * @brief   Encode type.
 * @details Type of encoding used for video encoding.
 */
typedef enum FFmpegEncodeType {
    SOFTWARE_ENCODE = 0,
    NVENC_ENCODE = 1,
    QSV_ENCODE = 2
} FFmpegEncodeType;

/**
 * @brief           Struct for handling ffmpeg encoding of video frames. If software encoding, the
 * codec and context determine the properties of the output frames, and scaling is done using the
 * filter_graph. Frames encoded on the GPU are stored in hw_frame, while frames encoded on the CPU
 * are stored in sw_frame.
 *
 */
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

/*
============================
Public Functions
============================
*/
/**
 * @brief                          Will create a new encoder
 *
 * @param in_width                 Width of the frames that the encoder must
 *                                 intake
 * @param in_height                Height of the frames that the encoder must
 *                                 intake
 * @param out_width                Width of the frames that the encoder must
 *                                 output
 * @param out_height               Height of the frames that the encoder must
 *                                 output
 * @param bitrate                  The number of bits per second that this
 *                                 encoder will encode to
 * @param codec_type               Which codec type (h264 or h265) to use
 *
 * @returns                        The newly created encoder
 */
FFmpegEncoder* create_ffmpeg_encoder(int in_width, int in_height, int out_width, int out_height,
                                     int bitrate, CodecType codec_type);
/**
 * @brief                          Put the input data into a software frame, and
 *                                 upload to a hardware frame if applicable.
 *
 * @param encoder                  The encoder to use
 * @param rgb_pixels               The frame to be in encoded
 * @param pitch                    The number of bytes per line
 *
 * @returns                        0 on success, else -1
 */
int ffmpeg_encoder_frame_intake(FFmpegEncoder* encoder, void* rgb_pixels, int pitch);
int ffmpeg_encoder_receive_packet(FFmpegEncoder* encoder, AVPacket* packet);

/**
 * @brief                          Encode the frame in `encoder->sw_frame` or
 *                                 `encoder->hw_frame`. The encoded packet(s)
 *                                 are stored in `encoder->packets`, and the
 *                                 size of the buffer necessary to store them
 *                                 is stored in `encoder->encoded_frame_size`
 *
 * @param encoder                  The encoder to use
 *
 * @returns                        0 on success, else -1
 */
int ffmpeg_encoder_send_frame(FFmpegEncoder* encoder);

/**
 * @brief                          Set the next frame to be an IDR-frame,
 *                                 with SPS/PPS headers included as well. Right now, this function
 * is not reliable.
 *
 * @param encoder                  Encoder to be updated
 */
void ffmpeg_set_iframe(FFmpegEncoder* encoder);

/**
 * @brief                          Allow the next frame to be either an I-frame
 *                                 or not an i-frame. Right now, this function is not reliable.
 *
 * @param encoder                  Encoder to be updated
 */
void ffmpeg_unset_iframe(FFmpegEncoder* encoder);

/**
 * @brief                          Destroy encoder
 *
 * @param encoder                  Encoder to be destroyed
 */
void destroy_ffmpeg_encoder(FFmpegEncoder* encoder);
#endif
