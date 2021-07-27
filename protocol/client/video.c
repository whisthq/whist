/**
 * Copyright Fractal Computers, Inc. 2020
 * @file video.c
 * @brief This file contains all code that interacts directly with receiving and
 *        processing video packets on the client.
============================
Usage
============================

initVideo() gets called before any video packet can be received. The video
packets are received as standard FractalPackets by ReceiveVideo(FractalPacket*
packet), before being saved in a proper video frame format.
*/

/*
============================
Includes
============================
*/

#include "video.h"
#include "sdl_utils.h"
#include "ringbuffer.h"

#include <stdio.h>

#include <SDL2/SDL.h>
#include <fractal/utils/color.h>
#include <fractal/utils/png.h>
#include <fractal/logging/log_statistic.h>
#include "peercursor.h"
#include "sdlscreeninfo.h"
#include "native_window_utils.h"
#include "network.h"

#define USE_HARDWARE true
#define NO_NACKS_DURING_IFRAME false

// Global Variables
extern volatile SDL_Window* window;

extern volatile int server_width;
extern volatile int server_height;
extern volatile CodecType server_codec_type;

extern int client_id;

// Keeping track of max mbps
extern volatile int max_bitrate;
extern volatile bool update_mbps;

extern volatile int output_width;
extern volatile int output_height;
extern volatile CodecType output_codec_type;
extern volatile double latency;

extern volatile FractalRGBColor* native_window_color;
extern volatile bool native_window_color_update;

// START VIDEO VARIABLES
volatile FractalCursorState cursor_state = CURSOR_STATE_VISIBLE;
volatile SDL_Cursor* sdl_cursor = NULL;
volatile FractalCursorID last_cursor = (FractalCursorID)SDL_SYSTEM_CURSOR_ARROW;
volatile bool pending_sws_update = false;
volatile bool pending_texture_update = false;
volatile bool pending_resize_render = false;
volatile bool initialized_video_renderer = false;

static enum AVPixelFormat sws_input_fmt;

#ifdef __APPLE__
// on macOS, we must initialize the renderer in `init_sdl()` instead of video.c
extern volatile SDL_Renderer* init_sdl_renderer;
#endif

// number of frames ahead we can receive packets for before asking for iframe
#define MAX_UNSYNCED_FRAMES 4
#define MAX_UNSYNCED_FRAMES_RENDER 6
// control whether we ask for iframes on missing too many packets - turned off for now
#define REQUEST_IFRAME_ON_MISSING_PACKETS false

#define BITRATE_BUCKET_SIZE 500000
#define NUMBER_LOADING_FRAMES 50

#define CURSORIMAGE_A 0xff000000
#define CURSORIMAGE_R 0x00ff0000
#define CURSORIMAGE_G 0x0000ff00
#define CURSORIMAGE_B 0x000000ff

// We only allow 1 nack in each update_audio call because we had too many false nacks in the past.
// Increase this as our nacking becomes more accurate.
#define MAX_NACKED 1

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
    clock missing_frame_nack_timer;
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

    Uint8* data[4];
    int linesize[4];

    VideoDecoder* decoder;
    struct SwsContext* sws;
} SDLVideoContext;
SDLVideoContext video_context;

// mbps that currently works
volatile double working_mbps;

// Context of the frame that is currently being rendered
static volatile FrameData render_context;

// True if RenderScreen is currently rendering a frame
static volatile bool rendering = false;
volatile bool skip_render = false;
volatile bool can_render;

SDL_mutex* render_mutex;

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

SDL_Rect get_true_render_rect();
void update_cursor(VideoFrame* frame);
SDL_Rect new_sdl_rect(int x, int y, int w, int h);
int32_t multithreaded_destroy_decoder(void* opaque);
void update_decoder_parameters(int width, int height, CodecType codec_type);
void loading_sdl(SDL_Renderer* renderer, int loading_index);
void log_frame_statistics(VideoFrame* frame);
bool request_iframe();
void update_sws_context();
void update_sws_pixel_format();
void finalize_video_context_data();
void replace_texture();
static int render_peers(SDL_Renderer* renderer, PeerUpdateMessage* msgs, size_t num_msgs);
void clear_sdl(SDL_Renderer* renderer);
void calculate_statistics();
void skip_to_next_iframe();
void sync_decoder_parameters(VideoFrame* frame);
void request_iframe_to_catch_up();
#if CAN_UPDATE_WINDOW_TITLEBAR_COLOR
void update_window_titlebar_color();
#endif
/*
============================
Private Function Implementations
============================
*/

void sync_decoder_parameters(VideoFrame* frame) {
    /*
        Update decoder parameters to match the server's width, height, and codec if the next frame
       is an iframe.

        Arguments:
            frame (VideoFrame*): next frame to render
    */
    if (frame->width != server_width || frame->height != server_height ||
        frame->codec_type != server_codec_type) {
        if (frame->is_iframe) {
            LOG_INFO(
                "Updating client rendering to match server's width and "
                "height and codec! "
                "From %dx%d, codec %d to %dx%d, codec %d",
                server_width, server_height, server_codec_type, frame->width, frame->height,
                frame->codec_type);
            update_decoder_parameters(frame->width, frame->height, frame->codec_type);
        } else {
            LOG_INFO("Wants to change resolution, but not an i-frame!");
        }
    }
}

SDL_Rect get_true_render_rect() {
    /*
        Since RenderCopy scales the texture to the size of the window by default, we use this to
        truncate the frame to the size of the window to avoid scaling artifacts (blurriness). The
        frame may be larger than the window because the video encoder rounds the width up to a
        multiple of 8, and the height to a multiple of 2.

        Returns:
            (SDL_Rect): the subsection of the texture we should actually render to the screen
     */
    int render_width, render_height;
    if (output_width <= server_width && server_width <= output_width + 8 &&
        output_height <= server_height && server_height <= output_height + 2) {
        render_width = output_width;
        render_height = output_height;
        if (server_width > output_width || server_height > output_height) {
            // We failed to force the window dimensions to be multiples of 8, 2 in
            // `handle_window_size_changed`
            static bool already_sent_message = false;
            static long long last_server_dims = -1;
            static long long last_output_dims = -1;
            if (server_width * 100000LL + server_height != last_server_dims ||
                output_width * 100000LL + output_height != last_output_dims) {
                // If truncation to/from dimensions have changed, then we should resend
                // Truncating message
                already_sent_message = false;
            }
            last_server_dims = server_width * 100000LL + server_height;
            last_output_dims = output_width * 100000LL + output_height;
            if (!already_sent_message) {
                LOG_WARNING("Truncating window from %dx%d to %dx%d", server_width, server_height,
                            output_width, output_height);
                already_sent_message = true;
            }
        }
    } else {
        // If the condition is false, most likely that means the server has not yet updated
        // to use the new dimensions, so we render the entire frame. This makes resizing
        // look more consistent.
        render_width = server_width;
        render_height = server_height;
    }
    return new_sdl_rect(0, 0, render_width, render_height);
}

void request_iframe_to_catch_up() {
    /*
        Check if we are too behind on rendering frames, measured as:
            - MAX_UNSYNCED_FRAMES behind the latest frame received if not currently rendering
            - MAX_UNSYNCED_FRAMES_RENDER behind the latest frame received if rendering
    */
    if (!rendering) {
        // If we are more than MAX_UNSYNCED_FRAMES behind, we should request an iframe.

        if (video_ring_buffer->max_id > video_data.last_rendered_id + MAX_UNSYNCED_FRAMES) {
            // old condition, which only checked if we hadn't received any packets in a while:
            // || (cur_ctx->id == VideoData.last_rendered_id && get_timer(
            // cur_ctx->last_packet_timer ) > 96.0 / 1000.0) )
            if (request_iframe()) {
                LOG_INFO(
                    "The most recent ID is %d frames ahead of the most recent rendered frame, "
                    "and there is no available frame to render. I-Frame is now being requested "
                    "to catch-up.",
                    MAX_UNSYNCED_FRAMES);
            }
        } else {
#if REQUEST_IFRAME_ON_MISSING_PACKETS
#define MAX_MISSING_PACKETS 50
            // We should also request an iframe if we are missing a lot of packets in case
            // frames are large.
            int missing_packets = 0;
            for (int i = video_data.last_rendered_id + 1;
                 i < video_data.last_rendered_id + MAX_UNSYNCED_FRAMES; i++) {
                FrameData* ctx = get_frame_at_id(video_ring_buffer, i);
                if (ctx->id == i) {
                    for (int j = 0; j < ctx->num_packets; j++) {
                        if (!ctx->received_indices[j]) {
                            missing_packets++;
                        }
                    }
                }
            }
            if (missing_packets > MAX_MISSING_PACKETS) {
                if (request_iframe()) {
                    LOG_INFO(
                        "Missing %d packets in the %d frames ahead of the most recently "
                        "rendered frame,"
                        "and there is no available frame to render. I-Frame is now being "
                        "requested "
                        "to catch-up.",
                        MAX_MISSING_PACKETS, MAX_UNSYNCED_FRAMES);
                }
            }
#endif
        }
    } else {
        // If we're rendering, we might catch up in a bit, so we can be more lenient
        // and will only i-frame if we're MAX_UNSYNCED_FRAMES_RENDER frames behind.
        if (video_ring_buffer->max_id > video_data.last_rendered_id + MAX_UNSYNCED_FRAMES_RENDER) {
            if (request_iframe()) {
                LOG_INFO(
                    "The most recent ID is %d frames ahead of the most recent rendered frame. "
                    "I-Frame is now being requested to catch-up.",
                    MAX_UNSYNCED_FRAMES_RENDER);
            }
        }
    }
}

void update_cursor(VideoFrame* frame) {
    /*
      Update the cursor image on the screen. If the cursor hasn't changed since the last frame we
      received, we don't do anything. Otherwise, we either use the provided bitmap or  update the
      cursor ID to tell SDL which cursor to render.
     */
    // Set cursor to frame's desired cursor type
    FractalCursorImage* cursor = get_frame_cursor_image(frame);
    // Only update the cursor, if a cursor image is even embedded in the frame at all.
    if (cursor) {
        if ((FractalCursorID)cursor->cursor_id != last_cursor || cursor->using_bmp) {
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

                int dpi = get_native_window_dpi((SDL_Window*)window);

                // Create the scaled cursor surface which takes DPI into account
                SDL_Surface* scaled_cursor_surface = SDL_CreateRGBSurface(
                    0, cursor->bmp_width * 96 / dpi, cursor->bmp_height * 96 / dpi,
                    cursor_surface->format->BitsPerPixel, cursor_surface->format->Rmask,
                    cursor_surface->format->Gmask, cursor_surface->format->Bmask,
                    cursor_surface->format->Amask);

                // Copy the original cursor into the scaled surface
                SDL_BlitScaled(cursor_surface, NULL, scaled_cursor_surface, NULL);

                // Potentially SDL_SetSurfaceBlendMode here since X11 cursor BMPs are
                // pre-alpha multplied. Remember to adjust hot_x/y by the DPI scaling.
                sdl_cursor =
                    SDL_CreateColorCursor(scaled_cursor_surface, cursor->bmp_hot_x * 96 / dpi,
                                          cursor->bmp_hot_y * 96 / dpi);
                SDL_FreeSurface(cursor_surface);
                SDL_FreeSurface(scaled_cursor_surface);
            } else {
                // use cursor id to set cursor
                sdl_cursor = SDL_CreateSystemCursor((SDL_SystemCursor)cursor->cursor_id);
            }
            SDL_SetCursor((SDL_Cursor*)sdl_cursor);

            last_cursor = (FractalCursorID)cursor->cursor_id;
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

void update_window_titlebar_color() {
    /*
      Update window titlebar color using the colors of the new frame
     */
    FractalYUVColor new_yuv_color = {video_context.data[0][0], video_context.data[1][0],
                                     video_context.data[2][0]};

    FractalRGBColor new_rgb_color = yuv_to_rgb(new_yuv_color);

    // delete the old color we were using
    if ((FractalRGBColor*)native_window_color != NULL) {
        FractalRGBColor* old_native_window_color = (FractalRGBColor*)native_window_color;
        free(old_native_window_color);
    }

    // make the new color and signal that we're ready to update
    FractalRGBColor* new_native_window_color = safe_malloc(sizeof(FractalRGBColor));
    *new_native_window_color = new_rgb_color;
    native_window_color = new_native_window_color;
    native_window_color_update = true;
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

    LOG_INFO("Updating Width & Height to %dx%d and Codec to %d", width, height, codec_type);

    if (video_context.decoder) {
        SDL_CreateThread(multithreaded_destroy_decoder, "multithreaded_destroy_decoder",
                         video_context.decoder);
    }

    VideoDecoder* decoder = create_video_decoder(width, height, USE_HARDWARE, codec_type);
    if (!decoder) {
        LOG_FATAL("ERROR: Decoder could not be created!");
    }
    video_context.decoder = decoder;
    pending_sws_update = true;
    sws_input_fmt = AV_PIX_FMT_NONE;

    server_width = width;
    server_height = height;
    server_codec_type = codec_type;
    output_codec_type = codec_type;
}

void loading_sdl(SDL_Renderer* renderer, int loading_index) {
    /*
        Make the screen black and show the loading screen
        Arguments:
            renderer (SDL_Renderer*): video renderer
            loading_index (int): the index of the loading frame
    */

    int gif_frame_index = loading_index % NUMBER_LOADING_FRAMES;

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

    if (get_timer(video_data.last_iframe_request_timer) > 1500.0 / 1000.0) {
        FractalClientMessage fmsg = {0};
        fmsg.type = MESSAGE_IFRAME_REQUEST;
        // This should give us a full IDR frame,
        // which includes PPS/SPS data
        fmsg.reinitialize_encoder = false;
        send_fmsg(&fmsg);
        start_timer(&video_data.last_iframe_request_timer);
        video_data.is_waiting_for_iframe = true;
        return true;
    } else {
        return false;
    }
}

void update_sws_context() {
    /*
        Update the SWS context for the decoded video
    */

    LOG_INFO("Updating SWS Context");
    VideoDecoder* decoder = video_context.decoder;

    sws_input_fmt = decoder->sw_frame->format;

    LOG_INFO("Decoder Format: %s", av_get_pix_fmt_name(sws_input_fmt));

    if (video_context.sws) {
        av_freep(&video_context.data[0]);
        sws_freeContext(video_context.sws);
    }

    video_context.sws = NULL;

    memset(video_context.data, 0, sizeof(video_context.data));

    // Rather than scaling the video frame data to the size of the window, we keep its original
    // dimensions so we can truncate it later.
    av_image_alloc(video_context.data, video_context.linesize, decoder->width, decoder->height,
                   AV_PIX_FMT_YUV420P, 32);
    LOG_INFO("Will be converting pixel format from %s to %s", av_get_pix_fmt_name(sws_input_fmt),
             av_get_pix_fmt_name(AV_PIX_FMT_YUV420P));
    video_context.sws =
        sws_getContext(decoder->width, decoder->height, sws_input_fmt, decoder->width,
                       decoder->height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
}

void update_sws_pixel_format() {
    /*
        Update the pixel format for the SWS context if the decoder format and our render format
       (yuv420p) differ.
    */

    if (sws_input_fmt != video_context.decoder->sw_frame->format || pending_sws_update) {
        sws_input_fmt = video_context.decoder->sw_frame->format;
        pending_sws_update = false;
        update_sws_context();
    }
}

void finalize_video_context_data() {
    /*
        Update the pixel format if necessary by applying sws_scale; otherwise, just memcpy the
       software frame into videocontext.data for processing. At the end of this function, the
       ready-to-render frame is in videocontext.data.
    */
    // if we need to change pixel formats, use sws_scale to do so
    update_sws_pixel_format();
    if (video_context.sws) {
        sws_scale(video_context.sws, (uint8_t const* const*)video_context.decoder->sw_frame->data,
                  video_context.decoder->sw_frame->linesize, 0, video_context.decoder->height,
                  video_context.data, video_context.linesize);
    } else {
        // we can directly copy the data
        memcpy(video_context.data, video_context.decoder->sw_frame->data,
               sizeof(video_context.data));
        memcpy(video_context.linesize, video_context.decoder->sw_frame->linesize,
               sizeof(video_context.linesize));
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
        SDL_CreateTexture(video_context.renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
                          MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
    if (!texture) {
        LOG_FATAL("SDL: could not create texture - exiting");
    }

    // Save the new texture over the old one
    video_context.texture = texture;
}

static int render_peers(SDL_Renderer* renderer, PeerUpdateMessage* msgs, size_t num_msgs) {
    /*
        Render peer cursors for multiclient

        Arguments:
            renderer (SDL_Renderer*): the video renderer
            msgs (PeerUpdateMessage*): array of peer update message packets
            num_msgs (size_t): how many peer update messages there are in `msgs`

        Return:
            (int): 0 on success, -1 on failure
    */

    int ret = 0;

    int window_width, window_height;
    SDL_GetWindowSize((SDL_Window*)window, &window_width, &window_height);
    int x = msgs->x * window_width / (int32_t)MOUSE_SCALING_FACTOR;
    int y = msgs->y * window_height / (int32_t)MOUSE_SCALING_FACTOR;

    for (; num_msgs > 0; msgs++, num_msgs--) {
        if (client_id == msgs->peer_id) {
            continue;
        }
        if (draw_peer_cursor(renderer, x, y, msgs->color.red, msgs->color.green,
                             msgs->color.blue) != 0) {
            LOG_ERROR("Failed to draw spectator cursor.");
            ret = -1;
        }
    }
    return ret;
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

void calculate_statistics() {
    /*
        Calculate statistics about bitrate nacking, etc. to request server bandwidth.
    */

    // Get statistics from the last 3 seconds of data
    double time = get_timer(video_data.frame_timer);

    // Calculate statistics
    /*
    int expected_frames = VideoData.max_id - VideoData.last_statistics_id;
    // double fps = 1.0 * expected_frames / time; // TODO: finish birate
    // throttling alg double mbps = VideoData.bytes_transferred * 8.0 /
    // 1024.0 / 1024.0 / time; // TODO bitrate throttle
    double receive_rate =
        expected_frames == 0
            ? 1.0
            : 1.0 * VideoData.frames_received / expected_frames;
    double dropped_rate = 1.0 - receive_rate;
    */

    double nack_per_second = video_ring_buffer->num_nacked / time;
    video_data.nack_by_bitrate[video_data.bucket] += video_ring_buffer->num_nacked;
    video_data.seconds_by_bitrate[video_data.bucket] += time;

    LOG_INFO("====\nBucket: %d\nSeconds: %f\nNacks/Second: %f\n====",
             video_data.bucket * BITRATE_BUCKET_SIZE, time, nack_per_second);

    // Print statistics

    // LOG_INFO("FPS: %f\nmbps: %f\ndropped: %f%%\n", fps, mbps, 100.0 *
    // dropped_rate);

    LOG_INFO("MBPS: %f %f", video_data.target_mbps, nack_per_second);

    // Adjust mbps based on dropped packets
    if (nack_per_second > 50) {
        video_data.target_mbps = video_data.target_mbps * 0.75;
        working_mbps = video_data.target_mbps;
        update_mbps = true;
    } else if (nack_per_second > 25) {
        video_data.target_mbps = video_data.target_mbps * 0.83;
        working_mbps = video_data.target_mbps;
        update_mbps = true;
    } else if (nack_per_second > 15) {
        video_data.target_mbps = video_data.target_mbps * 0.9;
        working_mbps = video_data.target_mbps;
        update_mbps = true;
    } else if (nack_per_second > 10) {
        video_data.target_mbps = video_data.target_mbps * 0.95;
        working_mbps = video_data.target_mbps;
        update_mbps = true;
    } else if (nack_per_second > 6) {
        video_data.target_mbps = video_data.target_mbps * 0.98;
        working_mbps = video_data.target_mbps;
        update_mbps = true;
    } else {
        working_mbps = max(video_data.target_mbps * 1.05, working_mbps);
        video_data.target_mbps = (video_data.target_mbps + working_mbps) / 2.0;
        video_data.target_mbps = min(video_data.target_mbps, MAXIMUM_BITRATE);
        update_mbps = true;
    }

    LOG_INFO("MBPS2: %f", video_data.target_mbps);

    video_data.bucket = (int)video_data.target_mbps / BITRATE_BUCKET_SIZE;
    max_bitrate = (int)video_data.bucket * BITRATE_BUCKET_SIZE + BITRATE_BUCKET_SIZE / 2;

    LOG_INFO("MBPS3: %d", max_bitrate);
    video_ring_buffer->num_nacked = 0;

    video_data.bytes_transferred = 0;
    video_data.frames_received = 0;
    video_data.last_statistics_id = video_ring_buffer->max_id;
    start_timer(&video_data.frame_timer);
}

void skip_to_next_iframe() {
    /*
        Skip to the latest iframe we're received if said iframe is ahead of what we are currently
        rendering.
    */
    // only run if we're not rendering or we haven't rendered anything yet
    if (video_data.last_rendered_id == -1 || !rendering) {
        if (video_data.most_recent_iframe > 0 && video_data.last_rendered_id == -1) {
            video_data.last_rendered_id = video_data.most_recent_iframe - 1;
        } else if (video_data.most_recent_iframe - 1 > video_data.last_rendered_id) {
            LOG_INFO("Skipping from %d to i-frame %d", video_data.last_rendered_id,
                     video_data.most_recent_iframe);
            for (int i =
                     max(video_data.last_rendered_id + 1,
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
                } else {
                    LOG_WARNING("Bad ID? %d instead of %d", frame_data->id, i);
                }
            }
            video_data.last_rendered_id = video_data.most_recent_iframe - 1;
        }
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
    render_mutex = safe_SDL_CreateMutex();
}

int last_rendered_index = 0;

void update_video() {
    /*
        Calculate statistics about bitrate, I-Frame, etc. and request video
        update from the server
    */

    if (get_timer(video_data.frame_timer) > 3) {
        calculate_statistics();
    }

    skip_to_next_iframe();

    if (!rendering && video_data.last_rendered_id >= 0) {
        int next_render_id = video_data.last_rendered_id + 1;
        FrameData* ctx = get_frame_at_id(video_ring_buffer, next_render_id);

        // When we receive a packet that is a part of the next_render_id, and we have received every
        // packet for that frame, we set rendering=true
        if (ctx->id == next_render_id) {
            if (ctx->packets_received == ctx->num_packets) {
                // Now render_context will own the frame_buffer memory block
                render_context = *ctx;
                // Tell the ring buffer we're rendering this frame
                set_rendering(video_ring_buffer, ctx->id);
                // Get the FrameData for the next frame
                int next_frame_render_id = next_render_id + 1;
                FrameData* next_frame_ctx =
                    get_frame_at_id(video_ring_buffer, next_frame_render_id);

                // If the next frame has been received, let's skip the rendering so we can render
                // the next frame faster. We do this because rendering is synced with screen
                // refresh, so rendering the backlogged frames requires the client to wait until the
                // screen refreshes N more times, causing it to fall behind the server.
                if (next_frame_ctx->id == next_frame_render_id &&
                    next_frame_ctx->packets_received == next_frame_ctx->num_packets) {
                    skip_render = true;
                    LOG_INFO("Skip this render");
                } else {
                    skip_render = false;
                }
                rendering = true;
            } else {
                if (get_timer(ctx->last_packet_timer) > latency &&
                    get_timer(ctx->last_nacked_timer) > latency + latency * ctx->num_times_nacked) {
                    if (ctx->num_times_nacked == -1) {
                        ctx->num_times_nacked = 0;
                        ctx->last_nacked_index = -1;
                    }
                    int num_nacked = 0;
                    for (int i = ctx->last_nacked_index + 1;
                         i < ctx->num_packets && num_nacked < MAX_NACKED; i++) {
                        if (!ctx->received_indices[i]) {
                            num_nacked++;
                            LOG_INFO(
                                "************NACKING VIDEO PACKET %d %d (/%d), "
                                "alive for %f MS",
                                ctx->id, i, ctx->num_packets,
                                get_timer(ctx->frame_creation_timer) * MS_IN_SECOND);
                            ctx->nacked_indices[i] = true;
                            nack_packet(video_ring_buffer, ctx->id, i);
                        }
                        ctx->last_nacked_index = i;
                    }
                    if (ctx->last_nacked_index == ctx->num_packets - 1) {
                        ctx->last_nacked_index = -1;
                        ctx->num_times_nacked++;
                    }
                    start_timer(&ctx->last_nacked_timer);
                }
            }
        }
    }
    request_iframe_to_catch_up();
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int32_t receive_video(FractalPacket* packet) {
    /*
        Receive video packet

        Arguments:
            packet (FractalPacket*): Packet received from the server, which gets
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

    if (init_peer_cursors() != 0) {
        LOG_WARNING("Failed to init peer cursors.");
    }

    can_render = true;

    LOG_INFO("Creating renderer for %dx%d display", output_width, output_height);

    // configure renderer
    if (SDL_GetWindowFlags((SDL_Window*)window) & SDL_WINDOW_OPENGL) {
        // only opengl if windowed mode
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

// SDL guidelines say that renderer functions should be done on the main thread,
//      but our implementation requires that the renderer is made in this thread
//      for non-MacOS
#ifdef __APPLE__
    SDL_Renderer* renderer = (SDL_Renderer*)init_sdl_renderer;
#else
    SDL_Renderer* renderer = SDL_CreateRenderer(
        (SDL_Window*)window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
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

    // True if RenderScreen is currently rendering a frame
    rendering = false;
    has_video_rendered_yet = false;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    // Allocate a place to put our YUV image on that screen.
    // Rather than allocating a new texture every time the dimensions change, we instead allocate
    // the texture once and render sub-rectangles of it.
    replace_texture();
    pending_texture_update = false;

    pending_sws_update = false;
    sws_input_fmt = AV_PIX_FMT_NONE;
    video_context.sws = NULL;
    video_ring_buffer = init_ring_buffer(FRAME_VIDEO, RECV_FRAMES_BUFFER_SIZE);

    max_bitrate = STARTING_BITRATE;
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

    // Resize event handling
    SDL_AddEventWatch(resizing_event_watcher, (SDL_Window*)window);

    // Init loading animation variables
    video_data.loading_index = 0;
    start_timer(&video_data.last_loading_frame_timer);
    // Present first frame of loading animation
    loading_sdl(renderer, video_data.loading_index);
    video_data.loading_index++;

    // Mark as initialized and return
    initialized_video_renderer = true;
    return 0;
}

void log_frame_statistics(VideoFrame* frame) {
    /*
        Log statistics of the currently rendering frame. If logging is verbose (i.e. LOG_VIDEO is
        set to true), then we log every single render. If not, we only indicate when frames are
        rendered more than 25ms after we've received them.

        Arguments:
            frame (VideoFrame*): frame we are rendering and logging about
    */
    // log how long it took us to begin rendering the frame from when we received its first
    // packet
    if (get_timer(render_context.frame_creation_timer) > 25.0 / 1000.0) {
        LOG_INFO("Late! Rendering ID %d (Age %f) (Packets %d) %s", render_context.id,
                 get_timer(render_context.frame_creation_timer), render_context.num_packets,
                 frame->is_iframe ? "(I-Frame)" : "");
    }
#if LOG_VIDEO
    else {
        LOG_INFO("Rendering ID %d (Age %f) (Packets %d) %s", render_context.id,
                 get_timer(render_context.frame_creation_timer), render_context.num_packets,
                 frame->is_iframe ? "(I-Frame)" : "");
    }
#endif

    if ((int)(get_total_frame_size(frame)) != render_context.frame_size) {
        LOG_INFO("Incorrect Frame Size! %d instead of %d", get_total_frame_size(frame),
                 render_context.frame_size);
    }

    static clock time_since_last_frame;
    static bool initialized_fps_logging = false;
    if (!initialized_fps_logging) {
        initialized_fps_logging = true;
        start_timer(&time_since_last_frame);
    }
    log_double_statistic("FPS", 1.0 / get_timer(time_since_last_frame));
    start_timer(&time_since_last_frame);
}

int render_video() {
    /*
        Render the video screen that the user sees

        Arguments:
            renderer (SDL_Renderer*): SDL renderer used to generate video

        Return:
            (int): 0 on success, -1 on failure
    */
    static clock latency_clock;

    if (!initialized_video_renderer) {
        LOG_ERROR(
            "Called render_video, but init_video_renderer not called yet! (Or has since been "
            "destroyed");
        return -1;
    }

    // Cast to VideoFrame* because this variable is not volatile in this section
    VideoFrame* frame = (VideoFrame*)render_context.frame_buffer;

    // If the frame hasn't changed since the last one, the server will just send an empty frame to
    // keep the framerate at least at MIN_FPS. We don't want to render this empty frame though.
    if (rendering && frame->is_empty_frame) {
        // We pretend we just rendered this frame. If we don't do this we'll keep assuming that
        // we're behind on frames and start requesting a bunch of iframes, which forces a render.
        video_data.last_rendered_id = render_context.id;
        rendering = false;
        return -1;
    }

    SDL_Renderer* renderer = video_context.renderer;

    if (rendering) {
        // Stop loading animation once rendering occurs
        video_data.loading_index = -1;

        safe_SDL_LockMutex(render_mutex);
        if (pending_resize_render) {
            // User is in the middle of resizing the window
            // retain server width/height if user is resizing the window
            SDL_Rect output_rect = new_sdl_rect(0, 0, server_width, server_height);
            SDL_RenderCopy(renderer, video_context.texture, &output_rect, NULL);
            SDL_RenderPresent(renderer);
        }
        safe_SDL_UnlockMutex(render_mutex);

        PeerUpdateMessage* peer_update_msgs = get_frame_peer_messages(frame);
        size_t num_peer_update_msgs = frame->num_peer_update_msgs;

        log_frame_statistics(frame);

        sync_decoder_parameters(frame);

        start_timer(&latency_clock);
        if (video_decoder_send_packets(video_context.decoder, get_frame_videodata(frame),
                                       frame->videodata_length) < 0) {
            LOG_WARNING("Failed to send packets to the decoder!");
            rendering = false;
            return -1;
        }
        int res;
        log_double_statistic("video_decoder_send_packets time in ms",
                             get_timer(latency_clock) * 1000);

        start_timer(&latency_clock);
        // we should only expect this while loop to run once.
        while ((res = video_decoder_get_frame(video_context.decoder)) == 0) {
            if (res < 0) {
                LOG_ERROR("Failed to get next frame from decoder!");
                rendering = false;
                return -1;
            }
            log_double_statistic("video_decoder_get_frame time in ms",
                                 get_timer(latency_clock) * 1000);

            safe_SDL_LockMutex(render_mutex);

            start_timer(&latency_clock);
            update_cursor(frame);
            log_double_statistic("Cursor update time in ms", get_timer(latency_clock) * 1000);

            // determine if we should be rendering at all or not
            bool render_this_frame = can_render && !skip_render;
            if (render_this_frame) {
                // begin rendering the decoded frame

                clock sws_timer;
                start_timer(&sws_timer);

                // recreate the texture if the video has been resized
                if (pending_texture_update) {
                    replace_texture();
                    pending_texture_update = false;
                }

                pending_resize_render = false;

                // appropriately convert the frame and move it into video_context.data
                finalize_video_context_data();

                // then, update the window titlebar color
                update_window_titlebar_color();

                // The texture object we allocate is larger than the frame (unless
                // MAX_SCREEN_WIDTH/HEIGHT) are violated, so we only copy the valid section of the
                // frame into the texture.

                start_timer(&latency_clock);
                SDL_Rect texture_rect =
                    new_sdl_rect(0, 0, video_context.decoder->width, video_context.decoder->height);
                // TODO: wrap this in Fractal update texture
                int ret = SDL_UpdateYUVTexture(video_context.texture, &texture_rect,
                                               video_context.data[0], video_context.linesize[0],
                                               video_context.data[1], video_context.linesize[1],
                                               video_context.data[2], video_context.linesize[2]);
                if (ret == -1) {
                    LOG_ERROR("SDL_UpdateYUVTexture failed: %s", SDL_GetError());
                }

                if (!video_context.sws) {
                    // Clear out bits that aren't used from av_alloc_frame
                    memset(video_context.data, 0, sizeof(video_context.data));
                }
                log_double_statistic("Write to SDL texture time in ms",
                                     get_timer(latency_clock) * 1000);

                // Subsection of texture that should be rendered to screen.

                start_timer(&latency_clock);
                SDL_Rect output_rect = get_true_render_rect();
                SDL_RenderCopy(renderer, video_context.texture, &output_rect, NULL);

                if (render_peers(renderer, peer_update_msgs, num_peer_update_msgs) != 0) {
                    LOG_ERROR("Failed to render peers.");
                }
                // this call takes up to 16 ms: takes 8 ms on average.
                SDL_RenderPresent(renderer);
                log_double_statistic("Rendering time in ms", get_timer(latency_clock) * 1000);
            }

            safe_SDL_UnlockMutex(render_mutex);

#if LOG_VIDEO
            LOG_DEBUG("Rendered %d (Size: %d) (Age %f)", render_context.id,
                      render_context.frame_size, get_timer(render_context.frame_creation_timer));
#endif

            // Clean up: set global variables and free the frame buffer
            if (frame->is_iframe) {
                video_data.is_waiting_for_iframe = false;
            }

            video_data.last_rendered_id = render_context.id;
        }
        has_video_rendered_yet = true;
        // rendering = false is set to false last,
        // since that can trigger the next frame render
        rendering = false;
    } else {
        // If rendering == false,
        // Then we potentially render the loading screen as long as loading_index is valid
        if (video_data.loading_index >= 0) {
            const float loading_animation_fps = 20.0;
            if (get_timer(video_data.last_loading_frame_timer) > 1 / loading_animation_fps) {
                // Present the loading screen
                loading_sdl(renderer, video_data.loading_index);
                // Progress animation
                video_data.loading_index++;
                // Reset timer
                start_timer(&video_data.last_loading_frame_timer);
            }
        }
    }

    return 0;
}

void destroy_video() {
    /*
        Free the video thread and VideoContext data to exit
    */

    if (!initialized_video_renderer) {
        LOG_ERROR("Destroying video, but never called init_video_renderer");
    } else {
        SDL_DestroyRenderer((SDL_Renderer*)video_context.renderer);

        if (native_window_color) {
            free((FractalRGBColor*)native_window_color);
        }

        // SDL_DestroyTexture(videoContext.texture); is not needed,
        // the renderer destroys it
        av_freep(&video_context.data[0]);
        if (destroy_peer_cursors() != 0) {
            LOG_ERROR("Failed to destroy peer cursors.");
        }
        // Destroy the ring buffer
        destroy_ring_buffer(video_ring_buffer);
    }

    SDL_DestroyMutex(render_mutex);
    render_mutex = NULL;

    has_video_rendered_yet = false;
}

void set_video_active_resizing(bool is_resizing) {
    /*
        Set the global variable 'resizing' to true if the SDL window is
        being resized, else false

        Arguments:
            is_resizing (bool): Boolean indicating whether or not the SDL
                window is being resized
    */

    if (!is_resizing) {
        safe_SDL_LockMutex(render_mutex);

        int new_width = get_window_pixel_width((SDL_Window*)window);
        int new_height = get_window_pixel_height((SDL_Window*)window);
        if (new_width != output_width || new_height != output_height) {
            pending_texture_update = true;
            pending_sws_update = true;
            output_width = new_width;
            output_height = new_height;
        }
        can_render = true;
        safe_SDL_UnlockMutex(render_mutex);
    } else {
        safe_SDL_LockMutex(render_mutex);
        can_render = false;
        safe_SDL_UnlockMutex(render_mutex);

        for (int i = 0; pending_resize_render && (i < 10); ++i) {
            SDL_Delay(1);
        }

        if (pending_resize_render) {
            safe_SDL_LockMutex(render_mutex);
            pending_resize_render = false;
            safe_SDL_UnlockMutex(render_mutex);
        }
    }
}
