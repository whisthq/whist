/**
 * Copyright (c) 2020-2022 Whist Technologies, Inc.
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
#include "bitrate.h"
#include "client_utils.h"
#include "client_statistic.h"

#define USE_HARDWARE true
#define NO_NACKS_DURING_IFRAME false

// Global Variables
extern volatile SDL_Window* window;

extern volatile int server_width;
extern volatile int server_height;
extern volatile CodecType server_codec_type;

// Keeping track of max mbps
extern volatile int client_max_bitrate;
extern volatile int max_burst_bitrate;
extern volatile bool update_bitrate;

extern volatile CodecType output_codec_type;
extern volatile double latency;

// START VIDEO VARIABLES
volatile bool initialized_video = false;
volatile bool initialized_video_buffer = false;

#define BITRATE_BUCKET_SIZE 500000

/*
============================
Custom Types
============================
*/

struct VideoData {
    // Variables needed for rendering
    VideoDecoder* decoder;
    struct SwsContext* sws;

    FrameData* pending_ctx;
    int frames_received;
    int bytes_transferred;
    clock frame_timer;
    int last_statistics_id;
    int last_rendered_id;
    int most_recent_iframe;

    clock last_iframe_request_timer;

    double target_mbps;
    int bucket;  // = STARTING_BITRATE / BITRATE_BUCKET_SIZE;
    int nack_by_bitrate[MAXIMUM_BITRATE / BITRATE_BUCKET_SIZE + 5];
    double seconds_by_bitrate[MAXIMUM_BITRATE / BITRATE_BUCKET_SIZE + 5];

    // Loading animation data
    int loading_index;
    clock last_loading_frame_timer;
} video_data;

// mbps that currently works
volatile double working_mbps;

// Context of the frame that is currently being rendered
static volatile FrameData* render_context;
static volatile bool pushing_render_context = false;

// Hold information about frames as the packets come in
#define RECV_FRAMES_BUFFER_SIZE 275
RingBuffer* video_ring_buffer;

bool has_video_rendered_yet = false;

// END VIDEO VARIABLES

// START VIDEO FUNCTIONS

/*
============================
Private Functions
============================
*/

int32_t multithreaded_destroy_decoder(void* opaque);
void calculate_statistics();
void skip_to_next_iframe();
void sync_decoder_parameters(VideoFrame* frame);
// Try to request any missing packets or video frames
void try_recovering_missing_packets_or_frames();
/*
============================
Private Function Implementations
============================
*/

void sync_decoder_parameters(VideoFrame* frame) {
    /*
        Update decoder parameters to match the server's width, height, and codec
        of the VideoFrame* that is currently being received

        Arguments:
            frame (VideoFrame*): next frame to render
    */
    if (frame->width == server_width && frame->height == server_height &&
        frame->codec_type == server_codec_type) {
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
        "From %dx%d, codec %d to %dx%d, codec %d",
        server_width, server_height, server_codec_type, frame->width, frame->height,
        frame->codec_type);
    if (video_data.decoder) {
        WhistThread destroy_decoder_thread = whist_create_thread(
            multithreaded_destroy_decoder, "multithreaded_destroy_decoder", video_data.decoder);
        whist_detach_thread(destroy_decoder_thread);
        video_data.decoder = NULL;
    }

    VideoDecoder* decoder =
        create_video_decoder(frame->width, frame->height, USE_HARDWARE, frame->codec_type);
    if (!decoder) {
        LOG_FATAL("ERROR: Decoder could not be created!");
    }
    video_data.decoder = decoder;

    server_width = frame->width;
    server_height = frame->height;
    server_codec_type = frame->codec_type;
    output_codec_type = frame->codec_type;
}

void update_sws_context(Uint8** data, int* linesize, enum AVPixelFormat input_format) {
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

    VideoDecoder* decoder = video_data.decoder;
    static enum AVPixelFormat cached_format = AV_PIX_FMT_NONE;
    static int cached_width = -1;
    static int cached_height = -1;

    static Uint8* static_data[4] = {0};
    static int static_linesize[4] = {0};

    // If the cache missed, reconstruct the sws and static avimage
    if (input_format != cached_format || decoder->width != cached_width ||
        decoder->height != cached_height) {
        cached_format = input_format;
        cached_width = decoder->width;
        cached_height = decoder->height;

        // No matter what, we now should destroy the old context if it exists
        if (video_data.sws) {
            av_freep(&static_data[0]);
            sws_freeContext(video_data.sws);
            video_data.sws = NULL;
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
            video_data.sws = sws_getContext(
                decoder->width, decoder->height, input_format, decoder->width, decoder->height,
                SDL_FRAMEBUFFER_PIXEL_FORMAT, SWS_FAST_BILINEAR, NULL, NULL, NULL);
            av_image_alloc(static_data, static_linesize, decoder->width, decoder->height,
                           SDL_FRAMEBUFFER_PIXEL_FORMAT, 32);
        }
    }

    // Copy the static avimage into the provided data/linesize buffers.
    if (video_data.sws) {
        memcpy(data, static_data, sizeof(static_data));
        memcpy(linesize, static_linesize, sizeof(static_linesize));
    }
}

// TODO: Refactor into ringbuffer.c, so that ringbuffer.h
// is what exposes the 'try_recovering_missing_packets_or_frames' function
void try_recovering_missing_packets_or_frames() {
    /*
        Try to recover any missing packets or frames
    */

// If we want an iframe, this is how often we keep asking for it
#define IFRAME_REQUEST_INTERVAL_MS 5.0
// Number of frames ahead we can be before asking for iframe
#define MAX_UNSYNCED_FRAMES 4
// How stale the next-to-render-frame can be before asking for an iframe
#define MAX_TO_RENDER_STALENESS 100.0

    // Try to nack for any missing packets
    bool nacking_succeeded = try_nacking(video_ring_buffer, latency);

    // Get how stale the frame we're trying to render is
    double next_to_render_staleness = -1.0;
    int next_render_id = video_data.last_rendered_id + 1;
    FrameData* ctx = get_frame_at_id(video_ring_buffer, next_render_id);
    if (ctx->id == next_render_id) {
        next_to_render_staleness = get_timer(ctx->frame_creation_timer);
    }

    // If nacking has failed to recover the packets we need,
    if (!nacking_succeeded
        // Or if we're more than MAX_UNSYNCED_FRAMES frames out-of-sync,
        || video_ring_buffer->max_id > video_data.last_rendered_id + MAX_UNSYNCED_FRAMES
        // Or we've spent MAX_TO_RENDER_STALENESS trying to render this one frame
        || (next_to_render_staleness > MAX_TO_RENDER_STALENESS / MS_IN_SECOND)) {
        // THEN, we should send an iframe request

        // Throttle the requests to prevent network upload saturation, however
        if (get_timer(video_data.last_iframe_request_timer) >
            IFRAME_REQUEST_INTERVAL_MS / MS_IN_SECOND) {
            WhistClientMessage wcmsg = {0};
            // Send a video packet stream reset request
            wcmsg.type = MESSAGE_STREAM_RESET_REQUEST;
            wcmsg.stream_reset_data.type = PACKET_VIDEO;
            wcmsg.stream_reset_data.last_failed_id =
                max(next_render_id, video_ring_buffer->max_id - 1);
            send_wcmsg(&wcmsg);
            LOG_INFO(
                "The most recent ID %d is %d frames ahead of the most recently rendered frame, "
                "and the frame we're trying to render has been alive for %fms. "
                "An I-Frame is now being requested to catch-up.",
                video_ring_buffer->max_id, video_ring_buffer->max_id - video_data.last_rendered_id,
                next_to_render_staleness * MS_IN_SECOND);
            start_timer(&video_data.last_iframe_request_timer);
        }
    }
}

int32_t multithreaded_destroy_decoder(void* opaque) {
    /*
        Destroy the video decoder. This will be run in a separate SDL thread, hence the void* opaque
       parameter.

        Arguments:
            opaque (void*): pointer to a video decoder

        Returns:
            (int32_t): 0
    */
    VideoDecoder* decoder = (VideoDecoder*)opaque;
    destroy_video_decoder(decoder);
    return 0;
}

#define STATISTICS_SECONDS 5
void calculate_statistics() {
    /*
        Calculate statistics about bitrate nacking, etc. to request server bandwidth.
    */

    static clock t;
    static bool init_t = false;
    static BitrateStatistics stats;
    static Bitrates new_bitrates;
    if (!init_t) {
        start_timer(&t);
        init_t = true;
    }
    // do some calculation
    // Update mbps every STATISTICS_SECONDS seconds
    if (get_timer(t) > STATISTICS_SECONDS) {
        stats.num_nacks_per_second = video_ring_buffer->num_packets_nacked / STATISTICS_SECONDS;
        stats.num_received_packets_per_second =
            video_ring_buffer->num_packets_received / STATISTICS_SECONDS;
        stats.num_rendered_frames_per_second =
            video_ring_buffer->num_frames_rendered / STATISTICS_SECONDS;

        LOG_METRIC("\"rendered_fps\" : %d", stats.num_rendered_frames_per_second);
        new_bitrates = calculate_new_bitrate(stats);
        if (new_bitrates.bitrate != client_max_bitrate ||
            new_bitrates.burst_bitrate != max_burst_bitrate) {
            client_max_bitrate = max(min(new_bitrates.bitrate, MAXIMUM_BITRATE), MINIMUM_BITRATE);
            max_burst_bitrate = new_bitrates.burst_bitrate;
            update_bitrate = true;
        }
        video_ring_buffer->num_packets_nacked = 0;
        video_ring_buffer->num_packets_received = 0;
        video_ring_buffer->num_frames_rendered = 0;
        start_timer(&t);
    }
}

void skip_to_next_iframe() {
    /*
        Skip to the latest iframe we're received if said iframe is ahead of what we are currently
        rendering.
        TODO: Refactor into ringbuffer.c
    */

    // Only run if we're not rendering or we haven't rendered anything yet
    if (video_data.most_recent_iframe > 0 && video_data.last_rendered_id == -1) {
        // Silently skip to next iframe if it's the start of the stream
        video_data.last_rendered_id = video_data.most_recent_iframe - 1;
    } else if (video_data.most_recent_iframe - 1 > video_data.last_rendered_id) {
        // Loudly declare the skip and log which frames we dropped
        LOG_INFO("Skipping from %d to I-Frame %d", video_data.last_rendered_id,
                 video_data.most_recent_iframe);
        for (int i = max(video_data.last_rendered_id + 1,
                         video_data.most_recent_iframe - video_ring_buffer->frames_received + 1);
             i < video_data.most_recent_iframe; i++) {
            FrameData* frame_data = get_frame_at_id(video_ring_buffer, i);
            if (frame_data->id == i) {
                LOG_WARNING("Frame dropped with ID %d: %d/%d", i,
                            frame_data->original_packets_received,
                            frame_data->num_original_packets);

                for (int j = 0; j < frame_data->num_original_packets; j++) {
                    if (!frame_data->received_indices[j]) {
                        LOG_WARNING("Did not receive ID %d, Index %d", i, j);
                    }
                }
                reset_frame(video_ring_buffer, frame_data);
            } else if (frame_data->id != -1) {
                LOG_WARNING("Bad ID? %d instead of %d", frame_data->id, i);
            }
        }
        video_data.last_rendered_id = video_data.most_recent_iframe - 1;
    }
}
// END VIDEO FUNCTIONS

/*
============================
Public Function Implementations
============================
*/

void init_video() {
    /*
        Initializes the video system
    */

    initialized_video = false;

    // Initialize everything
    memset(&video_data, 0, sizeof(video_data));
    video_ring_buffer = init_ring_buffer(PACKET_VIDEO, RECV_FRAMES_BUFFER_SIZE, nack_packet);
    initialized_video_buffer = true;
    working_mbps = STARTING_BITRATE;
    has_video_rendered_yet = false;
    video_data.sws = NULL;
    client_max_bitrate = STARTING_BITRATE;
    video_data.target_mbps = STARTING_BITRATE;
    video_data.pending_ctx = NULL;
    video_data.frames_received = 0;
    video_data.bytes_transferred = 0;
    start_timer(&video_data.frame_timer);
    video_data.last_statistics_id = 1;
    video_data.last_rendered_id = 0;
    video_data.most_recent_iframe = -1;
    video_data.bucket = STARTING_BITRATE / BITRATE_BUCKET_SIZE;
    start_timer(&video_data.last_iframe_request_timer);

    // Init loading animation variables
    video_data.loading_index = 0;
    start_timer(&video_data.last_loading_frame_timer);
    // Present first frame of loading animation
    sdl_update_framebuffer_loading_screen(video_data.loading_index);
    sdl_render_framebuffer();
    // Then progress the animation
    video_data.loading_index++;

    // Mark as initialized and return
    initialized_video = true;
}

int last_rendered_index = 0;

bool schedule_frame_render(VideoFrame* frame) { return true; }

void update_video() {
    /*
        Calculate statistics about bitrate, I-Frame, etc. and request video
        update from the server
    */

    if (!initialized_video_buffer) {
        return;
    }

    if (get_timer(video_data.frame_timer) > 3) {
        calculate_statistics();
    }

    if (!pushing_render_context) {
        // Try to skip up to the next iframe, if possible
        // This function will log the skip verbosely
        skip_to_next_iframe();

        // Try to render out the next frame, if possible
        if (video_data.last_rendered_id >= 0) {
            int next_render_id = video_data.last_rendered_id + 1;

            // When we receive a packet that is a part of the next_render_id, and we have received
            // every packet for that frame, we set rendering=true
            if (is_ready_to_render(video_ring_buffer, next_render_id)) {
                // The following line invalidates the information stored at the pointer ctx
                render_context = set_rendering(video_ring_buffer, next_render_id);
                // Progress the videodata last rendered pointer
                video_data.last_rendered_id = next_render_id;

                // Render out current_render_id
                pushing_render_context = true;
            }
        }
    }

    // Try to recover missing information, via nacks / iframe requests
    try_recovering_missing_packets_or_frames();
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int32_t receive_video(WhistPacket* packet) {
    /*
        Receive video packet

        Arguments:
            packet (WhistPacket*): Packet received from the server, which gets
                sorted as video packet with proper parameters

        Returns:
            (int32_t): -1 if failed to receive packet into video frame, else 0
    */

    // The next two lines are commented out, but left in the codebase to be
    // easily toggled back and forth during development. We considered putting
    // this under the LOG_VIDEO ifdef, but decided not to, since these lines
    // log every single packet, which is too verbose for standard video
    // logging.
    // LOG_INFO("Video Packet ID %d, Index %d (Packets: %d) (Size: %d)",
    // packet->id, packet->index, packet->num_indices, packet->payload_size);
    // Find frame in ring buffer that matches the id
    video_data.bytes_transferred += packet->payload_size;
    int res = receive_packet(video_ring_buffer, packet);
    if (res < 0) {
        return res;
    } else {
        // If we received all of the packets
        if (is_ready_to_render(video_ring_buffer, packet->id)) {
            FrameData* ctx = get_frame_at_id(video_ring_buffer, packet->id);

            // TODO: Clean this up by moving iframe logic into ringbuffer.c
            VideoFrame* video_frame;
            if (ctx->successful_fec_recovery) {
                video_frame = (VideoFrame*)ctx->fec_frame_buffer;
            } else {
                video_frame = (VideoFrame*)ctx->packet_buffer;
            }
            bool is_iframe = video_frame->is_iframe;

#if LOG_VIDEO
            LOG_INFO("Received Video Frame ID %d (Packets: %d) (Size: %d) %s", ctx->id,
                     ctx->num_packets, sizeof(VideoFrame) + video_frame->videodata_length,
                     is_iframe ? "(I-Frame)" : "");
#endif

            // If it's an I-frame, then just skip right to it, if the id is ahead of
            // the next to render id
            if (is_iframe) {
                video_data.most_recent_iframe = max(video_data.most_recent_iframe, packet->id);
            }
        }
    }

    return 0;
}

int render_video() {
    /*
        Render the video screen that the user sees

        Arguments:
            renderer (SDL_Renderer*): SDL renderer used to generate video

        Return:
            (int): 0 on success, -1 on failure
    */
    if (!initialized_video) {
        LOG_ERROR("Video rendering is not initialized!");
        return -1;
    }

    clock statistics_timer;

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
    if (pushing_render_context) {
        // Save frame data, drop volatile
        frame_data = *(FrameData*)render_context;
        // Grab and consume the actual frame
        VideoFrame* frame = (VideoFrame*)frame_data.frame_buffer;

        // If server thinks the window isn't visible, but the window is visible now,
        // Send a START_STREAMING message
        if (!frame->is_window_visible && sdl_is_window_visible()) {
            // The server thinks the client window is occluded/minimized, but it isn't. So
            // we'll correct it. NOTE: Most of the time, this is just because there was a
            // delay between the window losing visibility and the server reacting.
            WhistClientMessage wcmsg = {0};
            wcmsg.type = MESSAGE_START_STREAMING;
            send_wcmsg(&wcmsg);
        }

        if (!frame->is_empty_frame) {
            sync_decoder_parameters(frame);
            int ret;
            server_timestamp = frame->server_timestamp;
            client_input_timestamp = frame->client_input_timestamp;
            TIME_RUN(ret = video_decoder_send_packets(
                         video_data.decoder, get_frame_videodata(frame), frame->videodata_length),
                     VIDEO_DECODE_SEND_PACKET_TIME, statistics_timer);
            if (ret < 0) {
                LOG_ERROR("Failed to send packets to decoder, unable to render frame");
                pushing_render_context = false;
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
        pushing_render_context = false;
    }

    // Try to keep decoding frames from the decoder, if we can
    // Use static so we can remember, even if sdl_render_pending exits early.
    static bool got_frame_from_decoder = false;
    while (video_data.decoder != NULL) {
        int res;
        TIME_RUN(res = video_decoder_decode_frame(video_data.decoder), VIDEO_DECODE_GET_FRAME_TIME,
                 statistics_timer);
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

        // However, We only skip a render after setting `pushing_render_context = false`,
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
        decoded_frame_data = video_decoder_get_last_decoded_frame(video_data.decoder);

        // Pull the most recently decoded data/linesize from the decoder
        if (decoded_frame_data.using_hw) {
            data[0] = data[1] = decoded_frame_data.decoded_frame->data[3];
            linesize[0] = linesize[1] = decoded_frame_data.width;
        } else {
            // This will allocate the sws target frame into data/linesize,
            // using the provided pixel_format
            update_sws_context(data, linesize, decoded_frame_data.pixel_format);
            if (video_data.sws) {
                // Scale from the swframe into the sws target frame.
                sws_scale(video_data.sws,
                          (uint8_t const* const*)decoded_frame_data.decoded_frame->data,
                          decoded_frame_data.decoded_frame->linesize, 0, decoded_frame_data.height,
                          data, linesize);
            } else {
                memcpy(data, decoded_frame_data.decoded_frame->data, sizeof(data));
                memcpy(linesize, decoded_frame_data.decoded_frame->linesize, sizeof(linesize));
            }
        }

        // Invalidate loading animation once rendering occurs
        video_data.loading_index = -1;

        // Render out the cursor image
        if (has_cursor_image) {
            TIME_RUN(sdl_update_cursor(&cursor_image), VIDEO_CURSOR_UPDATE_TIME, statistics_timer);
        }

        // Update the window titlebar color
        sdl_render_window_titlebar_color(window_color);

        // Render the decoded frame
        sdl_update_framebuffer(data, linesize, video_data.decoder->width,
                               video_data.decoder->height);

        // Mark the framebuffer out to render
        sdl_render_framebuffer();

        // Declare user activity to prevent screensaver
        declare_user_activity();

#if LOG_VIDEO
        LOG_DEBUG("Rendered %d (Size: %d) (Age %f)", frame_data.id, frame_data.frame_buffer_size,
                  get_timer(frame_data.frame_creation_timer));
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

        has_video_rendered_yet = true;

        // Track time between consecutive frames
        static clock last_frame_timer;
        static bool last_frame_timer_started = false;
        if (last_frame_timer_started) {
            log_double_statistic(VIDEO_TIME_BETWEEN_FRAMES,
                                 get_timer(last_frame_timer) * MS_IN_SECOND);
        }
        start_timer(&last_frame_timer);
        last_frame_timer_started = true;
    } else if (video_data.loading_index >= 0) {
        // If we didn't get a frame, and loading_index is valid,
        // Render the loading animation
        const float loading_animation_fps = 20.0;
        if (get_timer(video_data.last_loading_frame_timer) > 1 / loading_animation_fps) {
            // Present the loading screen
            sdl_update_framebuffer_loading_screen(video_data.loading_index);
            sdl_render_framebuffer();
            // Progress animation
            video_data.loading_index++;
            // Reset timer
            start_timer(&video_data.last_loading_frame_timer);
        }
    }

    return 0;
}

void destroy_video() {
    /*
        Free the video thread and VideoContext data to exit
    */

    if (!initialized_video) {
        LOG_WARNING("Destroying video, but never called init_video");
    } else {
        // Destroy the ring buffer
        destroy_ring_buffer(video_ring_buffer);
        video_ring_buffer = NULL;

        if (video_data.decoder) {
            WhistThread destroy_decoder_thread = whist_create_thread(
                multithreaded_destroy_decoder, "multithreaded_destroy_decoder", video_data.decoder);
            whist_detach_thread(destroy_decoder_thread);
            video_data.decoder = NULL;
        }
    }

    // Reset globals
    server_width = -1;
    server_height = -1;
    server_codec_type = CODEC_TYPE_UNKNOWN;

    has_video_rendered_yet = false;
}
