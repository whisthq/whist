#ifndef VIDEO_CODEC_DECODE_H
#define VIDEO_CODEC_DECODE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file decode.h
 * @brief This file contains the code to create a video decoder and use that
 *        decoder to decode frames.
============================
Usage
============================
Video is decoded from H264 via ffmpeg; H265 is supported, but not currently used.
Hardware-accelerated decoders are given priority, but if those fail, we decode on the CPU. All
frames are eventually moved to the CPU for scaling and color conversion.

Usage:

- Create a decoder via create_video_decoder.
- To decode a frame, call video_decoder_decode_frame on the decoder and the encoded packets.
- To get the frame, call get_last_decoded_frame.
- To destroy the decoder when finished, use destroy_video_decoder.
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include <whist/utils/avpacket_buffer.h>

#define MAX_ENCODED_VIDEO_PACKETS 20

/*
============================
Custom Types
============================
*/

/**
 * @brief       Struct that `get_current_decoded_frame` returns.
 */
typedef struct DecodedFrameData {
    // If true, use hw_frame. If false, use sw_frame.
    bool using_hw;
    AVFrame* decoded_frame;
    enum AVPixelFormat pixel_format;
    int width;
    int height;
} DecodedFrameData;

/**
 * Struct for video decoder parameters at init time.
 */
typedef struct VideoDecoderParams {
    /** Video codec to use. */
    CodecType codec_type;
    /** Frame width in pixels. */
    int width;
    /** Frame height in pixels. */
    int height;
    /** Whether hardware decode should be used. */
    bool hardware_decode;
    /**
     * Hardware format to use for output.
     *
     * This indicates that the caller can support this format directly,
     * so frames of this format should be returned without downloading
     * them to normal frames in CPU memory.  If unset then all hardware
     * formats will be downloaded.
     */
    enum AVPixelFormat hardware_output_format;
    /**
     * Hardware device to use.
     *
     * If unset the decoder will create one internally.
     */
    AVBufferRef* hardware_device;
} VideoDecoderParams;

/**
 * @brief       Struct for decoding frames via ffmpeg. Decoding is handled by the codec and context,
 * and after decoding finishes, the decoded frame will be in sw_frame, regardless of whether or not
 * we used hardware-accelerated decoding.
 *
 */
typedef struct VideoDecoder {
    VideoDecoderParams params;

    unsigned int decode_type;
    const AVCodec* codec;
    AVCodecContext* context;
    AVFrame* decoded_frame;
    AVBufferRef* ref;
    AVPacket* packets[MAX_ENCODED_VIDEO_PACKETS];
    enum AVPixelFormat match_fmt;
    enum AVHWDeviceType device_type;

    bool using_hw;

    bool received_a_frame;

    // File written when saving input.
    FILE* save_input_file;
} VideoDecoder;

/*
============================
Public Functions
============================
*/

/**
 * Create a video decoder instance, using the given parameters.
 *
 * @param params  Parameters to create decoder with.
 *
 * @return  New video decoder instance, or NULL on failure.
 */
VideoDecoder* video_decoder_create(const VideoDecoderParams* params);

/**
 * @brief                          Initialize the FFmpeg H264 or H265
 *                                 video decoder, and set the proper video
 *                                 parameters for receiving from the server
 *
 * @param width                    Width of the frames to decode
 * @param height                   Height of the frames to decode
 * @param use_hardware             Toggle whether to try to decode in hardware
 * @param codec_type               Which codec type (h264 or h265) to use
 *
 * @returns                        The FFmpeg video decoder struct
 */
VideoDecoder* create_video_decoder(int width, int height, bool use_hardware, CodecType codec_type);

/**
 * @brief                          Destroys an initialized FFmpeg video decoder
 *                                 and frees its memory
 *
 * @param decoder                  The FFmpeg video decoder to destroy
 */
void destroy_video_decoder(VideoDecoder* decoder);

/**
 * @brief                           Send the packets contained in buffer into the decoder
 *
 * @param decoder                   The decoder we are using for decoding
 *
 * @param buffer                    The buffer containing the encoded packets
 *
 * @param buffer_size               The size of the buffer containing the frame to decode
 *
 * @param start_of_stream           True if the packets represent the start of a stream (e.g. for
 *                                  H.264 they contain parameter sets and an IDR frame).  If so, a
 *                                  new decoder may be started from this point.
 *
 * @returns                         0 on success, -1 on failure
 */
int video_decoder_send_packets(VideoDecoder* decoder, void* buffer, size_t buffer_size,
                               bool start_of_stream);

/**
 * @brief                           Decode the next available frame from the decoder.
 *
 * @param decoder                   The decoder we are using for decoding
 *
 * @returns                         0 on success (can call again), 1 on EAGAIN (send more input
 *                                  before calling again), -1 on failure
 */
int video_decoder_decode_frame(VideoDecoder* decoder);

/**
 * @brief                           Get the most recently decoded frame from the decoder,
 *                                  which will be the sw_frame or hw_frame in DecodedFrameData.
 *                                  `video_decoder_decode_frame` must return 0 first, at least once.
 *
 * @param decoder                   The decoder we are using for decoding
 *
 * @returns                         The last decoded frame data.
 *
 * @note                            This returns a new reference to the decoded frame, so it may be
 *                                  called multiple times and the caller can do whatever they like
 *                                  with the returned frame.
 */
DecodedFrameData video_decoder_get_last_decoded_frame(VideoDecoder* decoder);

/**
 * @brief                           Free the frame returned by video_decoder_get_last_decoded_frame,
 *                                  Or do nothing if a zero'd struct is passed in.
 *
 * @param decoded_frame_data        The decoded frame to free.
 */
void video_decoder_free_decoded_frame(DecodedFrameData* decoded_frame_data);

#endif  // VIDEO_CODEC_DECODE_H
