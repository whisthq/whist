/*
 * General client video functions.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "video.h"

#include <stdio.h>

#define USE_HARDWARE true

// Global Variables
extern volatile SDL_Window* window;

extern volatile int server_width;
extern volatile int server_height;
// Keeping track of max mbps
extern volatile int max_bitrate;
extern volatile bool update_mbps;

extern volatile int output_width;
extern volatile int output_height;

// START VIDEO VARIABLES
volatile FractalCursorState cursor_state = CURSOR_STATE_VISIBLE;
volatile SDL_Cursor* cursor = NULL;
volatile FractalCursorID last_cursor = (FractalCursorID)SDL_SYSTEM_CURSOR_ARROW;

#define LOG_VIDEO false

#define BITRATE_BUCKET_SIZE 500000

#define CURSORIMAGE_A 0xff000000
#define CURSORIMAGE_R 0x00ff0000
#define CURSORIMAGE_G 0x0000ff00
#define CURSORIMAGE_B 0x000000ff

struct VideoData {
    struct FrameData* pending_ctx;
    int frames_received;
    int bytes_transferred;
    clock frame_timer;
    int last_statistics_id;
    int last_rendered_id;
    int max_id;
    int most_recent_iframe;

    clock last_iframe_request_timer;
    bool is_waiting_for_iframe;

    SDL_Thread* render_screen_thread;
    bool run_render_screen_thread;

    SDL_sem* renderscreen_semaphore;

    double target_mbps;
    int num_nacked;
    int bucket;  // = STARTING_BITRATE / BITRATE_BUCKET_SIZE;
    int nack_by_bitrate[MAXIMUM_BITRATE / BITRATE_BUCKET_SIZE + 5];
    double seconds_by_bitrate[MAXIMUM_BITRATE / BITRATE_BUCKET_SIZE + 5];
} VideoData;

typedef struct SDLVideoContext {
    SDL_Texture* texture;

    Uint8* data[4];
    int linesize[4];

    video_decoder_t* decoder;
    struct SwsContext* sws;
} SDLVideoContext;
SDLVideoContext videoContext;

typedef struct FrameData {
    char* frame_buffer;
    int frame_size;
    int id;
    int packets_received;
    int num_packets;
    bool received_indicies[LARGEST_FRAME_SIZE / MAX_PAYLOAD_SIZE + 5];
    bool nacked_indicies[LARGEST_FRAME_SIZE / MAX_PAYLOAD_SIZE + 5];
    bool rendered;

    int num_times_nacked;

    int last_nacked_index;

    clock last_nacked_timer;

    clock last_packet_timer;

    clock frame_creation_timer;
} FrameData;

// mbps that currently works
volatile double working_mbps;

// Context of the frame that is currently being rendered
volatile struct FrameData renderContext;

// True if RenderScreen is currently rendering a frame
volatile bool rendering = false;
volatile bool skip_render = false;
volatile bool resizing = false;

// Hold information about frames as the packets come in
#define RECV_FRAMES_BUFFER_SIZE 275
struct FrameData receiving_frames[RECV_FRAMES_BUFFER_SIZE];
char frame_bufs[RECV_FRAMES_BUFFER_SIZE][LARGEST_FRAME_SIZE];

bool has_rendered_yet = false;

// END VIDEO VARIABLES

// START VIDEO FUNCTIONS

void updateWidthAndHeight(int width, int height);
int32_t RenderScreen(SDL_Renderer* renderer);
void loadingSDL(SDL_Renderer* renderer, int loading_index);

void nack(int id, int index) {
    if (VideoData.is_waiting_for_iframe) {
        return;
    }
    VideoData.num_nacked++;
    LOG_INFO("Missing Video Packet ID %d Index %d, NACKing...", id, index);
    FractalClientMessage fmsg;
    fmsg.type = MESSAGE_VIDEO_NACK;
    fmsg.nack_data.id = id;
    fmsg.nack_data.index = index;
    SendFmsg(&fmsg);
}

bool requestIframe() {
    if (GetTimer(VideoData.last_iframe_request_timer) > 250.0 / 1000.0) {
        FractalClientMessage fmsg;
        fmsg.type = MESSAGE_IFRAME_REQUEST;
        if (VideoData.last_rendered_id == 0) {
            fmsg.reinitialize_encoder = true;
        } else {
            fmsg.reinitialize_encoder = false;
        }
        SendFmsg(&fmsg);
        StartTimer(&VideoData.last_iframe_request_timer);
        VideoData.is_waiting_for_iframe = true;
        return true;
    } else {
        return false;
    }
}

void updateWidthAndHeight(int width, int height) {
    video_decoder_t* decoder =
        create_video_decoder(width, height, USE_HARDWARE);
    videoContext.decoder = decoder;
    if (!decoder) {
        LOG_WARNING("ERROR: Decoder could not be created!");
        exit(-1);
    }

    enum AVPixelFormat input_fmt = AV_PIX_FMT_YUV420P;
    if (decoder->type != DECODE_TYPE_SOFTWARE) {
        input_fmt = AV_PIX_FMT_NV12;
    }

    struct SwsContext* sws_ctx = NULL;
    if (input_fmt != AV_PIX_FMT_YUV420P || width != output_width ||
        height != output_height) {
        sws_ctx = sws_getContext(width, height, input_fmt, output_width,
                                 output_height, AV_PIX_FMT_YUV420P,
                                 SWS_BILINEAR, NULL, NULL, NULL);
    }
    struct SwsContext* old_sws_ctx = videoContext.sws;
    videoContext.sws = sws_ctx;
    sws_freeContext(old_sws_ctx);

    server_width = width;
    server_height = height;
}

int32_t RenderScreen(SDL_Renderer* renderer) {
    LOG_INFO("RenderScreen running on Thread %d", SDL_GetThreadID(NULL));

    int loading_index = 0;

    // present the loading screen
    loadingSDL(renderer, loading_index);

    while (VideoData.run_render_screen_thread) {
        int ret = SDL_SemTryWait(VideoData.renderscreen_semaphore);

        if (ret == SDL_MUTEX_TIMEDOUT) {
            if (loading_index >= 0) {
                loading_index++;
                loadingSDL(renderer, loading_index);
            }
            SDL_Delay(1);
            continue;
        }

        loading_index = -1;

        if (ret < 0) {
            LOG_ERROR("Semaphore Error");
            return -1;
        }

        if (!rendering) {
            LOG_WARNING("Sem opened but rendering is not true!");
            continue;
        }

        // Cast to Frame* because this variable is not volatile in this section
        Frame* frame = (Frame*)renderContext.frame_buffer;

#if LOG_VIDEO
        mprintf("Rendering ID %d (Age %f) (Packets %d) %s\n", renderContext.id,
                GetTimer(renderContext.frame_creation_timer),
                renderContext.num_packets, frame->is_iframe ? "(I-Frame)" : "");
#endif

        if (GetTimer(renderContext.frame_creation_timer) > 25.0 / 1000.0) {
            LOG_INFO(
                "Late! Rendering ID %d (Age %f) (Packets %d) %s",
                renderContext.id, GetTimer(renderContext.frame_creation_timer),
                renderContext.num_packets, frame->is_iframe ? "(I-Frame)" : "");
        }

        if ((int)(sizeof(Frame) + frame->size) != renderContext.frame_size) {
            mprintf("Incorrect Frame Size! %d instead of %d\n",
                    sizeof(Frame) + frame->size, renderContext.frame_size);
        }

        if (frame->width != server_width || frame->height != server_height) {
            LOG_INFO(
                "Updating client rendering to match server's width and height! "
                "From %dx%d to %dx%d",
                server_width, server_height, frame->width, frame->height);
            updateWidthAndHeight(frame->width, frame->height);
        }

        if (!video_decoder_decode(videoContext.decoder, frame->compressed_frame,
                                  frame->size)) {
            LOG_WARNING("Failed to video_decoder_decode!");
            rendering = false;
            continue;
        }

        if (!skip_render && !resizing) {
            if (videoContext.sws) {
                sws_scale(
                    videoContext.sws,
                    (uint8_t const* const*)videoContext.decoder->sw_frame->data,
                    videoContext.decoder->sw_frame->linesize, 0,
                    videoContext.decoder->context->height, videoContext.data,
                    videoContext.linesize);
            } else {
                memcpy(videoContext.data, videoContext.decoder->sw_frame->data,
                       sizeof(videoContext.data));
                memcpy(videoContext.linesize,
                       videoContext.decoder->sw_frame->linesize,
                       sizeof(videoContext.linesize));
            }
            SDL_UpdateYUVTexture(videoContext.texture, NULL,
                                 videoContext.data[0], videoContext.linesize[0],
                                 videoContext.data[1], videoContext.linesize[1],
                                 videoContext.data[2],
                                 videoContext.linesize[2]);
        }

        // Set cursor to frame's desired cursor type
        if ((FractalCursorID)frame->cursor.cursor_id != last_cursor ||
            frame->cursor.cursor_use_bmp) {
            if (cursor) {
                SDL_FreeCursor((SDL_Cursor*)cursor);
            }
            if (frame->cursor.cursor_use_bmp) {
                // use bitmap data to set cursor

                SDL_Surface* cursor_surface = SDL_CreateRGBSurfaceFrom(
                    frame->cursor.cursor_bmp, frame->cursor.cursor_bmp_width,
                    frame->cursor.cursor_bmp_height, sizeof(uint32_t) * 8,
                    sizeof(uint32_t) * frame->cursor.cursor_bmp_width,
                    CURSORIMAGE_R, CURSORIMAGE_G, CURSORIMAGE_B, CURSORIMAGE_A);
                // potentially SDL_SetSurfaceBlendMode since X11 cursor BMPs are
                // pre-alpha multplied
                cursor = SDL_CreateColorCursor(cursor_surface,
                                               frame->cursor.cursor_bmp_hot_x,
                                               frame->cursor.cursor_bmp_hot_y);
                SDL_FreeSurface(cursor_surface);
            } else {
                // use cursor id to set cursor
                cursor = SDL_CreateSystemCursor(frame->cursor.cursor_id);
            }
            SDL_SetCursor((SDL_Cursor*)cursor);

            last_cursor = (FractalCursorID)frame->cursor.cursor_id;
        }

        if (frame->cursor.cursor_state != cursor_state) {
            if (frame->cursor.cursor_state == CURSOR_STATE_HIDDEN) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            } else {
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }

            cursor_state = frame->cursor.cursor_state;
        }

        // mprintf("Client Frame Time for ID %d: %f\n", renderContext.id,
        // GetTimer(renderContext.client_frame_timer));

        if (!skip_render && !resizing) {
            // printf("Before, %x\n", renderer);
            SDL_RenderCopy((SDL_Renderer*)renderer, videoContext.texture, NULL,
                           NULL);
            // SDL_RenderCopy((SDL_Renderer*)renderer, NULL, NULL, NULL);
            // printf("After\n");
            // SDL_SetRenderDrawColor((SDL_Renderer*)renderer, 100, 20, 160,
            // SDL_ALPHA_OPAQUE); SDL_RenderClear((SDL_Renderer*)renderer);
            SDL_RenderPresent((SDL_Renderer*)renderer);
        }

#if LOG_VIDEO
        LOG_DEBUG("Rendered %d (Size: %d) (Age %f)\n", renderContext.id,
                renderContext.frame_size,
                GetTimer(renderContext.frame_creation_timer));
#endif

        if (frame->is_iframe) {
            VideoData.is_waiting_for_iframe = false;
        }

        VideoData.last_rendered_id = renderContext.id;
        has_rendered_yet = true;
        rendering = false;
    }

    SDL_Delay(5);
    return 0;
}

// Make the screen black
void loadingSDL(SDL_Renderer* renderer, int loading_index) {
    static SDL_Texture* loading_screen_texture = NULL;

    int gif_frame_index = loading_index % 83;

    while (true) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        char frame_name[24];
        if (gif_frame_index < 10) {
            snprintf(frame_name, sizeof(frame_name), "loading/frame_0%d.bmp",
                     gif_frame_index);
        } else {
            snprintf(frame_name, sizeof(frame_name), "loading/frame_%d.bmp",
                     gif_frame_index);
        }

        SDL_Surface* loading_screen = SDL_LoadBMP(frame_name);
        loading_screen_texture =
            SDL_CreateTextureFromSurface(renderer, loading_screen);
        SDL_FreeSurface(loading_screen);

        int w = 200;
        int h = 200;
        SDL_Rect dstrect;

        // SDL_QueryTexture( loading_screen_texture, NULL, NULL, &w, &h );
        dstrect.x = output_width / 2 - w / 2;
        dstrect.y = output_height / 2 - h / 2;
        dstrect.w = w;
        dstrect.h = h;
        SDL_RenderCopy(renderer, loading_screen_texture, NULL, &dstrect);
        SDL_RenderPresent(renderer);

        SDL_Delay(30);  // sleep 30 ms
        gif_frame_index += 1;
        gif_frame_index %= 83;  // number of loading frames
        break;
    }
}

void clearSDL(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

int initMultithreadedVideo(void* opaque) {
    opaque;

    LOG_INFO("Creating renderer for %dx%d display", output_width,
             output_height);

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    SDL_Renderer* renderer = SDL_CreateRenderer(
        (SDL_Window*)window, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        LOG_WARNING("SDL: could not create renderer - exiting: %s",
                    SDL_GetError());
        return -1;
    }

    // configure texture
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

    // mbps that currently works
    working_mbps = STARTING_BITRATE;
    VideoData.is_waiting_for_iframe = false;

    // True if RenderScreen is currently rendering a frame
    rendering = false;
    has_rendered_yet = false;

    SDL_Texture* texture;

    SDL_SetRenderDrawBlendMode((SDL_Renderer*)renderer, SDL_BLENDMODE_BLEND);
    // Allocate a place to put our YUV image on that screen
    texture = SDL_CreateTexture((SDL_Renderer*)renderer, SDL_PIXELFORMAT_YV12,
                                SDL_TEXTUREACCESS_STREAMING, output_width,
                                output_height);
    if (!texture) {
        LOG_ERROR("SDL: could not create texture - exiting");
        exit(1);
    }

    videoContext.texture = texture;

    av_image_alloc(videoContext.data, videoContext.linesize, output_width,
                   output_height, AV_PIX_FMT_YUV420P, 1);

    max_bitrate = STARTING_BITRATE;
    VideoData.target_mbps = STARTING_BITRATE;
    VideoData.pending_ctx = NULL;
    VideoData.frames_received = 0;
    VideoData.bytes_transferred = 0;
    StartTimer(&VideoData.frame_timer);
    VideoData.last_statistics_id = 1;
    VideoData.last_rendered_id = 0;
    VideoData.max_id = 0;
    VideoData.most_recent_iframe = -1;
    VideoData.num_nacked = 0;
    VideoData.bucket = STARTING_BITRATE / BITRATE_BUCKET_SIZE;
    StartTimer(&VideoData.last_iframe_request_timer);

    for (int i = 0; i < RECV_FRAMES_BUFFER_SIZE; i++) {
        receiving_frames[i].id = -1;
    }

    VideoData.renderscreen_semaphore = SDL_CreateSemaphore(0);
    VideoData.run_render_screen_thread = true;

    RenderScreen(renderer);
    SDL_DestroyRenderer(renderer);
    return 0;
}
// END VIDEO FUNCTIONS

void initVideo() {
    VideoData.render_screen_thread =
        SDL_CreateThread(initMultithreadedVideo, "VideoThread", NULL);
}

int last_rendered_index = 0;

void updateVideo() {
    // Get statistics from the last 3 seconds of data
    if (GetTimer(VideoData.frame_timer) > 3) {
        double time = GetTimer(VideoData.frame_timer);

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

        double nack_per_second = VideoData.num_nacked / time;
        VideoData.nack_by_bitrate[VideoData.bucket] += VideoData.num_nacked;
        VideoData.seconds_by_bitrate[VideoData.bucket] += time;

        mprintf( "====\nBucket: %d\nSeconds: %f\nNacks/Second: %f\n====\n", VideoData.bucket*BITRATE_BUCKET_SIZE, time, nack_per_second );

        // Print statistics

        // mprintf("FPS: %f\nmbps: %f\ndropped: %f%%\n\n", fps, mbps, 100.0 *
        // dropped_rate);

        LOG_INFO("MBPS: %f %f", VideoData.target_mbps, nack_per_second);

        // Adjust mbps based on dropped packets
        if (nack_per_second > 50) {
            VideoData.target_mbps = VideoData.target_mbps * 0.75;
            working_mbps = VideoData.target_mbps;
            update_mbps = true;
        } else if (nack_per_second > 25) {
            VideoData.target_mbps = VideoData.target_mbps * 0.83;
            working_mbps = VideoData.target_mbps;
            update_mbps = true;
        } else if (nack_per_second > 15) {
            VideoData.target_mbps = VideoData.target_mbps * 0.9;
            working_mbps = VideoData.target_mbps;
            update_mbps = true;
        } else if (nack_per_second > 10) {
            VideoData.target_mbps = VideoData.target_mbps * 0.95;
            working_mbps = VideoData.target_mbps;
            update_mbps = true;
        } else if (nack_per_second > 6) {
            VideoData.target_mbps = VideoData.target_mbps * 0.98;
            working_mbps = VideoData.target_mbps;
            update_mbps = true;
        } else {
            working_mbps = max(VideoData.target_mbps * 1.05, working_mbps);
            VideoData.target_mbps =
                (VideoData.target_mbps + working_mbps) / 2.0;
            VideoData.target_mbps =
                min(VideoData.target_mbps, MAXIMUM_BITRATE);
            update_mbps = true;
        }

        LOG_INFO("MBPS2: %f", VideoData.target_mbps);

        VideoData.bucket = (int)VideoData.target_mbps / BITRATE_BUCKET_SIZE;
        max_bitrate = (int)VideoData.bucket * BITRATE_BUCKET_SIZE +
                      BITRATE_BUCKET_SIZE / 2;

        LOG_INFO("MBPS3: %d", max_bitrate);
        VideoData.num_nacked = 0;

        VideoData.bytes_transferred = 0;
        VideoData.frames_received = 0;
        VideoData.last_statistics_id = VideoData.max_id;
        StartTimer(&VideoData.frame_timer);
    }

    if (VideoData.last_rendered_id == -1 && VideoData.most_recent_iframe > 0) {
        VideoData.last_rendered_id = VideoData.most_recent_iframe - 1;
    }

    if (!rendering && VideoData.last_rendered_id >= 0) {
        if (VideoData.most_recent_iframe - 1 > VideoData.last_rendered_id) {
            LOG_INFO("Skipping from %d to i-frame %d!",
                     VideoData.last_rendered_id, VideoData.most_recent_iframe);
            for (int i = VideoData.last_rendered_id + 1;
                 i < VideoData.most_recent_iframe; i++) {
                int index = i % RECV_FRAMES_BUFFER_SIZE;
                if (receiving_frames[index].id == i) {
                    LOG_WARNING("Frame dropped with ID %d: %d/%d", i,
                                receiving_frames[index].packets_received,
                                receiving_frames[index].num_packets);

                    for (int j = 0; j < receiving_frames[index].num_packets;
                         j++) {
                        if (!receiving_frames[index].received_indicies[j]) {
                            LOG_WARNING("Did not receive ID %d, Index %d", i,
                                        j);
                        }
                    }
                } else {
                    LOG_WARNING("Bad ID? %d instead of %d",
                                receiving_frames[index].id, i);
                }
            }
            VideoData.last_rendered_id = VideoData.most_recent_iframe - 1;
        }

        int next_render_id = VideoData.last_rendered_id + 1;

        int index = next_render_id % RECV_FRAMES_BUFFER_SIZE;

        struct FrameData* ctx = &receiving_frames[index];

        if (ctx->id == next_render_id) {
            if (ctx->packets_received == ctx->num_packets) {
                // mprintf( "Packets: %d %s\n", ctx->num_packets,
                // ((Frame*)ctx->frame_buffer)->is_iframe ? "(I-frame)" : "" );
                // mprintf("Rendering %d (Age %f)\n", ctx->id,
                // GetTimer(ctx->frame_creation_timer));

                renderContext = *ctx;
                rendering = true;

                skip_render = false;
                int after_render_id = next_render_id + 1;
                int after_index = after_render_id % RECV_FRAMES_BUFFER_SIZE;
                struct FrameData* after_ctx = &receiving_frames[after_index];
                if (after_ctx->id == after_render_id &&
                    after_ctx->packets_received == after_ctx->num_packets) {
                    skip_render = true;
                    LOG_INFO("Skip this render");
                }

                SDL_SemPost(VideoData.renderscreen_semaphore);
            } else {
                if ((GetTimer(ctx->last_packet_timer) > 6.0 / 1000.0) &&
                    GetTimer(ctx->last_nacked_timer) >
                        (8.0 + 8.0 * ctx->num_times_nacked) / 1000.0) {
                    if (ctx->num_times_nacked == -1) {
                        ctx->num_times_nacked = 0;
                        ctx->last_nacked_index = -1;
                    }
                    int num_nacked = 0;
                    // mprintf("************NACKING PACKET %d, alive for %f
                    // MS\n", ctx->id, GetTimer(ctx->frame_creation_timer));
                    for (int i = ctx->last_nacked_index + 1;
                         i < ctx->num_packets && num_nacked < 1; i++) {
                        if (!ctx->received_indicies[i]) {
                            num_nacked++;
                            LOG_INFO(
                                "************NACKING VIDEO PACKET %d %d (/%d), "
                                "alive for %f MS",
                                ctx->id, i, ctx->num_packets,
                                GetTimer(ctx->frame_creation_timer));
                            ctx->nacked_indicies[i] = true;
                            nack(ctx->id, i);
                        }
                        ctx->last_nacked_index = i;
                    }
                    if (ctx->last_nacked_index == ctx->num_packets - 1) {
                        ctx->last_nacked_index = -1;
                        ctx->num_times_nacked++;
                    }
                    StartTimer(&ctx->last_nacked_timer);
                }
            }
        }

        if (!rendering) {
            // struct FrameData* cur_ctx =
            // &receiving_frames[VideoData.last_rendered_id %
            // RECV_FRAMES_BUFFER_SIZE];

            if (VideoData.max_id >
                VideoData.last_rendered_id +
                    3)  // || (cur_ctx->id == VideoData.last_rendered_id &&
                        // GetTimer( cur_ctx->last_packet_timer ) > 96.0 /
                        // 1000.0) )
            {
                if (requestIframe()) {
                    LOG_INFO("TOO FAR BEHIND! REQUEST FOR IFRAME!");
                }
            }
        }
    }
}

int32_t ReceiveVideo(FractalPacket* packet) {
    // mprintf("Video Packet ID %d, Index %d (Packets: %d) (Size: %d)\n",
    // packet->id, packet->index, packet->num_indices, packet->payload_size);

    // Find frame in linked list that matches the id
    VideoData.bytes_transferred += packet->payload_size;

    int index = packet->id % RECV_FRAMES_BUFFER_SIZE;

    struct FrameData* ctx = &receiving_frames[index];

    // Check if we have to initialize the frame buffer
    if (packet->id < ctx->id) {
        LOG_INFO("Old packet received!");
        return -1;
    } else if (packet->id > ctx->id) {
        if (rendering && renderContext.id == ctx->id) {
            LOG_INFO(
                "Error! Currently rendering an ID that will be overwritten! "
                "Skipping packet.");
            return 0;
        }
        ctx->id = packet->id;
        ctx->frame_buffer = (char*)&frame_bufs[index];
        ctx->packets_received = 0;
        ctx->num_packets = packet->num_indices;
        ctx->last_nacked_index = -1;
        ctx->num_times_nacked = -1;
        ctx->rendered = false;
        ctx->frame_size = 0;
        memset(ctx->received_indicies, 0, sizeof(ctx->received_indicies));
        memset(ctx->nacked_indicies, 0, sizeof(ctx->nacked_indicies));
        StartTimer(&ctx->last_nacked_timer);
        StartTimer(&ctx->frame_creation_timer);
        // mprintf("Initialized packet %d!\n", ctx->id);
    } else {
        // mprintf("Already Started: %d/%d - %f\n", ctx->packets_received + 1,
        // ctx->num_packets, GetTimer(ctx->client_frame_timer));
    }

    StartTimer(&ctx->last_packet_timer);

    // If we already received this packet, we can skip
    if (packet->is_a_nack) {
        if (!ctx->received_indicies[packet->index]) {
            LOG_INFO("NACK for Video ID %d, Index %d Received!", packet->id,
                     packet->index);
        } else {
            LOG_INFO(
                "NACK for Video ID %d, Index %d Received! But didn't need "
                "it.",
                packet->id, packet->index);
        }
    } else if (ctx->nacked_indicies[packet->index]) {
        LOG_INFO(
            "Received the original Video ID %d Index %d, but we had NACK'ed "
            "for it.",
            packet->id, packet->index);
    }

    // Already received
    if (ctx->received_indicies[packet->index]) {
#if LOG_VIDEO
        mprintf(
            "Skipping duplicate Video ID %d Index %d at time since creation %f "
            "%s\n",
            packet->id, packet->index, GetTimer(ctx->frame_creation_timer),
            packet->is_a_nack ? "(nack)" : "");
#endif
        return 0;
    }

#if LOG_VIDEO
    // mprintf("Received Video ID %d Index %d at time since creation %f %s\n",
    // packet->id, packet->index, GetTimer(ctx->frame_creation_timer),
    // packet->is_a_nack ? "(nack)" : "");
#endif

    VideoData.max_id = max(VideoData.max_id, ctx->id);

    ctx->received_indicies[packet->index] = true;
    if (packet->index > 0 && GetTimer(ctx->last_nacked_timer) > 6.0 / 1000) {
        int to_index = packet->index - 5;
        for (int i = max(0, ctx->last_nacked_index + 1); i <= to_index; i++) {
            // Nacking index i
            ctx->last_nacked_index = max(ctx->last_nacked_index, i);
            if (!ctx->received_indicies[i]) {
                ctx->nacked_indicies[i] = true;
                nack(packet->id, i);
                StartTimer(&ctx->last_nacked_timer);
                break;
            }
        }
    }
    ctx->packets_received++;

    // Copy packet data
    int place = packet->index * MAX_PAYLOAD_SIZE;
    if (place + packet->payload_size >= LARGEST_FRAME_SIZE) {
        LOG_WARNING("Packet total payload is too large for buffer!");
        return -1;
    }
    memcpy(ctx->frame_buffer + place, packet->data, packet->payload_size);
    ctx->frame_size += packet->payload_size;

    // If we received all of the packets
    if (ctx->packets_received == ctx->num_packets) {
        bool is_iframe = ((Frame*)ctx->frame_buffer)->is_iframe;

        VideoData.frames_received++;

#if LOG_VIDEO
        mprintf("Received Video Frame ID %d (Packets: %d) (Size: %d) %s\n",
                ctx->id, ctx->num_packets, ctx->frame_size,
                is_iframe ? "(i-frame)" : "");
#endif

        // If it's an I-frame, then just skip right to it, if the id is ahead of
        // the next to render id
        if (is_iframe) {
            VideoData.most_recent_iframe =
                max(VideoData.most_recent_iframe, ctx->id);
        }
    }

    return 0;
}

void destroyVideo() {
    VideoData.run_render_screen_thread = false;
    SDL_WaitThread(VideoData.render_screen_thread, NULL);
    SDL_DestroySemaphore(VideoData.renderscreen_semaphore);

    //    SDL_DestroyTexture(videoContext.texture); not needed, the renderer
    //    destroys it
    av_freep(videoContext.data);

    has_rendered_yet = false;
}

void set_video_active_resizing(bool is_resizing) { resizing = is_resizing; }
