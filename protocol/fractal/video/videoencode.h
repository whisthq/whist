#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

/*
@brief                          The encoder
*/
typedef struct {
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    AVFilterGraph *pFilterGraph;
    AVFilterContext *pFilterGraphSource;
    AVFilterContext *pFilterGraphSink;
    AVBufferRef *hw_device_ctx;
    AVPacket packet;

    int in_width, in_height;
    int out_width, out_height;
    int gop_size;
    int encoded_frame_size;
    void *sw_frame_buffer;
    void *encoded_frame_data;

    AVFrame *hw_frame;
    AVFrame *sw_frame;
    AVFrame *filtered_frame;

    EncodeType type;
} video_encoder_t;

/*
@brief                          Will create a new encoder

@param in_width                 Width of the frames that the encoder must intake
@param in_height                Height of the frames that the encoder must
intake
@param out_width                Width of the frames that the encoder must output
@param out_height               Height of the frames that the encoder must
output
@param bitrate                  The number of bits per second that this encoder
will encode to

@returns                        The newly created encoder
*/
video_encoder_t *create_video_encoder(int in_width, int in_height,
                                      int out_width, int out_height,
                                      int bitrate);

/*
@brief                          Pass a single frame into the AVFilterGraph input
buffer in preparation for consumption by the encoder

@param encoder                  Encoder to use
@param rgb_pixels               The frame data to include
*/
void video_encoder_filter_graph_intake(video_encoder_t *encoder,
                                       void *rgb_pixels);

/*
@brief                          Flush the AVFilterGraph output buffer into the
encoder, and retrieve the next encoded frame. The frame can be accessed via
encoded_frame_size and encoded_frame_data

@param encoder                  Encoder to use

@returns                        0 on success, 1 if the encoder has no more
frames, and -1 on failure
*/
int video_encoder_encode_frame(video_encoder_t *encoder);

/*
@brief                          Intake and encode a frame, as a legacy API for
testing purposes.

@param encoder                  Encoder to use
@param rgb_pixels               The frame data to encode

@returns                        0 on success, 1 if the encoder has no more
frames, and -1 on failure
*/
int video_encoder_encode(video_encoder_t *encoder, void *rgb_pixels);

/*
@brief                          Set the next frame to be an i-frame

@param encoder                  Encoder to be updated
*/
void video_encoder_set_iframe(video_encoder_t *encoder);

/*
@brief                          Allow the next frame to be either an i-frame or
not an i-frame.

@param encoder                  Encoder to be updated
*/
void video_encoder_unset_iframe(video_encoder_t *encoder);

/*
@brief                          Destroy encoder

@param encoder                  Encoder to be destroyed
*/
void destroy_video_encoder(video_encoder_t *encoder);

#endif  // ENCODE_H
