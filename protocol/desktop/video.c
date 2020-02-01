#include "video.h"

#define DECODE_TYPE QSV_DECODE

// Global Variables

extern volatile int server_width;
extern volatile int server_height;
// Keeping track of max mbps
extern volatile double max_mbps;
extern volatile bool update_mbps;

// START VIDEO VARIABLES

struct VideoData {
    struct FrameData* pending_ctx;
    int frames_received;
    int bytes_transferred;
    clock frame_timer;
    int last_statistics_id;
    int last_rendered_id;
    int max_id;

    SDL_Thread* render_screen_thread;
    bool run_render_screen_thread;

    SDL_sem* renderscreen_semaphore;
} volatile VideoData;

typedef struct SDLVideoContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    Uint8* yPlane;
    Uint8* uPlane;
    Uint8* vPlane;
    int uvPitch;
    video_decoder_t* decoder;
    struct SwsContext* sws;
} SDLVideoContext;
volatile SDLVideoContext videoContext;

typedef struct FrameData {
    char* prev_frame;
    int frame_size;
    int id;
    int packets_received;
    int num_packets;
    bool received_indicies[LARGEST_FRAME_SIZE / MAX_PAYLOAD_SIZE + 5];

    int last_nacked_id;

    float time_sent[LARGEST_FRAME_SIZE / MAX_PAYLOAD_SIZE + 5];

    clock client_frame_timer;
    float client_frame_time;
    float time2;
    float time3;
} FrameData;

// mbps that currently works
volatile double working_mbps = START_MAX_MBPS;

// Context of the frame that is currently being rendered
volatile struct FrameData renderContext;

// True if RenderScreen is currently rendering a frame
volatile bool rendering = false;

// Hold information about frames as the packets come in
#define RECV_FRAMES_BUFFER_SIZE 100
struct FrameData receiving_frames[RECV_FRAMES_BUFFER_SIZE];
char frame_bufs[RECV_FRAMES_BUFFER_SIZE][LARGEST_FRAME_SIZE];

// END VIDEO VARIABLES

// START VIDEO FUNCTIONS

void updateWidthAndHeight(int width, int height);
int32_t RenderScreen(void* opaque);

void updateWidthAndHeight(int width, int height) {
    struct SwsContext* sws_ctx = NULL;

    enum AVPixelFormat input_fmt;
    if (DECODE_TYPE == QSV_DECODE) {
        input_fmt = AV_PIX_FMT_NV12;
    }
    else {
        input_fmt = AV_PIX_FMT_YUV420P;
    }

    sws_ctx = sws_getContext(width, height,
        input_fmt, OUTPUT_WIDTH, OUTPUT_HEIGHT,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    videoContext.sws = sws_ctx;

    video_decoder_t* decoder = create_video_decoder(width, height, OUTPUT_WIDTH, OUTPUT_HEIGHT, DECODE_TYPE);
    videoContext.decoder = decoder;

    server_width = width;
    server_height = height;
}

int32_t RenderScreen(void* opaque) {
    mprintf("RenderScreen running on Thread %d\n", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    while (VideoData.run_render_screen_thread) {
        int ret = SDL_SemTryWait(VideoData.renderscreen_semaphore);
        if (ret == SDL_MUTEX_TIMEDOUT) {
            continue;
        }
        if (ret < 0) {
            mprintf("Semaphore Error\n");
            return -1;
        }

        if (!rendering) {
            mprintf("Sem opened but rendering is not true!\n");
            continue;
        }

        //mprintf("Rendering ID %d\n", renderContext.id);

        Frame* frame = renderContext.prev_frame;
        if (frame->width != server_width || frame->height != server_height) {
            mprintf("Updating server width and height! From %dx%d to %dx%d\n", server_width, server_height, frame->width, frame->height);
            updateWidthAndHeight(frame->width, frame->height);
        }

        clock t;
        StartTimer(&t);
        video_decoder_decode(videoContext.decoder, frame->compressed_frame, frame->size);
        //mprintf("Decode Time: %f\n", GetTimer(t));

        AVPicture pict;
        pict.data[0] = videoContext.yPlane;
        pict.data[1] = videoContext.uPlane;
        pict.data[2] = videoContext.vPlane;
        pict.linesize[0] = OUTPUT_WIDTH;
        pict.linesize[1] = videoContext.uvPitch;
        pict.linesize[2] = videoContext.uvPitch;
        sws_scale(videoContext.sws, (uint8_t const* const*)videoContext.decoder->sw_frame->data,
            videoContext.decoder->sw_frame->linesize, 0, videoContext.decoder->context->height, pict.data,
            pict.linesize);

        SDL_UpdateYUVTexture(
            videoContext.texture,
            NULL,
            videoContext.yPlane,
            OUTPUT_WIDTH,
            videoContext.uPlane,
            videoContext.uvPitch,
            videoContext.vPlane,
            videoContext.uvPitch
        );

        SDL_RenderClear(videoContext.renderer);
        SDL_RenderCopy(videoContext.renderer, videoContext.texture, NULL, NULL);
        //mprintf("Client Frame Time for ID %d: %f\n", renderContext.id, GetTimer(renderContext.client_frame_timer));
        SDL_RenderPresent(videoContext.renderer);

        rendering = false;
    }

    SDL_Delay(5);
}
// END VIDEO FUNCTIONS

void initVideo() {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    Uint8* yPlane, * uPlane, * vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;

    window = SDL_CreateWindow(
        "Fractal",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        OUTPUT_WIDTH,
        OUTPUT_HEIGHT,
        0
    );

    if (!window) {
        fprintf(stderr, "SDL: could not create window - exiting\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting\n");
        exit(1);
    }
    // Allocate a place to put our YUV image on that screen
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING,
        OUTPUT_WIDTH,
        OUTPUT_HEIGHT
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    // set up YV12 pixel array (12 bits per pixel)
    yPlaneSz = OUTPUT_WIDTH * OUTPUT_HEIGHT;
    uvPlaneSz = OUTPUT_WIDTH * OUTPUT_HEIGHT / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);

    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = OUTPUT_WIDTH / 2;

    videoContext.window = window;
    videoContext.renderer = renderer;
    videoContext.texture = texture;

    videoContext.yPlane = yPlane;
    videoContext.uPlane = uPlane;
    videoContext.vPlane = vPlane;
    videoContext.uvPitch = uvPitch;

    VideoData.pending_ctx = NULL;
    VideoData.frames_received = 0;
    VideoData.bytes_transferred = 0;
    StartTimer(&VideoData.frame_timer);
    VideoData.last_statistics_id = 1;
    VideoData.last_rendered_id = 1;
    VideoData.max_id = 1;

    for (int i = 0; i < RECV_FRAMES_BUFFER_SIZE; i++) {
        receiving_frames[i].id = -1;
    }

    VideoData.renderscreen_semaphore = SDL_CreateSemaphore(0);
    VideoData.run_render_screen_thread = true;
    VideoData.render_screen_thread = SDL_CreateThread(RenderScreen, "RenderScreen", NULL);
}

void updateVideo() {
    // Get statistics from the last 3 seconds of data
    if (GetTimer(VideoData.frame_timer) > 3) {
        double time = GetTimer(VideoData.frame_timer);

        // Calculate statistics
        int expected_frames = VideoData.max_id - VideoData.last_statistics_id;
        double fps = 1.0 * expected_frames / time;
        double mbps = VideoData.bytes_transferred * 8.0 / 1024.0 / 1024.0 / time;
        double receive_rate = expected_frames == 0 ? 1.0 : 1.0 * VideoData.frames_received / expected_frames;
        double dropped_rate = 1.0 - receive_rate;

        // Print statistics

        //mprintf("FPS: %f\nmbps: %f\ndropped: %f%%\n\n", fps, mbps, 100.0 * dropped_rate);

        // Adjust mbps based on dropped packets
        if (dropped_rate > 0.4) {
            max_mbps = max_mbps * 0.75;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.2) {
            max_mbps = max_mbps * 0.83;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.1) {
            max_mbps = max_mbps * 0.9;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.05) {
            max_mbps = max_mbps * 0.95;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.00) {
            max_mbps = max_mbps * 0.98;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate == 0.00) {
            working_mbps = max(max_mbps * 1.05, working_mbps);
            max_mbps = (max_mbps + working_mbps) / 2.0;
            update_mbps = true;
        }

        VideoData.bytes_transferred = 0;
        VideoData.frames_received = 0;
        VideoData.last_statistics_id = VideoData.max_id;
        StartTimer(&VideoData.frame_timer);
    }

    if (!rendering) {
        if (VideoData.pending_ctx != NULL) {
            renderContext = *VideoData.pending_ctx;
            rendering = true;

            //mprintf("Status: %f\n", GetTimer(renderContext.client_frame_timer));
            SDL_SemPost(VideoData.renderscreen_semaphore);

            VideoData.pending_ctx = NULL;
        }
    }
}

int32_t ReceiveVideo(struct RTPPacket* packet, int recv_size) {
    //mprintf("Packet ID %d, Packet Index %d, Hash %x\n", packet->id, packet->index, packet->hash);

    // Find frame in linked list that matches the id
    VideoData.bytes_transferred += recv_size;

    int index = packet->id % RECV_FRAMES_BUFFER_SIZE;

    struct FrameData* ctx = &receiving_frames[index];
    if (ctx->id != packet->id) {
        if (rendering && renderContext.id == ctx->id) {
            mprintf("Error! Currently rendering an ID that will be overwritten! Skipping packet.\n");
            return 0;
        }
        ctx->id = packet->id;
        ctx->prev_frame = &frame_bufs[index];
        ctx->packets_received = 0;
        ctx->num_packets = -1;
        ctx->last_nacked_id = -1;
        memset(ctx->received_indicies, 0, sizeof(ctx->received_indicies));
        StartTimer(&ctx->client_frame_timer);
    }
    else {
       // mprintf("Already Started: %d/%d - %f\n", ctx->packets_received + 1, ctx->num_packets, GetTimer(ctx->client_frame_timer));
    }

    if (ctx->received_indicies[packet->index]) {
        //mprintf("NACK for Video ID %d, Index %d Received!\n", packet->id, packet->index);
        return 0;
    }

    ctx->received_indicies[packet->index] = true;
    if (packet->index > 0) {
        for (int i = max(0, ctx->last_nacked_id + 1); i < packet->index; i++) {
            if (!ctx->received_indicies[i]) {
                //mprintf("Missing Video Packet ID %d Index %d, NACKing...\n", packet->id, i);
                FractalClientMessage fmsg;
                fmsg.type = MESSAGE_VIDEO_NACK;
                fmsg.nack_data.id = packet->id;
                fmsg.nack_data.index = i;
                SendPacket(&fmsg, sizeof(fmsg));
                SendPacket(&fmsg, sizeof(fmsg));
                SendPacket(&fmsg, sizeof(fmsg));
            }
        }
        ctx->last_nacked_id = max(ctx->last_nacked_id, packet->index - 1);
    }
    ctx->packets_received++;

    // Copy packet data
    int place = packet->index * MAX_PAYLOAD_SIZE;
    if (place + packet->payload_size >= LARGEST_FRAME_SIZE) {
        mprintf("Packet total payload is too large for buffer!\n");
        return -1;
    }
    memcpy(ctx->prev_frame + place, packet->data, packet->payload_size);
    ctx->frame_size += packet->payload_size;

    // Keep track of how many packets are necessary
    if (packet->is_ending) {
        ctx->num_packets = packet->index + 1;
    }

    // If we received all of the packets
    if (ctx->packets_received == ctx->num_packets) {
        VideoData.frames_received++;

        if (packet->id > VideoData.max_id) {
            VideoData.max_id = packet->id;
            //mprintf("Received all packets for id %d, getting ready to render\n", packet.id);

            for (int i = VideoData.last_rendered_id + 1; i <= VideoData.max_id; i++) {
                int index = i % RECV_FRAMES_BUFFER_SIZE;
                if (receiving_frames[index].id == i && receiving_frames[index].packets_received != receiving_frames[index].num_packets) {
                    mprintf("Frame dropped with ID %d: %d/%d\n", i, receiving_frames[index].packets_received, receiving_frames[index].num_packets);
                }
            }

            VideoData.last_rendered_id = VideoData.max_id;

            VideoData.pending_ctx = ctx;
        }

    }

    return 0;
}

void destroyVideo() {
    VideoData.run_render_screen_thread = false;
    SDL_WaitThread(VideoData.render_screen_thread, NULL);
    SDL_DestroySemaphore(VideoData.renderscreen_semaphore);

    SDL_DestroyTexture(videoContext.texture);
    SDL_DestroyRenderer(videoContext.renderer);
    SDL_DestroyWindow(videoContext.window);
    free(videoContext.yPlane);
    free(videoContext.uPlane);
    free(videoContext.vPlane);
}