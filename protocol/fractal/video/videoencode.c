/**
 * Copyright Fractal Computers, Inc. 2020
 * @file videoencode.c
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
#include "videoencode.h"

#include <libavfilter/avfilter.h>
#include <stdio.h>
#include <stdlib.h>
/*
============================
Private Functions
============================
*/

void transfer_nvidia_data(VideoEncoder *encoder);
int transfer_ffmpeg_data(VideoEncoder *encoder);

/*
============================
Private Function Implementations
============================
*/

void transfer_nvidia_data(VideoEncoder *encoder) {
    /*
        Set encoder metadata according to nvidia_encoder members and tell the encoder there is only
       one packet, containing the encoded frame data from the nvidia encoder.

        Arguments:
            encoder (VideoEncoder*): encoder to use
    */
    if (encoder->active_encoder != NVIDIA_ENCODER) {
        LOG_ERROR("Encoder is not using nvidia, but we are trying to call transfer_nvidia_data!");
        return;
    }
    // Set meta data
    encoder->is_iframe = encoder->nvidia_encoder->is_iframe;
    encoder->out_width = encoder->nvidia_encoder->width;
    encoder->out_height = encoder->nvidia_encoder->height;

    // Construct frame packets
    encoder->encoded_frame_size = encoder->nvidia_encoder->frame_size;
    encoder->num_packets = 1;
    encoder->packets[0].data = encoder->nvidia_encoder->frame;
    encoder->packets[0].size = encoder->nvidia_encoder->frame_size;
    encoder->encoded_frame_size += 8;
}

int transfer_ffmpeg_data(VideoEncoder *encoder) {
    /*
        Set encoder metadata according to nvidia encoder members and receive all encoded packets
       from the ffmpeg encoder to the main encoder.

        Arguments:
            encoder (VideoEncoder*): encoder to use

        Returns:
            (int): 0 on success, -1 on failure
    */
    if (encoder->active_encoder != FFMPEG_ENCODER) {
        LOG_ERROR("Encoder is not using ffmpeg, but we are trying to call transfer_ffmpge_data!");
        return -1;
    }
    // If the encoder has been used to encode a frame before we should clear packet data for
    // previously used packets
    if (encoder->num_packets) {
        for (int i = 0; i < encoder->num_packets; i++) {
            av_packet_unref(&encoder->packets[i]);
        }
    }

    // The encoded frame size starts out as sizeof(int) because of the way we send packets. See
    // encode.h and decode.h.
    encoder->encoded_frame_size = sizeof(int);
    encoder->num_packets = 0;
    int res;

    // receive packets until we receive a nonzero code (indicating either an encoding error, or that
    // all packets have been received).
    while ((res = ffmpeg_encoder_receive_packet(encoder->ffmpeg_encoder,
                                                &encoder->packets[encoder->num_packets])) == 0) {
        encoder->encoded_frame_size += 4 + encoder->packets[encoder->num_packets].size;
        encoder->num_packets++;
        if (encoder->num_packets == MAX_ENCODER_PACKETS) {
            LOG_ERROR("TOO MANY PACKETS: REACHED %d", encoder->num_packets);
            return -1;
        }
    }
    if (res < 0) {
        return -1;
    }

    // set iframe metadata
    encoder->out_width = encoder->ffmpeg_encoder->out_width;
    encoder->out_height = encoder->ffmpeg_encoder->out_height;
    encoder->is_iframe = encoder->ffmpeg_encoder->is_iframe;
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

VideoEncoder *create_video_encoder(int in_width, int in_height, int out_width, int out_height,
                                   int bitrate, CodecType codec_type) {
    /*
       Create a video encoder with the specified parameters. Try Nvidia first if available, and fall
       back to FFmpeg if not.

        Arguments:
            in_width (int): Width of the frames that the encoder intakes
            in_height (int): height of the frames that the encoder intakes
            out_width (int): width of the frames that the encoder outputs
            out_height (int): Height of the frames that the encoder outputs
            bitrate (int): bits per second the encoder will encode to
            codec_type (CodecType): Codec (currently H264 or H265) the encoder will use

        Returns:
            (VideoEncoder*): the newly created encoder
     */

#if !USING_SERVERSIDE_SCALE
    out_width = in_width;
    out_height = in_height;
#endif

    VideoEncoder *encoder = (VideoEncoder *)safe_malloc(sizeof(VideoEncoder));
    memset(encoder, 0, sizeof(VideoEncoder));
    encoder->in_width = in_width;
    encoder->in_height = in_height;

#if USING_NVIDIA_CAPTURE_AND_ENCODE
#if USING_SERVERSIDE_SCALE
    LOG_ERROR(
        "Cannot create nvidia encoder, does not accept in_width and in_height when using "
        "serverside scaling");
    encoder->active_encoder = FFMPEG_ENCODER;
#else
    LOG_INFO("Creating nvidia encoder...");
    // find next nonempty entry in nvidia_encoders
    int index = 0;
    while (encoder->nvidia_encoders[index] != NULL && index < NUM_ENCODERS) {
        index++;
    }
    if (index == NUM_ENCODERS) {
        LOG_ERROR("create_video_encoder will replace one of the nvidia encoders!");
        index = 0;
    }
    encoder->nvidia_encoders[index] = create_nvidia_encoder(bitrate, codec_type, out_width, out_height, *get_video_thread_cuda_context_ptr());
    if (!encoder->nvidia_encoder) {
        LOG_ERROR("Failed to create nvidia encoder!");
        encoder->active_encoder = FFMPEG_ENCODER;
    } else {
        LOG_INFO("Created nvidia encoder!");
        // nvidia creation succeeded!
        encoder->active_encoder = NVIDIA_ENCODER;
        encoder->active_encoder_idx = index;
        return encoder;
    }
#endif  // USING_SERVERSIDE_SCALE
#else
    // No nvidia found
    encoder->active_encoder = FFMPEG_ENCODER;
#endif  // USING_NVIDIA_CAPTURE_AND_ENCODE

    LOG_INFO("Creating ffmpeg encoder...");
    encoder->ffmpeg_encoder =
        create_ffmpeg_encoder(in_width, in_height, out_width, out_height, bitrate, codec_type);
    if (!encoder->ffmpeg_encoder) {
        LOG_ERROR("FFmpeg encoder creation failed!");
        return NULL;
    }
    LOG_INFO("Created ffmpeg encoder!");

    encoder->codec_type = codec_type;

    return encoder;
}

int video_encoder_encode(VideoEncoder *encoder) {
    /*
        Encode a frame using the encoder and store the resulting packets into encoder->packets. If
       the frame has alreday been encoded, this function does nothing.

        Arguments:
            encoder (VideoEncoder*): encoder that will encode the frame

        Returns:
            (int): 0 on success, -1 on failure
     */
    if (!encoder) {
        LOG_ERROR("Tried to video_encoder_encode with a NULL encoder!");
        return -1;
    }
    switch (encoder->active_encoder) {
        case NVIDIA_ENCODER:
#ifdef __linux__
            nvidia_encoder_encode(encoder->nvidia_encoder);
            transfer_nvidia_data(encoder);
            return 0;
#else
            LOG_FATAL("NVIDIA_ENCODER should not be used on Windows!");
#endif
        case FFMPEG_ENCODER:
            if (ffmpeg_encoder_send_frame(encoder->ffmpeg_encoder)) {
                LOG_ERROR("Unable to send frame to encoder!");
                return -1;
            }
            return transfer_ffmpeg_data(encoder);
        default:
            LOG_ERROR("Unknown encoder type: %d!", encoder->active_encoder);
            return -1;
    }
}

bool reconfigure_encoder(VideoEncoder *encoder, int width, int height, int bitrate,
                         CodecType codec) {
    /*
        Attempt to reconfigure the encoder to use the specified width, height, bitrate, and codec.
       Unavailable on FFmpeg, but possible on Nvidia.

        Arguments:
            encoder (VideoEncoder*): encoder to reconfigure
            width (int): new width
            height (int): new height
            bitrate (int): new bitrate
            codec (CodecType): new codec

        Returns:
            (bool): true on success, false on failure
    */
    if (!encoder) {
        LOG_ERROR("Calling reconfigure_encoder on a NULL encoder!");
        return false;
    }
    encoder->in_width = width;
    encoder->in_height = height;
    encoder->codec_type = codec;
    // reconfigure both encoders
    bool nvidia_reconfigured = true;
    if (encoder->nvidia_encoder) {
#ifdef __linux__
        // NOTE: nvidia reconfiguration is currently disabled because it breaks CUDA resource
        // registration somehow.
        nvidia_reconfigured =
            nvidia_reconfigure_encoder(encoder->nvidia_encoder, width, height, bitrate, codec);
#else
        LOG_FATAL("NVIDIA_ENCODER should not be used on Windows!");
#endif
    }
    return nvidia_reconfigured && ffmpeg_reconfigure_encoder(encoder->ffmpeg_encoder, width, height,
                                                             width, height, bitrate, codec);
}

void video_encoder_set_iframe(VideoEncoder *encoder) {
    /*
        Set the next frame to be an IDR-frame with SPS/PPS headers.

        Arguments:
            encoder (VideoEncoder*): Encoder containing the frame
    */
    if (!encoder) {
        LOG_ERROR("video_encoder_set_iframe received NULL encoder!");
        return;
    }
    switch (encoder->active_encoder) {
        case NVIDIA_ENCODER:
#ifdef __linux__
            nvidia_set_iframe(encoder->nvidia_encoder);
            return;
#else
            LOG_FATAL("NVIDIA_ENCODER should not be used on Windows!");
#endif
        case FFMPEG_ENCODER:
            ffmpeg_set_iframe(encoder->ffmpeg_encoder);
            return;
        default:
            LOG_ERROR("Unknown encoder type: %d!", encoder->active_encoder);
            return;
    }
}

void destroy_video_encoder(VideoEncoder *encoder) {
    /*
        Destroy all components of the encoder, then free the encoder itself.

        Arguments:
            encoder (VideoEncoder*): encoder to destroy
    */
    LOG_INFO("Destroying video encoder...");

    // Check if encoder exists
    if (encoder == NULL) {
        LOG_ERROR("NULL Encoder passed into destroy_video_encoder, nothing will be destroyed.");
        return;
    }

    // Destroy the nvidia encoder, if any
    if (encoder->nvidia_encoder) {
#ifdef __linux__
        LOG_INFO("Destroying nvidia encoder...");
        destroy_nvidia_encoder(encoder->nvidia_encoder);
        LOG_INFO("Done destroying nvidia encoder!");
#else
        LOG_FATAL("NVIDIA_ENCODER should not be used on Windows!");
#endif
    }
    if (encoder->ffmpeg_encoder) {
        LOG_INFO("Destroying ffmpeg encoder...");
        destroy_ffmpeg_encoder(encoder->ffmpeg_encoder);
        LOG_INFO("Done destroying ffmpeg encoder!");
    }

    // free packets
    for (int i = 0; i < MAX_ENCODER_PACKETS; i++) {
        av_packet_unref(&encoder->packets[i]);
    }
    free(encoder);

    LOG_INFO("Done destroying encoder!");
}
