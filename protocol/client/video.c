/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file video.c
 * @brief This file contains all code that interacts directly with receiving and
 *        processing video packets on the client.
============================
Usage
============================

initVideo() gets called before any video packet can be received. The video
packets are received as standard WhistPackets by ReceiveVideo(WhistPacket*
packet), before being saved in a proper video frame format.
*/

/*
============================
Includes
============================
*/

#include "video.h"
#include "sdl_utils.h"
#include <whist/network/network.h>
#include <whist/network/ringbuffer.h>
#include <whist/video/codec/decode.h>
#include <whist/core/whist_frame.h>

#include <stdio.h>

#include <SDL2/SDL.h>
#include <whist/utils/color.h>
#include <whist/utils/png.h>
#include <whist/logging/log_statistic.h>
#include <whist/utils/rwlock.h>
#include "whist/core/features.h"
#include "whist/utils/command_line.h"
#include "native_window_utils.h"
#include "network.h"
#include "client_utils.h"
#include <whist/debug/protocol_analyzer.h>

#define USE_HARDWARE_DECODE_DEFAULT true
#define NO_NACKS_DURING_IFRAME false

static bool use_hardware_decode = USE_HARDWARE_DECODE_DEFAULT;
COMMAND_LINE_BOOL_OPTION(use_hardware_decode, 0, "hardware-decode",
                         "Set whether to use hardware decode.")

// Number of videoframes to have in the ringbuffer
#define RECV_FRAMES_BUFFER_SIZE 275

/*
============================
Custom Types
============================
*/

struct VideoContext {
    // Variables needed for rendering
    VideoDecoder* decoder;
    struct SwsContext* sws;

    // Stores metadata from the last rendered frame,
    // So that we know if the encoder/sws and such must
    // be reinitialized to a new width/height/codec
    int last_frame_width;
    int last_frame_height;
    CodecType last_frame_codec;

    bool has_video_rendered_yet;

    // Context of the frame that is currently being rendered
    VideoFrame* render_context;
    bool pending_render_context;
};

/*
============================
Private Functions
============================
*/

/**
 * @brief                          Syncs video_context->video_decoder to
 *                                 the width/height/format of the given VideoFrame.
 *                                 This may potentially recreate the video decoder.
 *
 * @param video_context            The video context being used
 *
 * @param frame                    The frame that we will soon attempt to decode
 */
static void sync_decoder_parameters(VideoContext* video_context, VideoFrame* frame);

/**
 * @brief                          Updates the sws context of the video_context,
 *                                 so that it can convert the format of the capture image
 *                                 to the format that SDL will use to render.
 *                                 If the formats already align,
 *                                 video_context.sws will be set to NULL
 *
 * @param video_context            The video context being used
 *
 * @param frame                    Current input frame to determine necessary scaling for.
 */
static void update_sws_context(VideoContext* video_context, const AVFrame* frame);

/**
 * @brief                          Destroys an ffmpeg decoder on another thread
 *
 * @param opaque                   The VideoDecoder* to destroy
 */
static int32_t multithreaded_destroy_decoder(void* opaque);

/*
============================
Public Function Implementations
============================
*/

VideoContext* init_video(int initial_width, int initial_height) {
    VideoContext* video_context = safe_malloc(sizeof(*video_context));
    memset(video_context, 0, sizeof(*video_context));

    video_context->has_video_rendered_yet = false;
    video_context->sws = NULL;
    video_context->render_context = NULL;
    video_context->pending_render_context = false;
    VideoDecoder* decoder =
        create_video_decoder(initial_width, initial_height, use_hardware_decode, CODEC_TYPE_H264);
    if (!decoder) {
        LOG_FATAL("ERROR: Decoder could not be created!");
    }
    video_context->decoder = decoder;

    video_context->last_frame_width = initial_width;
    video_context->last_frame_height = initial_height;
    video_context->last_frame_codec = CODEC_TYPE_H264;

    sdl_render_framebuffer();

    // Return the new struct
    return (VideoContext*)video_context;
}

void destroy_video(VideoContext* video_context) {
    // Destroy the ffmpeg decoder, if any exists
    if (video_context->decoder) {
        WhistThread destroy_decoder_thread = whist_create_thread(
            multithreaded_destroy_decoder, "multithreaded_destroy_decoder", video_context->decoder);
        whist_detach_thread(destroy_decoder_thread);
        video_context->decoder = NULL;
    }

    // Destroy the sws context, if any exists
    if (video_context->sws) {
        sws_freeContext(video_context->sws);
        video_context->sws = NULL;
    }

    // Free the video context
    free(video_context);
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
void receive_video(VideoContext* video_context, VideoFrame* video_frame) {
    // TODO: Move to ringbuffer.c
    // LOG_INFO("Video Packet ID %d, Index %d (Packets: %d) (Size: %d)",
    // packet->id, packet->index, packet->num_indices, packet->payload_size);

    if (!video_context->pending_render_context) {
        // Send a frame to the renderer,
        // then set video_context->pending_render_context to true to signal readiness

        whist_analyzer_record_pending_rendering(PACKET_VIDEO);
        // give data pointer to the video context
        video_context->render_context = video_frame;
        log_double_statistic(VIDEO_FPS_RENDERED, 1.0);
        // signal to the renderer that we're ready
        video_context->pending_render_context = true;
    } else {
        LOG_ERROR("We tried to send the video context a frame when it wasn't ready!");
    }
}

int render_video(VideoContext* video_context) {
    WhistTimer statistics_timer;

    // Information needed to render a FrameData* to the screen
    // We make this static so that even if `sdl_render_pending` happens,
    // a later call to render_video can still access the data
    // from the most recently consumed render context.
    static WhistRGBColor window_color = {0};
    static timestamp_us server_timestamp = 0;
    static timestamp_us client_input_timestamp = 0;
    static timestamp_us last_rendered_time = 0;

    // Receive and process a render context that's being pushed
    if (video_context->pending_render_context) {
        // Grab and consume the actual frame
        VideoFrame* frame = video_context->render_context;

        // If server thinks the window isn't visible, but the window is visible now,
        // Send a START_STREAMING message
        if (!frame->is_window_visible && sdl_is_window_visible()) {
            // The server thinks the client window is occluded/minimized, but it isn't. So
            // we'll correct it. NOTE: Most of the time, this is just because there was a
            // delay between the window losing visibility and the server reacting.
            LOG_INFO(
                "Server thinks the client window is occluded/minimized, but it isn't. So, Start "
                "Streaming");
            WhistClientMessage wcmsg = {0};
            wcmsg.type = MESSAGE_START_STREAMING;
            send_wcmsg(&wcmsg);
        }

        whist_analyzer_record_decode_video();
        if (!frame->is_empty_frame) {
            if (FEATURE_ENABLED(LONG_TERM_REFERENCE_FRAMES)) {
                // Indicate to the server that this frame is received
                // in full and will be decoded.
                if (LOG_LONG_TERM_REFERENCE_FRAMES) {
                    LOG_INFO("LTR: send frame ack for frame ID %d (%s).", frame->frame_id,
                             video_frame_type_string(frame->frame_type));
                }
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_FRAME_ACK;
                wcmsg.frame_ack.frame_id = frame->frame_id;
                send_wcmsg(&wcmsg);
            }

            sync_decoder_parameters(video_context, frame);
            int ret;
            server_timestamp = frame->server_timestamp;
            client_input_timestamp = frame->client_input_timestamp;
            TIME_RUN(
                ret = video_decoder_send_packets(video_context->decoder, get_frame_videodata(frame),
                                                 frame->videodata_length),
                VIDEO_DECODE_SEND_PACKET_TIME, statistics_timer);
            if (ret < 0) {
                LOG_ERROR("Failed to send packets to decoder, unable to render frame");
                video_context->pending_render_context = false;
                return -1;
            }

            window_color = frame->corner_color;

            WhistCursorInfo* frame_cursor_image = get_frame_cursor_info(frame);

            // set the cursor image as pending, so that it will be rendered in main.
            if (frame_cursor_image) {
                sdl_set_cursor_info_as_pending(frame_cursor_image);
            }
        } else {
            // Reset last_rendered_time for an empty frame, so that a non-empty frame following an
            // empty frame will not have a huge/wrong VIDEO_CAPTURE_LATENCY.
            last_rendered_time = 0;
        }

        // Mark as received so render_context can be overwritten again
        video_context->pending_render_context = false;
    }

    // Try to keep decoding frames from the decoder, if we can
    // Use static so we can remember, even if sdl_render_pending exits early.
    static bool got_frame_from_decoder = false;
    while (video_context->decoder != NULL) {
        int res;
        TIME_RUN(res = video_decoder_decode_frame(video_context->decoder),
                 VIDEO_DECODE_GET_FRAME_TIME, statistics_timer);
        if (res < 0) {
            LOG_ERROR("Error getting frame from decoder!");
            return -1;
        }

        if (res == 0) {
            // Mark that we got at least one frame from the decoder
            got_frame_from_decoder = true;
        } else {
            // Exit once we get EAGAIN
            break;
        }
    }

    // Render any frame we got from the decoder
    if (got_frame_from_decoder) {
        if (sdl_render_pending()) {
            // We cannot call `video_decoder_free_decoded_frame`,
            // until the renderer is done rendering the previously decoded frame data.
            // So, we skip renders until after the previous render is done.

            // However, We only skip a render after setting `pending_render_context = false`,
            // To make sure we can keep consuming frames and keep up-to-date.

            // And, We only skip a render after calling `video_decoder_decode_frame`, because the
            // internal decoder buffer overflow errors if we don't actively decode frames fast
            // enough.

            return 1;
        }

        // Mark frame as consumed
        got_frame_from_decoder = false;

        // Get the last decoded frame
        DecodedFrameData decoded_frame_data =
            video_decoder_get_last_decoded_frame(video_context->decoder);

        // Make a new frame to give to the renderer.
        AVFrame* frame = av_frame_alloc();
        FATAL_ASSERT(frame);

        // Fill our local frame with the correct data, which will be a
        // reference to the decoded frame if formats make that possible
        // or a new frame made with swscale if not.
        if (decoded_frame_data.using_hw) {
            av_frame_ref(frame, decoded_frame_data.decoded_frame);
        } else {
            // Update the scaler context with the properties of the next
            // frame, or destroy the scaler context if no scale will be
            // needed.
            update_sws_context(video_context, decoded_frame_data.decoded_frame);

            if (video_context->sws) {
                // Convert from the decoded frame into our target frame.
                sws_scale_frame(video_context->sws, frame, decoded_frame_data.decoded_frame);
            } else {
                // No conversion required.
                av_frame_ref(frame, decoded_frame_data.decoded_frame);
            }
        }

        // Free the decoded frame.  We have either copied the data to
        // our own frame or made another reference to it.
        video_decoder_free_decoded_frame(&decoded_frame_data);

        // Render out the cursor image
        if (cursor_image) {
            // TODO: whist_frontend_set_cursor here instead of sdl_update_cursor
            TIME_RUN(sdl_update_cursor(cursor_image), VIDEO_CURSOR_UPDATE_TIME, statistics_timer);
            // Cursors need not be double-rendered, so we just unset the cursor image here
            free(cursor_image);
            cursor_image = NULL;
        }

        // Update the window titlebar color
        sdl_render_window_titlebar_color(window_color);

        // Render the decoded frame
        sdl_update_framebuffer(frame);

        // Mark the framebuffer out to render
        sdl_render_framebuffer();

        // Declare user activity to prevent screensaver
        declare_user_activity();

        if (client_input_timestamp != 0) {
            // Calculate E2E latency
            // Get the difference in time from the moment client pressed user-input to now.
            timestamp_us pipeline_latency = current_time_us() - client_input_timestamp;
            log_double_statistic(VIDEO_PIPELINE_LATENCY, (double)(pipeline_latency / 1000));

            // But client_input_timestamp used above does not include time it took between
            // user-input to frame capture in server-side. Please refer to server\video.c to
            // understand how client_input_timestamp is calculated. But "Latency from user-click to
            // frame capture" cannot be calculated accurately. So we consider the worst-case of
            // "Server time elapsed since last captute". We are doing this calculation on the
            // client-side, since server cannot predict for frame drops due to packet drops.
            timestamp_us capture_latency = 0;
            if (last_rendered_time != 0) {
                capture_latency = (server_timestamp - last_rendered_time);
                log_double_statistic(VIDEO_CAPTURE_LATENCY, (double)(capture_latency / 1000));
            }
            log_double_statistic(VIDEO_E2E_LATENCY,
                                 (double)((pipeline_latency + capture_latency) / 1000));
        }
        last_rendered_time = server_timestamp;

        video_context->has_video_rendered_yet = true;

        // Track time between consecutive frames
        static WhistTimer last_frame_timer;
        static bool last_frame_timer_started = false;
        if (last_frame_timer_started) {
            log_double_statistic(VIDEO_TIME_BETWEEN_FRAMES,
                                 get_timer(&last_frame_timer) * MS_IN_SECOND);
        }
        start_timer(&last_frame_timer);
        last_frame_timer_started = true;
    }

    return 0;
}

bool has_video_rendered_yet(VideoContext* video_context) {
    return video_context->has_video_rendered_yet;
}

/*
============================
Private Function Implementations
============================
*/

void sync_decoder_parameters(VideoContext* video_context, VideoFrame* frame) {
    if (frame->width == video_context->last_frame_width &&
        frame->height == video_context->last_frame_height &&
        frame->codec_type == video_context->last_frame_codec) {
        // The width/height/codec are the same, so there's nothing to change
        return;
    }

    if (frame->frame_type != VIDEO_FRAME_TYPE_INTRA) {
        LOG_INFO("Wants to change resolution, but not an I-Frame!");
        return;
    }

    LOG_INFO(
        "Updating client rendering to match server's width and "
        "height and codec! "
        "From %dx%d (codec %d), to %dx%d (codec %d)",
        video_context->last_frame_width, video_context->last_frame_height,
        video_context->last_frame_codec, frame->width, frame->height, frame->codec_type);
    if (video_context->decoder) {
        WhistThread destroy_decoder_thread = whist_create_thread(
            multithreaded_destroy_decoder, "multithreaded_destroy_decoder", video_context->decoder);
        whist_detach_thread(destroy_decoder_thread);
        video_context->decoder = NULL;
    }

    VideoDecoder* decoder =
        create_video_decoder(frame->width, frame->height, use_hardware_decode, frame->codec_type);
    if (!decoder) {
        LOG_FATAL("ERROR: Decoder could not be created!");
    }
    video_context->decoder = decoder;

    video_context->last_frame_width = frame->width;
    video_context->last_frame_height = frame->height;
    video_context->last_frame_codec = frame->codec_type;
}

void update_sws_context(VideoContext* video_context, const AVFrame* frame) {
    static enum AVPixelFormat cached_format = AV_PIX_FMT_NONE;
    static int cached_width = -1;
    static int cached_height = -1;

    // If the cache missed, reconstruct the sws and static avimage
    if (frame->format != cached_format || frame->width != cached_width ||
        frame->height != cached_height) {
        cached_format = frame->format;
        cached_width = frame->width;
        cached_height = frame->height;

        // No matter what, we now should destroy the old context if it exists
        if (video_context->sws) {
            sws_freeContext(video_context->sws);
            video_context->sws = NULL;
        }

        if (frame->format == WHIST_CLIENT_FRAMEBUFFER_PIXEL_FORMAT) {
            // If the pixel format already matches, we require no pixel format conversion,
            // we can just pass directly to the renderer!
            LOG_INFO("The input format is already correct, no sws needed!");
        } else {
            // We need to create a new context to handle the pixel fmt conversion
            LOG_INFO("Creating sws context to convert from %s to %s",
                     av_get_pix_fmt_name(frame->format),
                     av_get_pix_fmt_name(WHIST_CLIENT_FRAMEBUFFER_PIXEL_FORMAT));
            video_context->sws = sws_getContext(
                frame->width, frame->height, frame->format, frame->width, frame->height,
                WHIST_CLIENT_FRAMEBUFFER_PIXEL_FORMAT, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        }
    }
}

int32_t multithreaded_destroy_decoder(void* opaque) {
    VideoDecoder* decoder = (VideoDecoder*)opaque;
    destroy_video_decoder(decoder);
    return 0;
}

bool video_ready_for_frame(VideoContext* context) { return !context->pending_render_context; }
