/**
 * Copyright 2021 Whist Technologies, Inc.
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

extern volatile int output_width;
extern volatile int output_height;
extern volatile CodecType output_codec_type;
extern volatile double latency;

volatile WhistRGBColor* native_window_color = NULL;
volatile bool native_window_color_update = false;

// START VIDEO VARIABLES
volatile WhistCursorState cursor_state = CURSOR_STATE_VISIBLE;
volatile SDL_Cursor* sdl_cursor = NULL;
volatile WhistCursorID last_cursor = (WhistCursorID)SDL_SYSTEM_CURSOR_ARROW;
volatile bool initialized_video_renderer = false;
volatile bool initialized_video_buffer = false;

#ifdef __APPLE__
// on macOS, we must initialize the renderer in `init_sdl()` instead of video.c
extern volatile SDL_Renderer* init_sdl_renderer;
#endif

// number of frames ahead we can receive packets for before asking for iframe
#define MAX_UNSYNCED_FRAMES 4
// If we want an iframe, this is how often we keep asking for it
#define IFRAME_REQUEST_INTERVAL_MS (50)

#define BITRATE_BUCKET_SIZE 500000
#define NUMBER_LOADING_FRAMES 50

#define CURSORIMAGE_A 0xff000000
#define CURSORIMAGE_R 0x00ff0000
#define CURSORIMAGE_G 0x0000ff00
#define CURSORIMAGE_B 0x000000ff

// The pixel format expected by SDL_UpdateNVTexture
#define SDL_TEXTURE_PIXEL_FORMAT AV_PIX_FMT_NV12

/*
============================
Custom Types
============================
*/

struct VideoData {
    FrameData* pending_ctx;
    int frames_received;
    int bytes_transferred;
    clock frame_timer;
    int last_statistics_id;
    int last_rendered_id;
    int most_recent_iframe;

    clock last_iframe_request_timer;
    bool is_waiting_for_iframe;

    double target_mbps;
    int bucket;  // = STARTING_BITRATE / BITRATE_BUCKET_SIZE;
    int nack_by_bitrate[MAXIMUM_BITRATE / BITRATE_BUCKET_SIZE + 5];
    double seconds_by_bitrate[MAXIMUM_BITRATE / BITRATE_BUCKET_SIZE + 5];

    // Loading animation data
    int loading_index;
    clock last_loading_frame_timer;
} video_data;

typedef struct SDLVideoContext {
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    struct SwsContext* sws;

    VideoDecoder* decoder;
} SDLVideoContext;
SDLVideoContext video_context;

// mbps that currently works
volatile double working_mbps;

// Context of the frame that is currently being rendered
static volatile VideoFrame* render_context;
static volatile bool pushing_render_context = false;

// True if RenderScreen is currently trying to render something
// This is only used for iframe logic
static volatile bool rendering = false;

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

SDL_Rect new_sdl_rect(int x, int y, int w, int h);
int32_t multithreaded_destroy_decoder(void* opaque);
bool request_iframe();
void finalize_video_context_data();
void replace_texture();
void clear_sdl(SDL_Renderer* renderer);
void calculate_statistics();
void skip_to_next_iframe();
void sync_decoder_parameters(VideoFrame* frame);
// Try to request an iframe, if we need to catch up,
// returns true if we're trying to get an iframe
bool try_request_iframe_to_catch_up();
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
        LOG_INFO("Wants to change resolution, but not an i-frame!");
        return;
    }

    LOG_INFO(
        "Updating client rendering to match server's width and "
        "height and codec! "
        "From %dx%d, codec %d to %dx%d, codec %d",
        server_width, server_height, server_codec_type, frame->width, frame->height,
        frame->codec_type);
    if (video_context.decoder) {
        SDL_Thread* destroy_decoder_thread = SDL_CreateThread(
            multithreaded_destroy_decoder, "multithreaded_destroy_decoder", video_context.decoder);
        SDL_DetachThread(destroy_decoder_thread);
        video_context.decoder = NULL;
    }

    VideoDecoder* decoder =
        create_video_decoder(frame->width, frame->height, USE_HARDWARE, frame->codec_type);
    if (!decoder) {
        LOG_FATAL("ERROR: Decoder could not be created!");
    }
    video_context.decoder = decoder;

    server_width = frame->width;
    server_height = frame->height;
    server_codec_type = frame->codec_type;
    output_codec_type = frame->codec_type;
}

void update_sws_context(Uint8** data, int* linesize) {
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
    */

    VideoDecoder* decoder = video_context.decoder;
    enum AVPixelFormat input_format = decoder->sw_frame->format;
    static enum AVPixelFormat cached_format = AV_PIX_FMT_NONE;
    static int cached_width = -1;
    static int cached_height = -1;

    // Update the cache on a cache miss
    if (input_format != cached_format || decoder->width != cached_width ||
        decoder->height != cached_height) {
        cached_format = input_format;
        cached_width = decoder->width;
        cached_height = decoder->height;

        // No matter what, we now should destroy the old context if it exists
        if (video_context.sws) {
            av_freep(&data[0]);
            sws_freeContext(video_context.sws);
            video_context.sws = NULL;
        }

        if (input_format == SDL_TEXTURE_PIXEL_FORMAT) {
            LOG_INFO("The input format is already correct, no sws needed!");
            // If the pixel format already matches, we require no pixel format conversion,
            // we can just pass directly to the renderer!
        } else {
            // We need to create a new context to handle the pixel fmt conversion
            LOG_INFO("Creating sws context to convert from %s to %s",
                     av_get_pix_fmt_name(input_format),
                     av_get_pix_fmt_name(SDL_TEXTURE_PIXEL_FORMAT));
            video_context.sws = sws_getContext(
                decoder->width, decoder->height, input_format, decoder->width, decoder->height,
                SDL_TEXTURE_PIXEL_FORMAT, SWS_FAST_BILINEAR, NULL, NULL, NULL);
            av_image_alloc(data, linesize, decoder->width, decoder->height,
                           SDL_TEXTURE_PIXEL_FORMAT, 32);
        }
    }
}

bool try_request_iframe_to_catch_up() {
    /*
        Check if we are MAX_UNSYNCED_FRAMES behind on rendering frames,
        and request an iframe if so
    */

    // If we are more than MAX_UNSYNCED_FRAMES behind, we should request an iframe.
    // we should also request if the next to render frame has been sitting for 200ms and we
    // still haven't rendered.
    int next_render_id = video_data.last_rendered_id + 1;
    FrameData* ctx = get_frame_at_id(video_ring_buffer, next_render_id);
    if (video_ring_buffer->max_id > video_data.last_rendered_id + MAX_UNSYNCED_FRAMES ||
        (ctx->id == next_render_id && get_timer(ctx->frame_creation_timer) > 200.0 / MS_IN_SECOND &&
         !video_data.is_waiting_for_iframe)) {
        // old condition, which only checked if we hadn't received any packets in a while:
        // || (cur_ctx->id == VideoData.last_rendered_id && get_timer(
        // cur_ctx->last_packet_timer ) > 96.0 / MS_IN_SECOND) )
        if (request_iframe()) {
            LOG_INFO(
                "The most recent ID %d is %d frames ahead of the most recently rendered frame, "
                "and the next frame to render has been alive for %fms. I-Frame is now being "
                "requested to catch-up.",
                video_ring_buffer->max_id, video_ring_buffer->max_id - video_data.last_rendered_id,
                get_timer(ctx->frame_creation_timer) * MS_IN_SECOND);
        }
        return true;
    }

    // No I-Frame is being requested
    return false;
}

void render_sdl_cursor(WhistCursorImage* cursor) {
    /*
      Update the cursor image on the screen. If the cursor hasn't changed since the last frame we
      received, we don't do anything. Otherwise, we either use the provided bitmap or  update the
      cursor ID to tell SDL which cursor to render.
     */
    // Set cursor to frame's desired cursor type
    // Only update the cursor, if a cursor image is even embedded in the frame at all.
    if (cursor) {
        if ((WhistCursorID)cursor->cursor_id != last_cursor || cursor->using_bmp) {
            if (sdl_cursor) {
                SDL_FreeCursor((SDL_Cursor*)sdl_cursor);
            }
            if (cursor->using_bmp) {
                // use bitmap data to set cursor
                SDL_Surface* cursor_surface = SDL_CreateRGBSurfaceFrom(
                    cursor->bmp, cursor->bmp_width, cursor->bmp_height, sizeof(uint32_t) * 8,
                    sizeof(uint32_t) * cursor->bmp_width, CURSORIMAGE_R, CURSORIMAGE_G,
                    CURSORIMAGE_B, CURSORIMAGE_A);

                // Use BLENDMODE_NONE to allow for proper cursor blit-resize
                SDL_SetSurfaceBlendMode(cursor_surface, SDL_BLENDMODE_NONE);

#ifdef _WIN32
                // on Windows, the cursor DPI is unchanged
                const int cursor_dpi = 96;
#else
                // on other platforms, use the window DPI
                const int cursor_dpi = get_native_window_dpi((SDL_Window*)window);
#endif  // _WIN32

                // Create the scaled cursor surface which takes DPI into account
                SDL_Surface* scaled_cursor_surface = SDL_CreateRGBSurface(
                    0, cursor->bmp_width * 96 / cursor_dpi, cursor->bmp_height * 96 / cursor_dpi,
                    cursor_surface->format->BitsPerPixel, cursor_surface->format->Rmask,
                    cursor_surface->format->Gmask, cursor_surface->format->Bmask,
                    cursor_surface->format->Amask);

                // Copy the original cursor into the scaled surface
                SDL_BlitScaled(cursor_surface, NULL, scaled_cursor_surface, NULL);

                // Potentially SDL_SetSurfaceBlendMode here since X11 cursor BMPs are
                // pre-alpha multplied. Remember to adjust hot_x/y by the DPI scaling.
                sdl_cursor = SDL_CreateColorCursor(scaled_cursor_surface,
                                                   cursor->bmp_hot_x * 96 / cursor_dpi,
                                                   cursor->bmp_hot_y * 96 / cursor_dpi);
                SDL_FreeSurface(cursor_surface);
                SDL_FreeSurface(scaled_cursor_surface);
            } else {
                // use cursor id to set cursor
                sdl_cursor = SDL_CreateSystemCursor((SDL_SystemCursor)cursor->cursor_id);
            }
            SDL_SetCursor((SDL_Cursor*)sdl_cursor);

            last_cursor = (WhistCursorID)cursor->cursor_id;
        }

        if (cursor->cursor_state != cursor_state) {
            if (cursor->cursor_state == CURSOR_STATE_HIDDEN) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            } else {
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }

            cursor_state = cursor->cursor_state;
        }
    }
}

SDL_Rect new_sdl_rect(int x, int y, int w, int h) {
    /*
      Wrapper to create a new SDL rectangle.

      Returns:
      (SDL_Rect): SDL_Rect with the given dimensions
     */
    SDL_Rect new_rect;
    new_rect.x = x;
    new_rect.y = y;
    new_rect.w = w;
    new_rect.h = h;
    return new_rect;
}

void render_window_titlebar_color(WhistRGBColor color) {
    /*
      Update window titlebar color using the colors of the new frame
     */
    WhistRGBColor* current_color = (WhistRGBColor*)native_window_color;
    if (current_color != NULL) {
        if (current_color->red != color.red || current_color->green != color.green ||
            current_color->blue != color.blue) {
            // delete the old color we were using
            free(current_color);

            // make the new color and signal that we're ready to update
            WhistRGBColor* new_native_window_color = safe_malloc(sizeof(WhistRGBColor));
            *new_native_window_color = color;
            native_window_color = new_native_window_color;
            native_window_color_update = true;
        }
    } else {
        // make the new color and signal that we're ready to update
        WhistRGBColor* new_native_window_color = safe_malloc(sizeof(WhistRGBColor));
        *new_native_window_color = color;
        native_window_color = new_native_window_color;
        native_window_color_update = true;
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

void update_decoder_parameters(int width, int height, CodecType codec_type) {
    /*
        Update video decoder parameters

        Arguments:
            width (int): video width
            height (int): video height
            codec_type (CodecType): decoder codec type
    */
}

void render_loading_screen(SDL_Renderer* renderer, int idx) {
    /*
        Make the screen black and show the loading screen
        Arguments:
            renderer (SDL_Renderer*): video renderer
            idx (int): the index of the loading frame
    */

    int gif_frame_index = idx % NUMBER_LOADING_FRAMES;

    clock c;
    start_timer(&c);

    char frame_name[24];
    if (gif_frame_index < 10) {
        snprintf(frame_name, sizeof(frame_name), "loading/frame_0%d.png", gif_frame_index);
        //            LOG_INFO("Frame loading/frame_0%d.png", gif_frame_index);
    } else {
        snprintf(frame_name, sizeof(frame_name), "loading/frame_%d.png", gif_frame_index);
        //            LOG_INFO("Frame loading/frame_%d.png", gif_frame_index);
    }

    SDL_Surface* loading_screen = sdl_surface_from_png_file(frame_name);
    if (loading_screen == NULL) {
        LOG_WARNING("Loading screen not loaded from %s", frame_name);
        return;
    }

    // free pkt.data which is initialized by calloc in png_file_to_bmp

    SDL_Texture* loading_screen_texture = SDL_CreateTextureFromSurface(renderer, loading_screen);

    // surface can now be freed
    free_sdl_rgb_surface(loading_screen);

    int w = 200;
    int h = 200;
    SDL_Rect dstrect;

    // SDL_QueryTexture( loading_screen_texture, NULL, NULL, &w, &h );
    dstrect.x = output_width / 2 - w / 2;
    dstrect.y = output_height / 2 - h / 2;
    dstrect.w = w;
    dstrect.h = h;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, loading_screen_texture, NULL, &dstrect);
    SDL_RenderPresent(renderer);

    // texture may now be destroyed
    SDL_DestroyTexture(loading_screen_texture);

    gif_frame_index += 1;
    gif_frame_index %= NUMBER_LOADING_FRAMES;
}

bool request_iframe() {
    /*
        Request an IFrame from the server if too long since last frame

        Return:
            (bool): true if IFrame requested, false if not
    */

    // Only request an iframe once every `IFRAME_REQUEST_INTERVAL_MS` ms
    if (get_timer(video_data.last_iframe_request_timer) >
        IFRAME_REQUEST_INTERVAL_MS / MS_IN_SECOND) {
        WhistClientMessage wcmsg = {0};
        wcmsg.type = MESSAGE_IFRAME_REQUEST;
        // This should give us a full IDR frame,
        // which includes PPS/SPS data
        wcmsg.reinitialize_encoder = false;
        send_wcmsg(&wcmsg);
        start_timer(&video_data.last_iframe_request_timer);
        video_data.is_waiting_for_iframe = true;
        return true;
    } else {
        return false;
    }
}

void replace_texture() {
    /*
        Destroy the old texture and create a new texture. This function is called during renderer
        initialization and if the user has finished resizing the window.
    */

    // Destroy the old texture
    if (video_context.texture) {
        SDL_DestroyTexture(video_context.texture);
        video_context.texture = NULL;
    }
    // Create a new texture
    SDL_Texture* texture =
        SDL_CreateTexture(video_context.renderer, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING,
                          MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
    if (!texture) {
        LOG_FATAL("SDL: could not create texture - exiting");
    }

    // Save the new texture over the old one
    video_context.texture = texture;
}

void clear_sdl(SDL_Renderer* renderer) {
    /*
        Clear the SDL renderer

        Arguments:
            renderer (SDL_Renderer*): the video renderer
    */

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
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
        stats.num_skipped_frames_per_second =
            video_ring_buffer->num_frames_skipped / STATISTICS_SECONDS;
        stats.num_rendered_frames_per_second =
            video_ring_buffer->num_frames_rendered / STATISTICS_SECONDS;

        LOG_METRIC("\"rendered_fps\" : %d, \"skipped_fps\" : %d",
                   stats.num_rendered_frames_per_second, stats.num_skipped_frames_per_second);
        new_bitrates = calculate_new_bitrate(stats);
        if (new_bitrates.bitrate != client_max_bitrate ||
            new_bitrates.burst_bitrate != max_burst_bitrate) {
            client_max_bitrate = max(min(new_bitrates.bitrate, MAXIMUM_BITRATE), MINIMUM_BITRATE);
            max_burst_bitrate = new_bitrates.burst_bitrate;
            update_bitrate = true;
        }
        video_ring_buffer->num_packets_nacked = 0;
        video_ring_buffer->num_packets_received = 0;
        video_ring_buffer->num_frames_skipped = 0;
        video_ring_buffer->num_frames_rendered = 0;
        start_timer(&t);
    }
}

void skip_to_next_iframe() {
    /*
        Skip to the latest iframe we're received if said iframe is ahead of what we are currently
        rendering.
    */
    // Only run if we're not rendering or we haven't rendered anything yet
    if (video_data.most_recent_iframe > 0 && video_data.last_rendered_id == -1) {
        // Silently skip to next iframe if it's the start of the stream
        video_data.last_rendered_id = video_data.most_recent_iframe - 1;
    } else if (video_data.most_recent_iframe - 1 > video_data.last_rendered_id) {
        // Loudly declare the skip and log which frames we dropped
        LOG_INFO("Skipping from %d to i-frame %d", video_data.last_rendered_id,
                 video_data.most_recent_iframe);
        for (int i = max(video_data.last_rendered_id + 1,
                         video_data.most_recent_iframe - video_ring_buffer->frames_received + 1);
             i < video_data.most_recent_iframe; i++) {
            FrameData* frame_data = get_frame_at_id(video_ring_buffer, i);
            if (frame_data->id == i) {
                LOG_WARNING("Frame dropped with ID %d: %d/%d", i, frame_data->packets_received,
                            frame_data->num_packets);

                for (int j = 0; j < frame_data->num_packets; j++) {
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
    initialized_video_renderer = false;
    memset(&video_context, 0, sizeof(video_context));
    video_ring_buffer = init_ring_buffer(PACKET_VIDEO, RECV_FRAMES_BUFFER_SIZE, nack_packet);
    initialized_video_buffer = true;
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
            FrameData* ctx = get_frame_at_id(video_ring_buffer, next_render_id);

            // When we receive a packet that is a part of the next_render_id, and we have received
            // every packet for that frame, we set rendering=true
            if (ctx->id == next_render_id && ctx->packets_received == ctx->num_packets) {
                // The following line invalidates the information stored at the pointer ctx
                render_context =
                    (VideoFrame*)set_rendering(video_ring_buffer, next_render_id)->frame_buffer;
                // Progress the videodata last rendered pointer
                video_data.last_rendered_id = next_render_id;
                // If server thinks the window isn't visible, but the window is visible now,
                // Send a START_STREAMING message
                if (!render_context->is_window_visible &&
                    !(SDL_GetWindowFlags((SDL_Window*)window) &
                      (SDL_WINDOW_OCCLUDED | SDL_WINDOW_MINIMIZED))) {
                    // The server thinks the client window is occluded/minimized, but it isn't. So
                    // we'll correct it. NOTE: Most of the time, this is just because there was a
                    // delay between the window losing visibility and the server reacting.
                    WhistClientMessage wcmsg = {0};
                    wcmsg.type = MESSAGE_START_STREAMING;
                    send_wcmsg(&wcmsg);
                }

                // Render out current_render_id
                pushing_render_context = true;
            }
        }
    }

    // Try requesting an iframe, if we're too far behind
    try_request_iframe_to_catch_up();

    // Try to nack
    try_nacking(video_ring_buffer, latency);
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
        FrameData* ctx = get_frame_at_id(video_ring_buffer, packet->id);
        // If we received all of the packets
        if (ctx->packets_received == ctx->num_packets) {
            bool is_iframe = ((VideoFrame*)ctx->frame_buffer)->is_iframe;

#if LOG_VIDEO
            LOG_INFO("Received Video Frame ID %d (Packets: %d) (Size: %d) %s", ctx->id,
                     ctx->num_packets, ctx->frame_size, is_iframe ? "(i-frame)" : "");
#endif

            // If it's an I-frame, then just skip right to it, if the id is ahead of
            // the next to render id
            if (is_iframe) {
                video_data.most_recent_iframe = max(video_data.most_recent_iframe, ctx->id);
            }
        }
    }

    return 0;
}

int init_video_renderer() {
    /*
        Initialize the video renderer. Used as a thread function.

        Return:
            (int): 0 on success, -1 on failure
    */

    LOG_INFO("Creating renderer for %dx%d display", output_width, output_height);

    // configure renderer
    if (SDL_GetWindowFlags((SDL_Window*)window) & SDL_WINDOW_OPENGL) {
        // only opengl if windowed mode
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

// SDL guidelines say that renderer functions should be done on the main thread,
//      but our implementation requires that the renderer is made in this thread
//      for non-MacOS
#ifdef __APPLE__
    SDL_Renderer* renderer = (SDL_Renderer*)init_sdl_renderer;
#else
    SDL_Renderer* renderer = init_renderer((SDL_Window*)window);
#endif

    // Show a black screen initially before anything else
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    video_context.renderer = renderer;
    if (!renderer) {
        LOG_WARNING("SDL: could not create renderer - exiting: %s", SDL_GetError());
        return -1;
    }

    // mbps that currently works
    working_mbps = STARTING_BITRATE;
    video_data.is_waiting_for_iframe = false;

    has_video_rendered_yet = false;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    // Allocate a place to put our YUV image on that screen.
    // Rather than allocating a new texture every time the dimensions change, we instead allocate
    // the texture once and render sub-rectangles of it.
    replace_texture();

    video_context.sws = NULL;
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
    render_loading_screen(renderer, video_data.loading_index);
    video_data.loading_index++;

    // Mark as initialized and return
    initialized_video_renderer = true;
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
    if (!initialized_video_renderer) {
        LOG_ERROR("Video rendering is not initialized!");
        return -1;
    }

    clock statistics_timer;

    // Information needed to render a FrameData* to the screen
    WhistRGBColor window_color = {0};
    WhistCursorImage cursor_image = {0};
    bool has_cursor_image = false;

    // Receive and process a render context that's being pushed
    if (pushing_render_context) {
        // Drop volatile
        VideoFrame* frame = (VideoFrame*)render_context;

        if (!frame->is_empty_frame) {
            sync_decoder_parameters(frame);
            int ret;
            TIME_RUN(
                ret = video_decoder_send_packets(video_context.decoder, get_frame_videodata(frame),
                                                 frame->videodata_length),
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

            if (frame->is_iframe) {
                video_data.is_waiting_for_iframe = false;
            }
        }

        // Mark as received
        pushing_render_context = false;
    }

    // Make these static so sws context doesn't need to be updated every time
    static Uint8* data[4];
    static int linesize[4];

    // Try to get a frame from the decoder, if any exist
    bool got_frame_from_decoder = false;
    while (video_context.decoder != NULL) {
        int res;
        TIME_RUN(res = video_decoder_get_frame(video_context.decoder), VIDEO_DECODE_GET_FRAME_TIME,
                 statistics_timer);
        if (res < 0) {
            LOG_ERROR("Error getting frame from decoder!");
            return -1;
        }

        if (res == 0) {
            // Mark that we got at least one frame from the decoder
            got_frame_from_decoder = true;

            // Pull the most recently decoded data/linesize from the decoder
            if (video_context.decoder->hw_frame && video_context.decoder->hw_frame->data[3]) {
                data[0] = data[1] = video_context.decoder->hw_frame->data[3];
                linesize[0] = linesize[1] = video_context.decoder->width;
            } else {
                update_sws_context(data, linesize);
                if (video_context.sws) {
                    sws_scale(video_context.sws,
                              (uint8_t const* const*)video_context.decoder->sw_frame->data,
                              video_context.decoder->sw_frame->linesize, 0,
                              video_context.decoder->height, data, linesize);
                } else {
                    memcpy(data, video_context.decoder->sw_frame->data, sizeof(data));
                    memcpy(linesize, video_context.decoder->sw_frame->linesize, sizeof(linesize));
                }
            }
        } else {
            // Exit once we get EAGAIN
            break;
        }
    }

    // Render any frame we got from the decoder
    if (got_frame_from_decoder) {
        // Invalidate loading animation once rendering occurs
        video_data.loading_index = -1;

        // Render out the cursor image
        if (has_cursor_image) {
            TIME_RUN(render_sdl_cursor(&cursor_image), VIDEO_CURSOR_UPDATE_TIME, statistics_timer);
        }

        // Update the window titlebar color
        render_window_titlebar_color(window_color);

        // The texture object we allocate is larger than the frame, so we only
        // copy the valid section of the frame into the texture.
        SDL_Rect texture_rect =
            new_sdl_rect(0, 0, video_context.decoder->width, video_context.decoder->height);
        int res;
        TIME_RUN(res = SDL_UpdateNVTexture(video_context.texture, &texture_rect, data[0],
                                           linesize[0], data[1], linesize[1]),
                 VIDEO_SDL_WRITE_TIME, statistics_timer);
        if (res < 0) {
            LOG_ERROR("SDL_UpdateNVTexture failed: %s", SDL_GetError());
            return -1;
        }

        // Subsection of texture that should be rendered to screen.
        SDL_Rect output_rect = texture_rect;
        if (texture_rect.w != output_width || texture_rect.h != output_height) {
            // If the texture is smaller than the screen, we need to render it
            // to the screen.
            output_rect = new_sdl_rect(0, 0, output_width, output_height);
        }
        SDL_RenderCopy(video_context.renderer, video_context.texture, &output_rect, NULL);

        // Declare user activity to prevent screensaver
        declare_user_activity();
        // This function call will take up to 16ms if VSYNC is ON, otherwise 0ms
        TIME_RUN(SDL_RenderPresent(video_context.renderer), VIDEO_RENDER_TIME, statistics_timer);

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
            render_loading_screen(video_context.renderer, video_data.loading_index);
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

    if (!initialized_video_renderer) {
        LOG_WARNING("Destroying video, but never called init_video_renderer");
    } else {
#ifdef __APPLE__
        // On __APPLE__, video_context.renderer is maintained in init_sdl_renderer
        if (video_context.texture) {
            SDL_DestroyTexture(video_context.texture);
            video_context.texture = NULL;
        }
#else
        // SDL_DestroyTexture(video_context.texture); is not needed,
        // the renderer destroys it
        SDL_DestroyRenderer((SDL_Renderer*)video_context.renderer);
        video_context.renderer = NULL;
        video_context.texture = NULL;
#endif

        if (native_window_color) {
            free((WhistRGBColor*)native_window_color);
            native_window_color = NULL;
        }

        // Destroy the ring buffer
        destroy_ring_buffer(video_ring_buffer);
        video_ring_buffer = NULL;

        if (video_context.decoder) {
            SDL_Thread* destroy_decoder_thread =
                SDL_CreateThread(multithreaded_destroy_decoder, "multithreaded_destroy_decoder",
                                 video_context.decoder);
            SDL_DetachThread(destroy_decoder_thread);
            video_context.decoder = NULL;
        }
    }

    // Reset globals
    server_width = -1;
    server_height = -1;
    server_codec_type = CODEC_TYPE_UNKNOWN;

    has_video_rendered_yet = false;
}

void trigger_video_resize() {
    /*
        Set the global variable 'resizing' to true if the SDL window is
        being resized, else false

        Arguments:
            is_resizing (bool): Boolean indicating whether or not the SDL
                window is being resized
    */

    int new_width = get_window_pixel_width((SDL_Window*)window);
    int new_height = get_window_pixel_height((SDL_Window*)window);
    if (new_width != output_width || new_height != output_height) {
        output_width = new_width;
        output_height = new_height;
    }
}
