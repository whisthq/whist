#ifndef VIDEOENCODE_H
#define VIDEOENCODE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file videoencode.h
 * @brief This file contains the code to create and destroy Encoders and use
 *        them to encode captured screens.
============================
Usage
============================
Video is encoded to H264 via either a hardware encoder (currently, we use NVidia GPUs, so we use
NVENC) or a software encoder. H265 is also supported but not currently used. Since NVidia allows us
to both capture and encode the screen, most of the functions will be called in server/main.c with an
empty dummy encoder. For encoders, create an H264 encoder via create_video_encode, and use
it to encode frames via video_encoder_encode. Write the encoded output via
video_encoder_write_buffer, and when finished, destroy the encoder using destroy_video_encoder.
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>
#include "nvidia_encode.h"

/*
============================
Custom Types
============================
*/

#define MAX_ENCODER_PACKETS 20

/**
 * @brief           Struct for handling ffmpeg encoding of video frames. Set to a dummy struct if we
 *                  are using NVidia's SDK, which both captures and encodes frames. If software
 *                  encoding, the codec and context determine the properties of the output frames,
 *                  and scaling is done using the filter_graph. Frames encoded on the GPU are stored
 *                  in hw_frame, while frames encoded on the CPU are stored in sw_frame.
 *
 */
typedef struct VideoEncoder {
    // FFmpeg members to encode and scale video
    const AVCodec* codec;
    AVCodecContext* context;
    AVFilterGraph* filter_graph;
    AVFilterContext* filter_graph_source;
    AVFilterContext* filter_graph_sink;
    AVBufferRef* hw_device_ctx;
    int frames_since_last_iframe;

    // packet metadata + data
    int num_packets;
    AVPacket packets[MAX_ENCODER_PACKETS];

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

    EncodeType type;
    CodecType codec_type;

    bool capture_is_on_nvidia;
    NvidiaEncoder* nvidia_encoder;
} VideoEncoder;

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
 * @param p_cuda_context           Pointer to the CUDA context
 *
 * @returns                        The newly created encoder
 */
VideoEncoder* create_video_encoder(int in_width, int in_height, int out_width, int out_height,
                                   int bitrate, CodecType codec_type, void** p_cuda_context);

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
int video_encoder_frame_intake(VideoEncoder* encoder, void* rgb_pixels, int pitch);

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
int video_encoder_encode(VideoEncoder* encoder);

/**
 * @brief                          Write the data from the encoded packet(s) in
 *                                 `encoder->packets` to a buffer, along with
 *                                 header information specifying the size of
 *                                 each packet.
 *
 * @param encoder                  The encoder with stored encoded packet(s)
 * @param buf                      A pointer to an allocated buffer of size at
 *                                 least `encoder->encoded_frame_size` into
 *                                 which to copy the data
 */
void video_encoder_write_buffer(VideoEncoder* encoder, int* buf);

/**
 * @brief                          Reconfigure the encoder using new parameters
 *
 * @param encoder                  The encoder to be updated
 * @param width                    The new width
 * @param height                   The new height
 * @param bitrate                  The new bitrate
 * @param codec                    The new codec
 *
 * @returns                        0 if the encoder was successfully reconfigured,
 *                                 -1 if no reconfiguration was possible
 */
bool reconfigure_encoder(VideoEncoder* encoder, int width, int height, int bitrate,
                         CodecType codec);

/**
 * @brief                          Set the next frame to be an IDR-frame,
 *                                 with SPS/PPS headers included as well.
 *
 * @param encoder                  Encoder to be updated
 */
void video_encoder_set_iframe(VideoEncoder* encoder);

/**
 * @brief                          Allow the next frame to be either an I-frame
 *                                 or not an i-frame
 *
 * @param encoder                  Encoder to be updated
 */
void video_encoder_unset_iframe(VideoEncoder* encoder);

/**
 * @brief                          Destroy encoder
 *
 * @param encoder                  Encoder to be destroyed
 */
void destroy_video_encoder(VideoEncoder* encoder);

#endif  // VIDEOENCODE_H
