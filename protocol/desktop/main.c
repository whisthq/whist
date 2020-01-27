#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>
#include <synchapi.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <unistd.h>
#endif

#include "../include/fractal.h"
#include "../include/webserver.h" // header file for webserver query functions
#include "../include/videodecode.h" // header file for audio decoder
#include "../include/audiodecode.h" // header file for audio decoder
#include "../include/linkedlist.h" // header file for audio decoder

#define SDL_MAIN_HANDLED
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

#define BUFLEN 500000
#define SDL_AUDIO_BUFFER_SIZE 1024
#define DECODE_TYPE QSV_DECODE

struct SDLVideoContext {
    Uint8* yPlane;
    Uint8* uPlane;
    Uint8* vPlane;
    int uvPitch;
    int frame_size;
    SDL_Renderer* Renderer;
    SDL_Texture* Texture;
    decoder_t* decoder;
    char* prev_frame;
    struct SwsContext* sws;
    int id;
    int packets_received;
    int num_packets;
    bool received_indicies[500];
};

struct VideoData {
    struct SDLVideoContext* pending_ctx;
    int frames_received;
    int bytes_transferred;
    clock frame_timer;
    int last_max_id;
    int max_id;

    int previous_id;
    int previous_index;
    clock packet_timer;

    clock test_timer;
    float test_time;

    SDL_Thread* render_screen_thread;
    bool run_render_screen_thread;

    SDL_sem* renderscreen_semaphore;
} volatile static VideoData;

// Global state variables
volatile static bool shutting_down = false;
volatile static bool rendering = false;
volatile static bool is_timing_latency = false;
volatile static clock latency_timer;

// Base video context that all contexts derive from
volatile static struct SDLVideoContext videoContext;
// Context of the frame that is currently being rendered
volatile static struct SDLVideoContext renderContext;

// Semaphores and Mutexes
volatile static SDL_sem* multithreadedprintf_semaphore;
volatile static SDL_mutex* multithreadedprintf_mutex;

// Hold information about frames as the packets come in
#define RECV_FRAMES_BUFFER_SIZE 100
struct SDLVideoContext receiving_frames[RECV_FRAMES_BUFFER_SIZE];
char frame_bufs[RECV_FRAMES_BUFFER_SIZE][BUFLEN];

// Keeping track of mbps
volatile static double max_mbps = START_MAX_MBPS;
volatile static double working_mbps = START_MAX_MBPS;
volatile static bool update_mbps = false;

// Multithreaded printf queue
#define MPRINTF_QUEUE_SIZE 100
#define MPRINTF_BUF_SIZE 1000
volatile static char mprintf_queue[MPRINTF_QUEUE_SIZE][MPRINTF_BUF_SIZE];
volatile static int mprintf_queue_index = 0;
volatile static int mprintf_queue_size = 0;

// Function Declarations
static void initVideo();
static void initAudio();
static int32_t ReceiveVideo(struct RTPPacket* packet, int recv_size);
static int32_t ReceiveAudio(struct RTPPacket* packet, int recv_size);
static void destroyVideo();
static void destroyAudio();

void initMultiThreadedPrintf();
void destroyMultiThreadedPrintf();
void MultiThreadedPrintf(void* opaque);
void mprintf(const char* fmtStr, ...);
SDL_Thread* mprintf_thread;
volatile static bool run_multithreaded_printf;

void initMultiThreadedPrintf() {
    run_multithreaded_printf = true;
    multithreadedprintf_mutex = SDL_CreateMutex();
    multithreadedprintf_semaphore = SDL_CreateSemaphore(0);
    mprintf_thread = SDL_CreateThread(MultiThreadedPrintf, "MultiThreadedPrintf", NULL);
}

void destroyMultiThreadedPrintf() {
    run_multithreaded_printf = false;
    for (int i = 0; i < 200; i++) {
        SDL_SemPost(multithreadedprintf_semaphore);
    }
    SDL_WaitThread(mprintf_thread, NULL);
    mprintf_thread = NULL;
}

void MultiThreadedPrintf(void* opaque) {
    while (true) {
        SDL_SemWait(multithreadedprintf_semaphore);

        if (!run_multithreaded_printf) {
            break;
        }

        char* buf;

        SDL_LockMutex(multithreadedprintf_mutex);
        buf = mprintf_queue[mprintf_queue_index];
        mprintf_queue_index++;
        mprintf_queue_index %= MPRINTF_QUEUE_SIZE;
        mprintf_queue_size--;
        SDL_UnlockMutex(multithreadedprintf_mutex);

        printf("%s", buf);
    }
}

void mprintf(const char* fmtStr, ...) {
    va_list args;
    va_start(args, fmtStr);

    SDL_LockMutex(multithreadedprintf_mutex);
    int index = (mprintf_queue_index + mprintf_queue_size) % MPRINTF_QUEUE_SIZE;
    char* buf = NULL;
    if (mprintf_queue_size < 98) {
        buf = &mprintf_queue[index];
        mprintf_queue_size++;
    }
    else if (mprintf_queue_size == 99) {
        strcpy(buf, "Buffer maxed out!!!\n");
        buf = &mprintf_queue[index];
        mprintf_queue_size++;
    }
    if (buf != NULL) {
        vsnprintf(buf, MPRINTF_BUF_SIZE, fmtStr, args);
        buf[MPRINTF_BUF_SIZE - 5] = '.';
        buf[MPRINTF_BUF_SIZE - 4] = '.';
        buf[MPRINTF_BUF_SIZE - 3] = '.';
        buf[MPRINTF_BUF_SIZE - 2] = '\n';
        buf[MPRINTF_BUF_SIZE - 1] = '\0';
        SDL_SemPost(multithreadedprintf_semaphore);
    }
    SDL_UnlockMutex(multithreadedprintf_mutex);

    va_end(args);
}

static int32_t RenderScreen(void* opaque) {
    printf("RenderScreen running on Thread %d\n", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    while (VideoData.run_render_screen_thread) {
        int ret = SDL_SemTryWait(VideoData.renderscreen_semaphore);
        if (ret == SDL_MUTEX_TIMEDOUT) {
            continue;
        }
        if (ret < 0) {
            printf("Semaphore Error\n");
            return -1;
        }

        //mprintf("Rendering ID %d\n", context.id);

        video_decoder_decode(renderContext.decoder, renderContext.prev_frame, renderContext.frame_size);

        AVPicture pict;
        pict.data[0] = renderContext.yPlane;
        pict.data[1] = renderContext.uPlane;
        pict.data[2] = renderContext.vPlane;
        pict.linesize[0] = OUTPUT_WIDTH;
        pict.linesize[1] = renderContext.uvPitch;
        pict.linesize[2] = renderContext.uvPitch;
        sws_scale(renderContext.sws, (uint8_t const* const*)renderContext.decoder->sw_frame->data,
            renderContext.decoder->sw_frame->linesize, 0, renderContext.decoder->context->height, pict.data,
            pict.linesize);

        SDL_UpdateYUVTexture(
            renderContext.Texture,
            NULL,
            renderContext.yPlane,
            OUTPUT_WIDTH,
            renderContext.uPlane,
            renderContext.uvPitch,
            renderContext.vPlane,
            renderContext.uvPitch
        );

        SDL_RenderClear(renderContext.Renderer);
        SDL_RenderCopy(renderContext.Renderer, renderContext.Texture, NULL, NULL);
        SDL_RenderPresent(renderContext.Renderer);

        rendering = false;
    }
}

static int32_t ReceivePackets(void* opaque) {
    printf("ReceivePackets running on Thread %d\n", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    struct RTPPacket packet = { 0 };
    struct SocketContext socketContext = *(struct SocketContext*) opaque;
    int slen = sizeof(socketContext.addr);
    SendAck(&socketContext, 1);

    initVideo();
    initAudio();

    for (int i = 0; !shutting_down; i++) {
        StartTimer(&VideoData.test_timer);
        int recv_size = recvfrom(socketContext.s, &packet, sizeof(packet), 0, (struct sockaddr*)(&socketContext.addr), &slen);
        VideoData.test_time += GetTimer(VideoData.test_timer);

        int packet_size = sizeof(packet) - sizeof(packet.data) + packet.payload_size;
        if (recv_size < 0) {
            int error = WSAGetLastError();
            switch (error) {
            case WSAETIMEDOUT:
                break;
            default:
                mprintf("Unexpected Error: %d\n", error);
                break;
            }
        }
        else if (packet.payload_size < 0 || packet_size != recv_size) {
            mprintf("Invalid packet size\nPayload Size: %d\nPacket Size: %d\nRecv_size: %d\n", packet_size, recv_size, packet.payload_size);
        }
        else {
            uint32_t hash = Hash((char*)&packet + sizeof(packet.hash), packet_size - sizeof(packet.hash));
            if (hash != packet.hash) {
                mprintf("Incorrect Hash\n");
            }
            else {
                switch (packet.type) {
                case PACKET_AUDIO:
                    ReceiveAudio(&packet, recv_size);
                    break;
                case PACKET_VIDEO:
                    ReceiveVideo(&packet, recv_size);
                    VideoData.test_time = 0.0;
                    break;
                case PACKET_MESSAGE:
                    ReceiveMessage(&packet, recv_size);
                    break;
                }
            }
        }

        if (i % 20 == 0) {
            SendAck(&socketContext, 1);
        }
    }
}

static void initVideo() {
    VideoData.pending_ctx = NULL;
    VideoData.frames_received = 0;
    VideoData.bytes_transferred = 0;
    StartTimer(&VideoData.frame_timer);
    VideoData.last_max_id = 1;
    VideoData.max_id = 1;

    VideoData.previous_id = -1;
    VideoData.previous_index = -1;

    for (int i = 0; i < RECV_FRAMES_BUFFER_SIZE; i++) {
        receiving_frames[i].id = -1;
    }

    VideoData.renderscreen_semaphore = SDL_CreateSemaphore(0);
    VideoData.render_screen_thread = SDL_CreateThread(RenderScreen, "RenderScreen", NULL);
    VideoData.run_render_screen_thread = true;
}

static void destroyVideo() {
    VideoData.run_render_screen_thread = false;
    SDL_WaitThread(VideoData.render_screen_thread, NULL);
    SDL_DestroySemaphore(VideoData.renderscreen_semaphore);
}

static int32_t ReceiveVideo(struct RTPPacket* packet, int recv_size) {
    // Get statistics from the last 3 seconds of data
    if (GetTimer(VideoData.frame_timer) > 3) {
        double time = GetTimer(VideoData.frame_timer);

        // Calculate statistics
        int expected_frames = VideoData.max_id - VideoData.last_max_id;
        double fps = 1.0 * expected_frames / time;
        double mbps = VideoData.bytes_transferred * 8.0 / 1024.0 / 1024.0 / time;
        double receive_rate = expected_frames == 0 ? 1.0 : 1.0 * VideoData.frames_received / expected_frames;
        double dropped_rate = 1.0 - receive_rate;

        // Print statistics

        for (int i = VideoData.last_max_id + 1; i <= VideoData.max_id; i++) {
            int index = i % RECV_FRAMES_BUFFER_SIZE;
            if (receiving_frames[index].id == i) {
                mprintf("Frame with ID %d: %d/%d\n", i, receiving_frames[index].packets_received, receiving_frames[index].num_packets);
            }
        }

        mprintf("FPS: %f\nmbps: %f\ndropped: %f%%\n\n", fps, mbps, 100.0 * dropped_rate);

        // Adjust mbps based on dropped packets
        if (dropped_rate > 0.4) {
            max_mbps = max_mbps * 0.75;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.2) {
            max_mbps = max_mbps * 0.8;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.1) {
            max_mbps = max_mbps * 0.85;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.05) {
            max_mbps = max_mbps * 0.9;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.00) {
            max_mbps = max_mbps * 0.95;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate == 0.00) {
            working_mbps = max(max_mbps * 1.05, working_mbps);
            max_mbps = (max_mbps + working_mbps) / 2.0;
            update_mbps = true;
        }

        StartTimer(&VideoData.frame_timer);
        VideoData.bytes_transferred = 0;
        VideoData.frames_received = 0;
        VideoData.last_max_id = VideoData.max_id;
    }

    if (!rendering) {
        if (VideoData.pending_ctx != NULL) {
            renderContext = *VideoData.pending_ctx;
            rendering = true;

            SDL_SemPost(VideoData.renderscreen_semaphore);

            VideoData.pending_ctx = NULL;
        }
    }

    if (VideoData.previous_id != -1) {
        //mprintf("Took %f to compute packet for ID %d and Index %d | Test %f\n", GetTimer(VideoData.packet_timer), VideoData.previous_id, VideoData.previous_index, VideoData.test_time);
    }

    VideoData.previous_id = packet->id;
    VideoData.previous_index = packet->index;
    StartTimer(&VideoData.packet_timer);

    //mprintf("Packet ID %d, Packet Index %d, Hash %x\n", packet->id, packet->index, packet->hash);

    // Find frame in linked list that matches the id
    if (recv_size > 0) {
        VideoData.bytes_transferred += recv_size;

        int index = packet->id % RECV_FRAMES_BUFFER_SIZE;

        struct SDLVideoContext* ctx = &receiving_frames[index];
        if (ctx->id != packet->id) {
            if (rendering && renderContext.id == ctx->id) {
                mprintf("Error! Currently rendering an ID that will be overwritten! Skipping packet.\n");
                return 0;
            }
            *ctx = videoContext;
            ctx->id = packet->id;
            ctx->prev_frame = &frame_bufs[index];
            ctx->packets_received = 0;
            ctx->num_packets = -1;
            memset(ctx->received_indicies, 0, sizeof(ctx->received_indicies));
        }

        if (ctx->received_indicies[packet->index]) {
            return 0;
        }

        ctx->received_indicies[packet->index] = true;
        ctx->packets_received++;

        // Copy packet data
        int place = packet->index * MAX_PACKET_SIZE;
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
                //printf("Received all packets for id %d, getting ready to render\n", packet.id);

                VideoData.pending_ctx = ctx;
            }

        }
    }

    return 0;
}

struct AudioData {
    SDL_AudioDeviceID dev;
    audio_decoder_t* audio_decoder;
    int last_played_id;
} volatile static AudioData;

static void initAudio() {
    // cast socket and SDL variables back to their data type for usage
    SDL_AudioSpec wantedSpec = { 0 }, audioSpec = { 0 };

    AudioData.audio_decoder = create_audio_decoder();

    SDL_zero(wantedSpec);
    SDL_zero(audioSpec);
    wantedSpec.channels = AudioData.audio_decoder->context->channels;
    wantedSpec.freq = AudioData.audio_decoder->context->sample_rate;
    wantedSpec.format = AUDIO_F32SYS;
    wantedSpec.silence = 0;
    wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;

    AudioData.dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (AudioData.dev == 0) {
        printf("Failed to open audio\n");
        exit(1);
    }

    SDL_PauseAudioDevice(AudioData.dev, 0);

    AudioData.last_played_id = -1;
}

static void destroyAudio() {
    SDL_CloseAudioDevice(AudioData.dev);
    destroy_audio_decoder(AudioData.audio_decoder);
}

static int32_t ReceiveAudio(struct RTPPacket* packet, int recv_size) {
    int played_id = packet->id * 1000 + packet->index;
    if (played_id > AudioData.last_played_id) {
        AudioData.last_played_id = played_id;
        SDL_QueueAudio(AudioData.dev, packet->data, packet->payload_size);
    }

    return 0;
}

static int32_t ReceiveMessage(struct RTPPacket* packet, int recv_size) {
    FractalServerMessage fmsg = *(FractalServerMessage*)packet->data;
    switch (fmsg.type) {
    case MESSAGE_PONG:
        mprintf("Latency: %f\n", GetTimer(latency_timer));
        is_timing_latency = false;
        break;
    default:
        mprintf("Unknown Server Message Received\n");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    initMultiThreadedPrintf();

    // initialize the windows socket library if this is a windows client
#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        return -1;
    }
#endif

    SDL_Event msg;
    SDL_Window* screen;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    Uint8* yPlane, * uPlane, * vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;
    FractalClientMessage fmsg = { 0 };

    char* ip = "52.186.125.178";

    struct SocketContext PacketSendContext = { 0 };
    if (CreateUDPContext(&PacketSendContext, "C", ip, 1) < 0) {
        exit(1);
    }

    struct SocketContext PacketReceiveContext = { 0 };
    if (CreateUDPContext(&PacketReceiveContext, "C", ip, 1) < 0) {
        exit(1);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    screen = SDL_CreateWindow(
        "Fractal",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        OUTPUT_WIDTH,
        OUTPUT_HEIGHT,
        0
    );

    if (!screen) {
        fprintf(stderr, "SDL: could not create window - exiting\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(screen, -1, 0);
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

    struct SwsContext* sws_ctx = NULL;
    sws_ctx = sws_getContext(CAPTURE_WIDTH, CAPTURE_HEIGHT,
        AV_PIX_FMT_YUV420P, OUTPUT_WIDTH, OUTPUT_HEIGHT,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL);

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

    videoContext.yPlane = yPlane;
    videoContext.sws = sws_ctx;
    videoContext.uPlane = uPlane;
    videoContext.vPlane = vPlane;
    videoContext.uvPitch = uvPitch;
    videoContext.Renderer = renderer;
    videoContext.Texture = texture;
    videoContext.decoder = create_video_decoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, OUTPUT_WIDTH, OUTPUT_HEIGHT, OUTPUT_WIDTH * 12000, DECODE_TYPE);

    printf("Receiving\n\n");

    SDL_Thread* receive_packets_thread = SDL_CreateThread(ReceivePackets, "ReceivePackets", &PacketReceiveContext);

    StartTimer(&latency_timer);
    while (!shutting_down)
    {
        if (update_mbps) {
            mprintf("Updating MBPS: %f\n", max_mbps);
            update_mbps = false;
            memset(&fmsg, 0, sizeof(fmsg));
            fmsg.type = MESSAGE_MBPS;
            fmsg.mbps = max_mbps;
            sendto(PacketSendContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&PacketSendContext.addr), sizeof(PacketSendContext.addr));
        }

        if (!is_timing_latency && GetTimer(latency_timer) > 2) {
            memset(&fmsg, 0, sizeof(fmsg));
            fmsg.type = MESSAGE_PING;
            is_timing_latency = true;
            StartTimer(&latency_timer);
            sendto(PacketSendContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&PacketSendContext.addr), sizeof(PacketSendContext.addr));
        }

        memset(&fmsg, 0, sizeof(fmsg));
        if (SDL_PollEvent(&msg)) {
            switch (msg.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                fmsg.type = MESSAGE_KEYBOARD;
                fmsg.keyboard.code = (FractalKeycode)msg.key.keysym.scancode;
                fmsg.keyboard.mod = msg.key.keysym.mod;
                fmsg.keyboard.pressed = msg.key.type == SDL_KEYDOWN;
                break;
            case SDL_MOUSEMOTION:
                fmsg.type = MESSAGE_MOUSE_MOTION;
                fmsg.mouseMotion.x = msg.motion.x * CAPTURE_WIDTH / OUTPUT_WIDTH;
                fmsg.mouseMotion.y = msg.motion.y * CAPTURE_HEIGHT / OUTPUT_HEIGHT;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                fmsg.type = MESSAGE_MOUSE_BUTTON;
                fmsg.mouseButton.button = msg.button.button;
                fmsg.mouseButton.pressed = msg.button.type == SDL_MOUSEBUTTONDOWN;
                break;
            case SDL_MOUSEWHEEL:
                fmsg.type = MESSAGE_MOUSE_WHEEL;
                fmsg.mouseWheel.x = msg.wheel.x;
                fmsg.mouseWheel.y = msg.wheel.y;
                break;
            case SDL_QUIT:
                printf("Quitting...\n");
                fmsg.type = MESSAGE_QUIT;
                shutting_down = true;
                break;
            }
            if (fmsg.type != 0) {
                sendto(PacketSendContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&PacketSendContext.addr), sizeof(PacketSendContext.addr));
            }
        }
    }

    destroyVideo();
    destroyAudio();
    destroyMultiThreadedPrintf();
    SDL_WaitThread(receive_packets_thread, NULL);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();
#if defined(_WIN32)
    closesocket(PacketSendContext.s);
    closesocket(PacketReceiveContext.s);
    WSACleanup();
#else
    close(PacketSendContext.s);
    close(PacketReceiveContext.s);
#endif
    printf("Closing Client...\n");

    return 0;
}
