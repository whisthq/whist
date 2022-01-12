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
#include "sdlscreeninfo.h"
#include "native_window_utils.h"
#include "network.h"
#include "client_utils.h"
#include "client_statistic.h"

#define USE_HARDWARE true
#define NO_NACKS_DURING_IFRAME false

// Global Variables

extern volatile double latency;

// Number of videoframes to have in the ringbuffer
#define RECV_FRAMES_BUFFER_SIZE 275

/*
============================
Custom Types
============================
*/

struct VideoContext {
    // Variables needed for rendering
    RingBuffer* ring_buffer;
    VideoDecoder* decoder;
    struct SwsContext* sws;

    // Stores metadata from the last rendered frame,
    // So that we know if the encoder/sws and such must
    // be reinitialized to a new width/height/codec
    int last_frame_width;
    int last_frame_height;
    CodecType last_frame_codec;

    FrameData* pending_ctx;
    int frames_received;
    int bytes_transferred;
    int last_statistics_id;
    int last_rendered_id;
    int most_recent_iframe;

    WhistTimer last_iframe_request_timer;

    // Loading animation data
    int loading_index;
    WhistTimer last_loading_frame_timer;
    bool has_video_rendered_yet;

    // Context of the frame that is currently being rendered
    FrameData* render_context;
    bool pending_render_context;

    // Statistics calculator
    WhistTimer network_statistics_timer;
    NetworkStatistics network_statistics;
    bool statistics_initialized;
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
 * @param data                     The pointers to the texture pointers that sws will write to.
 *                                 This function will write into this array.
 * @param linesize                 The pointers to the linesize array that sws will write to
 *                                 This function will write into this array.
 * @param input_width              The width of the capture image.
 * @param input_height             The height of the capture image.
 * @param input_format             The format of the capture image.
 */
static void update_sws_context(VideoContext* video_context, Uint8** data, int* linesize, int width,
                               int height, enum AVPixelFormat input_format);

// TODO: Refactor into ringbuffer.c, so that ringbuffer.h
// is what exposes theese functions
static void calculate_statistics(VideoContext* video_context);
static void try_recovering_missing_packets_or_frames(VideoContext* video_context);
static void skip_to_next_iframe(VideoContext* video_context);

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

VideoContext* init_video() {
    VideoContext* video_context = safe_malloc(sizeof(*video_context));
    memset(video_context, 0, sizeof(*video_context));

    // Initialize everything
    video_context->ring_buffer =
        init_ring_buffer(PACKET_VIDEO, RECV_FRAMES_BUFFER_SIZE, nack_packet);
    video_context->last_frame_width = -1;
    video_context->last_frame_height = -1;
    video_context->last_frame_codec = CODEC_TYPE_UNKNOWN;
    video_context->has_video_rendered_yet = false;
    video_context->sws = NULL;
    video_context->pending_ctx = NULL;
    video_context->frames_received = 0;
    video_context->bytes_transferred = 0;
    video_context->last_statistics_id = 1;
    video_context->last_rendered_id = 0;
    video_context->most_recent_iframe = -1;
    video_context->render_context = NULL;
    video_context->pending_render_context = false;
    video_context->statistics_initialized = false;

    start_timer(&video_context->last_iframe_request_timer);

    // Init loading animation variables
    video_context->loading_index = 0;
    start_timer(&video_context->last_loading_frame_timer);
    // Present first frame of loading animation
    sdl_update_framebuffer_loading_screen(video_context->loading_index);
    sdl_render_framebuffer();
    // Then progress the animation
    video_context->loading_index++;

    // Return the new struct
    return (VideoContext*)video_context;
}

void destroy_video(VideoContext* video_context) {
    // Destroy the ring buffer
    destroy_ring_buffer(video_context->ring_buffer);
    video_context->ring_buffer = NULL;

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
void receive_video(VideoContext* video_context, WhistPacket* packet) {
    // The next two lines are commented out, but left in the codebase to be
    // easily toggled back and forth during development. We considered putting
    // this under the LOG_VIDEO ifdef, but decided not to, since these lines
    // log every single packet, which is too verbose for standard video
    // logging.
    // LOG_INFO("Video Packet ID %d, Index %d (Packets: %d) (Size: %d)",
    // packet->id, packet->index, packet->num_indices, packet->payload_size);
    // Find frame in ring buffer that matches the id
    // ===========================
    // BEGIN NEW CODE
    // ===========================
    if (!video_context->pending_render_context) {
        // send a frame to the renderer and set video_context->pending_render_context to true to signal readiness
        if (video_context->last_rendered_id >= 0) {
            // give data pointer to the video context
            video_context->render_context = packet->data;
            log_double_statistic(VIDEO_FPS_RENDERED, 1.0);
            // increment last_rendered_id
            LOG_DEBUG("received packet with ID %d, video last rendered %d", packet->id, video_context->last_rendered_id);
            video_context->last_rendered_id = packet->id;
            // record that we got an iframe
            if (video_context->render_context->is_iframe) {
                video_context->most_recent_iframe =
                    max(video_context->most_recent_iframe, packet->id);
            }
            // signal to the renderer that we're ready
            video_context->pending_render_context = true;
        }
    } else {
        LOG_ERROR("We tried to send the video context a frame when it wasn't ready!");
    }
    // ===========================
    // END NEW CODE -  DELETE CODE AFTER THIS
    // ===========================
}

int render_video(VideoContext* video_context) {
    WhistTimer statistics_timer;

    // Information needed to render a FrameData* to the screen
    // We make this static so that even if `sdl_render_pending` happens,
    // a later call to render_video can still access the data
    // from the most recently consumed render context.
    static WhistRGBColor window_color = {0};
    static WhistCursorImage cursor_image = {0};
    static bool has_cursor_image = false;
    static FrameData frame_data = {0};
    static timestamp_us server_timestamp = 0;
    static timestamp_us client_input_timestamp = 0;

    // Receive and process a render context that's being pushed
    if (video_context->pending_render_context) {
        // in our new setup, the video context just receives the frame as it was sent from the server and can render it - no need to look at frame_data->frame.
        // Save frame data, drop volatile
        frame_data = *(FrameData*)video_context->render_context;
        // Grab and consume the actual frame
        VideoFrame* frame = (VideoFrame*)frame_data.frame_buffer;

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

        if (!frame->is_empty_frame) {
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

            WhistCursorImage* cursor_image_ptr = get_frame_cursor_image(frame);
            if (cursor_image_ptr) {
                cursor_image = *cursor_image_ptr;
                has_cursor_image = true;
            } else {
                has_cursor_image = false;
            }
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

    if (sdl_render_pending()) {
        // We cannot call `video_decoder_free_decoded_frame`,
        // until the renderer is done rendering the previously decoded frame data.
        // So, we skip renders until after the previous render is done.

        // However, We only skip a render after setting `pending_render_context = false`,
        // To make sure we can keep consuming frames and keep up-to-date.

        // And, We only skip a render after calling `video_decoder_decode_frame`, because the
        // internal decoder buffer overflow errors if we don't actively decode frames fast enough.

        return 0;
    }

    // Render any frame we got from the decoder
    if (got_frame_from_decoder) {
        // Mark frame as consumed
        got_frame_from_decoder = false;

        // The final data/linesize that we will render
        Uint8* data[4] = {0};
        int linesize[4] = {0};

        static DecodedFrameData decoded_frame_data = {0};

        // Free the previous frame, if there was any
        video_decoder_free_decoded_frame(&decoded_frame_data);

        // Get the last decoded frame
        decoded_frame_data = video_decoder_get_last_decoded_frame(video_context->decoder);

        // Pull the most recently decoded data/linesize from the decoder
        if (decoded_frame_data.using_hw) {
            data[0] = data[1] = decoded_frame_data.decoded_frame->data[3];
            linesize[0] = linesize[1] = decoded_frame_data.width;
        } else {
            // This will allocate the sws target frame into data/linesize,
            // using the provided pixel_format
            update_sws_context(video_context, data, linesize, video_context->decoder->width,
                               video_context->decoder->height, decoded_frame_data.pixel_format);
            if (video_context->sws) {
                // Scale from the swframe into the sws target frame.
                sws_scale(video_context->sws,
                          (uint8_t const* const*)decoded_frame_data.decoded_frame->data,
                          decoded_frame_data.decoded_frame->linesize, 0, decoded_frame_data.height,
                          data, linesize);
            } else {
                memcpy(data, decoded_frame_data.decoded_frame->data, sizeof(data));
                memcpy(linesize, decoded_frame_data.decoded_frame->linesize, sizeof(linesize));
            }
        }

        // Invalidate loading animation once rendering occurs
        video_context->loading_index = -1;

        // Render out the cursor image
        if (has_cursor_image) {
            TIME_RUN(sdl_update_cursor(&cursor_image), VIDEO_CURSOR_UPDATE_TIME, statistics_timer);
        }

        // Update the window titlebar color
        sdl_render_window_titlebar_color(window_color);

        // Render the decoded frame
        sdl_update_framebuffer(data, linesize, video_context->decoder->width,
                               video_context->decoder->height);

        // Mark the framebuffer out to render
        sdl_render_framebuffer();

        // Declare user activity to prevent screensaver
        declare_user_activity();

#if LOG_VIDEO
        LOG_DEBUG("Rendered %d (Size: %d) (Age %f)", frame_data.id, frame_data.frame_buffer_size,
                  get_timer(&frame_data.frame_creation_timer));
#endif

        static timestamp_us last_rendered_time = 0;

        // Calculate E2E latency
        // Get the difference in time from the moment client pressed user-input to now.
        timestamp_us e2e_latency = current_time_us() - client_input_timestamp;
        // But client_input_timestamp used above does not include time it took between user-input to
        // frame refresh in server-side. Please refer to server\video.c to understand how
        // client_input_timestamp is calculated.
        // But "Latency from user-click to frame refresh" cannot be calculated accurately.
        // We approximate it as "Server time elapsed since last render"/2.
        // We are doing this calculation on the client-side, since server cannot predict for frame
        // drops due to packet drops.
        if (last_rendered_time != 0) {
            e2e_latency += (server_timestamp - last_rendered_time) / 2;
        }
        last_rendered_time = server_timestamp;
        log_double_statistic(VIDEO_E2E_LATENCY, (double)(e2e_latency / 1000));

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
    } else if (video_context->loading_index >= 0) {
        // If we didn't get a frame, and loading_index is valid,
        // Render the loading animation
        const float loading_animation_fps = 20.0;
        if (get_timer(&video_context->last_loading_frame_timer) > 1 / loading_animation_fps) {
            // Present the loading screen
            sdl_update_framebuffer_loading_screen(video_context->loading_index);
            sdl_render_framebuffer();
            // Progress animation
            video_context->loading_index++;
            // Reset timer
            start_timer(&video_context->last_loading_frame_timer);
        }
    }

    return 0;
}

bool has_video_rendered_yet(VideoContext* video_context) {
    return video_context->has_video_rendered_yet;
}

NetworkStatistics get_video_network_statistics(VideoContext* video_context) {
    return video_context->network_statistics;
}

/*
============================
Private Function Implementations
============================
*/

void sync_decoder_parameters(VideoContext* video_context, VideoFrame* frame) {
    /*
        Update decoder parameters to match the server's width, height, and codec
        of the VideoFrame* that is currently being received

        Arguments:
            frame (VideoFrame*): next frame to render
    */
    if (frame->width == video_context->last_frame_width &&
        frame->height == video_context->last_frame_height &&
        frame->codec_type == video_context->last_frame_codec) {
        // The width/height/codec are the same, so there's nothing to change
        return;
    }

    if (!frame->is_iframe) {
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
        create_video_decoder(frame->width, frame->height, USE_HARDWARE, frame->codec_type);
    if (!decoder) {
        LOG_FATAL("ERROR: Decoder could not be created!");
    }
    video_context->decoder = decoder;

    video_context->last_frame_width = frame->width;
    video_context->last_frame_height = frame->height;
    video_context->last_frame_codec = frame->codec_type;
}

void update_sws_context(VideoContext* video_context, Uint8** data, int* linesize, int width,
                        int height, enum AVPixelFormat input_format) {
    /*
        Create an SWS context as needed to perform pixel format
        conversions, destroying any existing context first. If no
        context is needed, return false. If the existing context is
        still valid, return true. This function also updates the
        data and linesize arrays by properly allocating/maintaining
        an av_image to house the converted data.

        Arguments:
            data (Uint8**): pointer to the data array to be filled in
            linesize (int*): pointer to the linesize array to be filled in
            input_format (AVPixelFormat): the pixel format that the data is in
    */

    static enum AVPixelFormat cached_format = AV_PIX_FMT_NONE;
    static int cached_width = -1;
    static int cached_height = -1;

    static Uint8* static_data[4] = {0};
    static int static_linesize[4] = {0};

    // If the cache missed, reconstruct the sws and static avimage
    if (input_format != cached_format || width != cached_width || height != cached_height) {
        cached_format = input_format;
        cached_width = width;
        cached_height = height;

        // No matter what, we now should destroy the old context if it exists
        if (static_data[0] != NULL) {
            av_freep(&static_data[0]);
            static_data[0] = NULL;
        }
        if (video_context->sws) {
            sws_freeContext(video_context->sws);
            video_context->sws = NULL;
        }

        if (input_format == SDL_FRAMEBUFFER_PIXEL_FORMAT) {
            // If the pixel format already matches, we require no pixel format conversion,
            // we can just pass directly to the renderer!
            LOG_INFO("The input format is already correct, no sws needed!");
        } else {
            // We need to create a new context to handle the pixel fmt conversion
            LOG_INFO("Creating sws context to convert from %s to %s",
                     av_get_pix_fmt_name(input_format),
                     av_get_pix_fmt_name(SDL_FRAMEBUFFER_PIXEL_FORMAT));
            video_context->sws =
                sws_getContext(width, height, input_format, width, height,
                               SDL_FRAMEBUFFER_PIXEL_FORMAT, SWS_FAST_BILINEAR, NULL, NULL, NULL);
            av_image_alloc(static_data, static_linesize, width, height,
                           SDL_FRAMEBUFFER_PIXEL_FORMAT, 32);
        }
    }

    // Copy the static avimage into the provided data/linesize buffers.
    if (video_context->sws) {
        memcpy(data, static_data, sizeof(static_data));
        memcpy(linesize, static_linesize, sizeof(static_linesize));
    }
}

void calculate_statistics(VideoContext* video_context) {
    if (!video_context->statistics_initialized) {
        start_timer(&video_context->network_statistics_timer);
        video_context->statistics_initialized = true;
    }

    RingBuffer* ring_buffer = video_context->ring_buffer;
    // do some calculation
    // Update mbps every STATISTICS_SECONDS seconds
#define STATISTICS_SECONDS 5
    if (get_timer(&video_context->network_statistics_timer) > STATISTICS_SECONDS) {
        NetworkStatistics network_statistics = {0};
        network_statistics.num_nacks_per_second =
            ring_buffer->num_packets_nacked / STATISTICS_SECONDS;
        network_statistics.num_received_packets_per_second =
            ring_buffer->num_packets_received / STATISTICS_SECONDS;
        network_statistics.num_rendered_frames_per_second =
            ring_buffer->num_frames_rendered / STATISTICS_SECONDS;
        network_statistics.statistics_gathered = true;
        video_context->network_statistics = network_statistics;

        ring_buffer->num_packets_nacked = 0;
        ring_buffer->num_packets_received = 0;
        ring_buffer->num_frames_rendered = 0;
        start_timer(&video_context->network_statistics_timer);
    }
}

void try_recovering_missing_packets_or_frames(VideoContext* video_context) {
// If we want an iframe, this is how often we keep asking for it
#define IFRAME_REQUEST_INTERVAL_MS 5.0
// Number of frames ahead we can be before asking for iframe
#define MAX_UNSYNCED_FRAMES 4
// How stale the next-to-render-frame can be before asking for an iframe
#define MAX_TO_RENDER_STALENESS 100.0

    // Try to nack for any missing packets
    bool nacking_succeeded = try_nacking(video_context->ring_buffer, latency);

    // Get how stale the frame we're trying to render is
    double next_to_render_staleness = -1.0;
    int next_render_id = video_context->last_rendered_id + 1;
    FrameData* ctx = get_frame_at_id(video_context->ring_buffer, next_render_id);
    if (ctx->id == next_render_id) {
        next_to_render_staleness = get_timer(&ctx->frame_creation_timer);
    }

    static bool already_logged_iframe_request = false;

    // If nacking has failed to recover the packets we need,
    if (!nacking_succeeded
        // Or if we're more than MAX_UNSYNCED_FRAMES frames out-of-sync,
        ||
        video_context->ring_buffer->max_id > video_context->last_rendered_id + MAX_UNSYNCED_FRAMES
        // Or we've spent MAX_TO_RENDER_STALENESS trying to render this one frame
        || (next_to_render_staleness > MAX_TO_RENDER_STALENESS / MS_IN_SECOND)) {
        // THEN, we should send an iframe request

        // Throttle the requests to prevent network upload saturation, however
        if (get_timer(&video_context->last_iframe_request_timer) >
            IFRAME_REQUEST_INTERVAL_MS / MS_IN_SECOND) {
            int last_failed_id = max(next_render_id, video_context->ring_buffer->max_id - 1);
            // Send a video packet stream reset request
            WhistClientMessage wcmsg = {0};
            wcmsg.type = MESSAGE_STREAM_RESET_REQUEST;
            wcmsg.stream_reset_data.type = PACKET_VIDEO;
            wcmsg.stream_reset_data.last_failed_id = last_failed_id;
            send_wcmsg(&wcmsg);

            // If we haven't already logged the iframe request,
            // Logged the iframe request
            if (!already_logged_iframe_request) {
                LOG_INFO(
                    "The most recent ID %d is %d frames ahead of the most recently rendered frame, "
                    "and the frame we're trying to render has been alive for %fms. "
                    "An I-Frame is now being requested to catch-up.",
                    video_context->ring_buffer->max_id,
                    video_context->ring_buffer->max_id - video_context->last_rendered_id,
                    next_to_render_staleness * MS_IN_SECOND);
#if !LOG_VIDEO
                // Prevent logging every iframe requests, when LOG_VIDEO is false
                already_logged_iframe_request = true;
#endif
            }

            start_timer(&video_context->last_iframe_request_timer);
        }
    } else {
        already_logged_iframe_request = false;
    }
}

void skip_to_next_iframe(VideoContext* video_context) {
    // Only run if we're not rendering or we haven't rendered anything yet
    if (video_context->most_recent_iframe > 0 && video_context->last_rendered_id == -1) {
        // Silently skip to next iframe if it's the start of the stream
        video_context->last_rendered_id = video_context->most_recent_iframe - 1;
    } else if (video_context->most_recent_iframe - 1 > video_context->last_rendered_id) {
        // Loudly declare the skip and log which frames we dropped
        LOG_INFO("Skipping from %d to I-Frame %d", video_context->last_rendered_id,
                 video_context->most_recent_iframe);
        for (int i = max(video_context->last_rendered_id + 1,
                         video_context->most_recent_iframe -
                             video_context->ring_buffer->frames_received + 1);
             i < video_context->most_recent_iframe; i++) {
            FrameData* frame_data = get_frame_at_id(video_context->ring_buffer, i);
            if (frame_data->id == i) {
                LOG_WARNING("Frame dropped with ID %d: %d/%d", i,
                            frame_data->original_packets_received,
                            frame_data->num_original_packets);

                for (int j = 0; j < frame_data->num_original_packets; j++) {
                    if (!frame_data->received_indices[j]) {
                        LOG_WARNING("Did not receive ID %d, Index %d", i, j);
                    }
                }
                reset_frame(video_context->ring_buffer, frame_data);
            } else if (frame_data->id != -1) {
                LOG_WARNING("Bad ID? %d instead of %d", frame_data->id, i);
            }
        }
        video_context->last_rendered_id = video_context->most_recent_iframe - 1;
    }
}

int32_t multithreaded_destroy_decoder(void* opaque) {
    VideoDecoder* decoder = (VideoDecoder*)opaque;
    destroy_video_decoder(decoder);
    return 0;
}

bool video_ready_for_frame(VideoContext* context) {
    return !context->pending_render_context;
}
