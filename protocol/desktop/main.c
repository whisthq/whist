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
#define MAX_PACKET_SIZE 1400

struct SDLVideoContext {
    Uint8 *yPlane;
    Uint8 *uPlane;
    Uint8 *vPlane;
    int uvPitch;
    int frame_size;
    SDL_Renderer* Renderer;
    SDL_Texture* Texture;
    decoder_t *decoder;
    char *prev_frame;
    struct SocketContext socketContext;
    struct SwsContext* sws;
    int id;
    int packets_received;
    int num_packets;
};

SDL_sem* renderscreen_semaphore;
SDL_sem* multithreadedprintf_semaphore;
SDL_mutex* multithreadedprintf_mutex;
volatile static bool rendering = false;
volatile static struct SDLVideoContext* renderContext;

static double max_mbps = 100.0;
static double working_mbps = 100.0;
volatile static bool update_mbps = false;

volatile static char* queue[100];
volatile static int size = 0;

LARGE_INTEGER start, end, frequency;
double interval;

void MultiThreadedPrintf(void* opaque) {
    while (true) {
        SDL_SemWait(multithreadedprintf_semaphore);

        char* buf;
        SDL_LockMutex(multithreadedprintf_mutex);
        buf = queue[0];
        for (int i = 0; i < size; i++) {
            queue[i] = queue[i + 1];
        }
        size--;
        SDL_UnlockMutex(multithreadedprintf_mutex);
        printf("%s", buf);

        free(buf);
    }
}

void mprintf(const char* fmtStr, ...) {
    va_list args;
    va_start(args, fmtStr);

    const int buf_len = 1000;

    char* buf = malloc(buf_len);
    vsnprintf(buf, buf_len, fmtStr, args);
    buf[buf_len - 5] = '.';
    buf[buf_len - 4] = '.';
    buf[buf_len - 3] = '.';
    buf[buf_len - 2] = '\n';
    buf[buf_len - 1] = '\0';

    va_end(args);

    SDL_LockMutex(multithreadedprintf_mutex);
    if (size < 98) {
        queue[size++] = buf;
        SDL_SemPost(multithreadedprintf_semaphore);
    }
    else if (size == 99) {
        strcpy(buf, "Buffer maxed out!!!\n");
        queue[size++] = buf;
        SDL_SemPost(multithreadedprintf_semaphore);
    }
    else {
        free(buf);
    }
    SDL_UnlockMutex(multithreadedprintf_mutex);
}

static int32_t RenderScreen(void *opaque) {
    while (true) {
        SDL_SemWait(renderscreen_semaphore);
        struct SDLVideoContext context = *renderContext;
        QueryPerformanceCounter(&start);
        QueryPerformanceFrequency(&frequency);
        video_decoder_decode(context.decoder, context.prev_frame, context.frame_size);

        AVPicture pict;
        pict.data[0] = context.yPlane;
        pict.data[1] = context.uPlane;
        pict.data[2] = context.vPlane;
        pict.linesize[0] = OUTPUT_WIDTH;
        pict.linesize[1] = context.uvPitch;
        pict.linesize[2] = context.uvPitch;
        sws_scale(context.sws, (uint8_t const* const*)context.decoder->sw_frame->data,
            context.decoder->sw_frame->linesize, 0, context.decoder->context->height, pict.data,
            pict.linesize);

        SDL_UpdateYUVTexture(
            context.Texture,
            NULL,
            context.yPlane,
            OUTPUT_WIDTH,
            context.uPlane,
            context.uvPitch,
            context.vPlane,
            context.uvPitch
        );

        SDL_RenderClear(context.Renderer);
        SDL_RenderCopy(context.Renderer, context.Texture, NULL, NULL);
        SDL_RenderPresent(context.Renderer);

        renderContext = NULL;
        rendering = false;
        QueryPerformanceCounter(&end);
        interval = (double) (end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
        mprintf("Decode time is %f ms\n", interval);
    }
}

static int32_t ReceiveVideo(void *opaque) {
    struct SDLVideoContext context = *(struct SDLVideoContext *) opaque;
    int recv_size, slen = sizeof(context.socketContext.addr), recv_index = 0, i = 0, intercepts = 0;
    char recv_buf[MAX_PACKET_SIZE];

    struct RTPPacket packet = {0};
    struct SDLVideoContext* pending_ctx = NULL;
    struct SDLVideoContext* rendered_ctx = NULL;

    context.decoder = create_video_decoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, OUTPUT_WIDTH, OUTPUT_HEIGHT, OUTPUT_WIDTH * 12000);

    if (SendAck(&context.socketContext, 5) < 0)
        printf("Could not send video ACK\n");

    renderscreen_semaphore = SDL_CreateSemaphore(0);
    SDL_CreateThread(RenderScreen, "RenderScreen", NULL);

    struct gll_t* frame_list;
    frame_list = gll_init();

    int frames_received = 0;
    int bytes_transferred = 0;
    clock frameTimer;
    StartTimer(&frameTimer);
    int last_max_id = 1;
    int max_id = 1;

    while(1)
    {
        if (GetTimer(frameTimer) > 1.5) {
            double time = GetTimer(frameTimer);
            int expected_frames = max_id - last_max_id;

            double fps = 1.0 * expected_frames / time;
            double mbps = bytes_transferred * 8.0 / 1024.0 / 1024.0 / time;
            double receive_rate = expected_frames == 0 ? 1.0 : 1.0 * frames_received / expected_frames;
            double dropped_rate = 1.0 - receive_rate;

            mprintf("FPS: %f\nmbps: %f\ndropped: %f%%\n\n", fps, mbps, 100.0 * dropped_rate);

            if (dropped_rate > 0.05) {
                max_mbps = 0.95 * min(mbps, max_mbps);
                working_mbps = max_mbps;
                update_mbps = true;
            }

            if (dropped_rate == 0.00) {
                working_mbps = max(mbps, working_mbps);
                max_mbps = max_mbps + (working_mbps - max_mbps) * 0.05;
                update_mbps = true;
            }

            StartTimer(&frameTimer);
            bytes_transferred = 0;
            frames_received = 0;
            last_max_id = max_id;
        }

        if (!rendering) {
          if (rendered_ctx != NULL) {
            free(rendered_ctx->prev_frame);
            free(rendered_ctx);
            rendered_ctx = NULL;
          }
          if (pending_ctx != NULL) {
            renderContext = pending_ctx;
            rendering = true;

            SDL_SemPost(renderscreen_semaphore);

            rendered_ctx = pending_ctx;
            pending_ctx = NULL;
          }
        }

        recv_size = recvfrom(context.socketContext.s, &packet, sizeof(packet), 0, (struct sockaddr*)(&context.socketContext.addr), &slen);

        // Find frame in linked list that matches the id
        if(recv_size > 0 && packet.id > max_id - 4) {
            if (Hash(packet.data, min(packet.payload_size, recv_size)) != packet.hash) {
                mprintf("Incorrect Hash\n");
                continue;
            }

            bytes_transferred += recv_size;
            if (packet.id > max_id) {
                max_id = packet.id;
            }

          struct SDLVideoContext* gllctx = NULL;
          int gllindex = -1;
          
          struct gll_node_t *node = frame_list->first;
          for(int i = 0; i < frame_list->size; i++) {
            struct SDLVideoContext* ctx = node->data;
            if (ctx->id == packet.id) {
              gllctx = ctx;
              gllindex = i;
              break;
            }
            node = node->next;
          }

          // Could not find frame in linked list, add a new node to the linked list
          if (gllctx == NULL) {
              struct SDLVideoContext* threadContext = malloc(sizeof(struct SDLVideoContext));
              memcpy(threadContext, &context, sizeof(struct SDLVideoContext));
              threadContext->id = packet.id;
              threadContext->prev_frame = malloc(sizeof(char) * BUFLEN);
              threadContext->packets_received = 0;
              threadContext->num_packets = -1;
              gll_push_end(frame_list, threadContext);
              gllctx = threadContext;
              gllindex = frame_list->size - 1;
          }

          gllctx->packets_received++;

          // Copy packet data
          int place = packet.index * MAX_PACKET_SIZE;
          memcpy(gllctx->prev_frame + place, packet.data, packet.payload_size);
          gllctx->frame_size += packet.payload_size;

          // Keep track of how many packets are necessary
          if (packet.is_ending) {
            gllctx->num_packets = packet.index + 1;
          }

          // If we received all of the packets
          if (gllctx->packets_received == gllctx->num_packets) {
            frames_received++;

            //printf("Received all packets for id %d, getting ready to render\n", packet.id);
            gll_remove(frame_list, gllindex);

            // Wipe out the out of date linked list
            int keepers = 0;
            while (frame_list->size > keepers) {
              struct SDLVideoContext *linkedlistctx = gll_find_node(frame_list, keepers)->data;

              if (linkedlistctx->id < gllctx->id) {
                free(linkedlistctx->prev_frame);
                free(linkedlistctx);
                gll_remove(frame_list, keepers);
              } else {
                keepers++;
              }
            }

            if (pending_ctx != NULL) {
              free(pending_ctx->prev_frame);
              free(pending_ctx);
            }

            pending_ctx = gllctx;
          }

          if (frame_list->size > 10) {
            mprintf("Too many frames!\n");
            int keepers = 0;
            // Wipe Array
            while(frame_list->size > keepers) {
              struct SDLVideoContext *linkedlistctx = gll_find_node(frame_list, keepers)->data;
              if (linkedlistctx->id <= max_id - 6) {
                  free(linkedlistctx->prev_frame);
                  free(linkedlistctx);
                  gll_remove(frame_list, keepers);
              }
              else {
                  keepers++;
              }
            }
          }
        }   

        if (i == 30 * 60) {
            i = 0;
            SendAck(&context.socketContext, 1);
        }
        i++;
    }

    return 0;
}

static int32_t ReceiveAudio(void *opaque) {
    // cast socket and SDL variables back to their data type for usage
    struct SocketContext* context = (struct SocketContext *) opaque;
    int recv_size, slen = sizeof(context->addr), recv_index = 0, i = 0, curr_id = -1, sample_size = 0;
    uint8_t recv_sample[10000];
    struct RTPPacket packet = {0};

    SDL_AudioSpec wantedSpec = { 0 }, audioSpec = { 0 };
    SDL_AudioDeviceID dev;

    audio_decoder_t* audio_decoder;
    audio_decoder = create_audio_decoder();

    SDL_zero(wantedSpec);
    SDL_zero(audioSpec);
    wantedSpec.channels = audio_decoder->context->channels;
    wantedSpec.freq = audio_decoder->context->sample_rate;
    wantedSpec.format = AUDIO_F32SYS;
    wantedSpec.silence = 0;
    wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;

//    dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if(dev == 0) {
        printf("Failed to open audio\n");
        exit(1);
    }

    SDL_PauseAudioDevice(dev, 0);

    if (SendAck(context, 5) < 0)
        printf("Could not send audio ACK\n");

    while(1) {
        if ((recv_size = recvfrom(context->s, &packet, sizeof(packet), 0, (struct sockaddr*)(&context->addr), &slen)) > 0) {
          SDL_QueueAudio(dev, packet.data, packet.payload_size);
        }
        if (i == 30 * 60) {
          i = 0;
          SendAck(context, 1);
        }
        i++;
    }

    SDL_CloseAudioDevice(dev);
    destroy_audio_decoder(audio_decoder);
    return 0;
}

int main(int argc, char* argv[])
{
    multithreadedprintf_mutex = SDL_CreateMutex();
    multithreadedprintf_semaphore = SDL_CreateSemaphore(0);
    SDL_CreateThread(MultiThreadedPrintf, "MultiThreadedPrintf", NULL);

    // initialize the windows socket library if this is a windows client
#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        return -1;
    }
#endif

    SDL_Event msg;
    SDL_Window *screen;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch, slen, repeat = 1;
    struct SDLVideoContext SDLVideoContext = {0};
    FractalMessage fmsg = {0};

    char* ip = "40.117.176.26";

    struct SocketContext InputContext = {0};
    if(CreateUDPContext(&InputContext, "C", ip, -1) < 0) {
        exit(1);
    }

    struct SocketContext VideoReceiveContext = {0};
    if(CreateUDPContext(&VideoReceiveContext, "C", ip, -1) < 0) {
        exit(1);
    }

    struct SocketContext AudioReceiveContext = {0};
    if(CreateUDPContext(&AudioReceiveContext, "C", ip, -1) < 0) {
        exit(1);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
      fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
      exit(1);
    }

    slen = sizeof(InputContext.addr);

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

    struct SwsContext *sws_ctx = NULL;
    sws_ctx = sws_getContext(CAPTURE_WIDTH, CAPTURE_HEIGHT,
          AV_PIX_FMT_NV12, OUTPUT_WIDTH, OUTPUT_HEIGHT,
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

    SDLVideoContext.yPlane = yPlane;
    SDLVideoContext.sws = sws_ctx;
    SDLVideoContext.uPlane = uPlane;
    SDLVideoContext.vPlane = vPlane;
    SDLVideoContext.uvPitch = uvPitch;
    SDLVideoContext.Renderer = renderer;
    SDLVideoContext.Texture = texture;
    SDLVideoContext.socketContext = VideoReceiveContext;

    printf("Receiving\n\n");

    SDL_Thread *receive_video = SDL_CreateThread(ReceiveVideo, "ReceiveVideo", &SDLVideoContext);
    SDL_Thread *receive_audio = SDL_CreateThread(ReceiveAudio, "ReceiveAudio", &AudioReceiveContext);

    while (repeat)
    {
        if (update_mbps) {
            mprintf("Updating MBPS\n");
            update_mbps = false;
            fmsg.type = MESSAGE_MBPS;
            fmsg.mbps = max_mbps < 20.0 ? 20.0 : max_mbps;
            sendto(InputContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&InputContext.addr), slen);
            memset(&fmsg, 0, sizeof(fmsg));
        }
        if(SDL_WaitEventTimeout(&msg, 10)) {
            switch (msg.type) {
              case SDL_KEYDOWN:
              case SDL_KEYUP:
                fmsg.type = MESSAGE_KEYBOARD;
                fmsg.keyboard.code = (FractalKeycode) msg.key.keysym.scancode;
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
                fmsg.type = MESSAGE_QUIT;
                sendto(InputContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&InputContext.addr), slen);
                repeat = 0;
                SDL_DestroyTexture(texture);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(screen);
                SDL_Quit();
                break;
            }
        }
        if (fmsg.type != 0) {
          sendto(InputContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&InputContext.addr), slen);
          memset(&fmsg, 0, sizeof(fmsg));
        }
    }

#if defined(_WIN32)
    closesocket(InputContext.s);
    closesocket(VideoReceiveContext.s);
    closesocket(AudioReceiveContext.s);
    WSACleanup();
#else
    close(InputContext.s);
    close(VideoReceiveContext.s);
    close(AudioReceiveContext.s);
#endif

    return 0;
}
