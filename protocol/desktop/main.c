#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

#include "../include/fractal.h"
#include "../include/webserver.h" // header file for webserver query functions
#include "../include/videodecode.h" // header file for audio decoder
#include "../include/audiodecode.h" // header file for audio decoder

#define SDL_MAIN_HANDLED
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 500000
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_PACKET_SIZE 1400

/// BEGIN TIME
LARGE_INTEGER frequency;        // ticks per second
LARGE_INTEGER t1, t2;           // ticks

void StartCounter()
{
    if(!QueryPerformanceFrequency(&frequency))
      printf("QueryPerformanceFrequency failed!\n");

    QueryPerformanceCounter(&t1);
}
double GetCounter()
{
    QueryPerformanceCounter(&t2);
    return (t2.QuadPart-t1.QuadPart) * 1000.0/frequency.QuadPart;
}
/// END TIME


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
};

struct RTPPacket {
  uint8_t data[MAX_PACKET_SIZE];
  int index;
  int payload_size;
  int id;
};

struct node {
  struct SDLVideoContext context;
  struct node* next;
};

static int sillymutex = 0;

static int32_t RenderScreen(void *opaque) {
  struct SDLVideoContext context = *(struct SDLVideoContext *) opaque;
  video_decoder_decode(context.decoder, context.prev_frame, context.frame_size);

  AVPicture pict;
  pict.data[0] = context.yPlane;
  pict.data[1] = context.uPlane;
  pict.data[2] = context.vPlane;
  pict.linesize[0] = OUTPUT_WIDTH;
  pict.linesize[1] = context.uvPitch;
  pict.linesize[2] = context.uvPitch;
  sws_scale(context.sws, (uint8_t const * const *) context.decoder->frame->data,
       context.decoder->frame->linesize, 0, context.decoder->context->height, pict.data,
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

  free(opaque);
  sillymutex = 0;
  return 0;
}

static int32_t ReceiveVideo(void *opaque) {
    struct SDLVideoContext context = *(struct SDLVideoContext *) opaque;
    int recv_size, slen = sizeof(context.socketContext.addr), recv_index = 0, i = 0;
    char recv_buf[MAX_PACKET_SIZE];

    context.prev_frame = malloc(sizeof(char) * BUFLEN);
    memset(context.prev_frame, 0, sizeof(char) * BUFLEN);

    uint8_t tracker = 0;
    int current_id = -1;

    int max_index = 0, curr_index = 0;
    struct RTPPacket packet = {0};
    // init decoder
    context.decoder = create_video_decoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, OUTPUT_WIDTH, OUTPUT_HEIGHT, OUTPUT_WIDTH * 12000);

    if (SendAck(&context.socketContext, 5) < 0)
        printf("Could not send video ACK\n");


    while(1)
    {
        recv_size = recvfrom(context.socketContext.s, &packet, sizeof(packet), 0, (struct sockaddr*)(&context.socketContext.addr), &slen);

        if (current_id == -1) {
          current_id = packet.id;
        }

        StartCounter();
        bool final = false;

        if(recv_size > 0) {
            if (current_id == packet.id) {
                tracker++;

                int place = packet.index * MAX_PACKET_SIZE;
                memcpy(context.prev_frame + place, packet.data, packet.payload_size);
                if(packet.payload_size != MAX_PACKET_SIZE) {
                    if (packet.index + 1 == tracker) {                  
                        final = true;
                        context.frame_size = place + packet.payload_size;

                        while(sillymutex != 0);
                        sillymutex = 1;

                        struct SDLVideoContext* threadContext = malloc(sizeof(struct SDLVideoContext));
                        memcpy(threadContext, &context, sizeof(struct SDLVideoContext));

                        SDL_Thread *render_screen = SDL_CreateThread(RenderScreen, "RenderScreen", threadContext);
                        struct SDLVideoContext context = *(struct SDLVideoContext *) opaque;

                        tracker = 0;
                        current_id = -1;
                        //printf("Rendered!\n");
                    } else {
                        printf("Missed a packet: Only got %d, but expected %d\n", tracker, packet.index + 1);
                    }
                }
            } else {
              printf("Intercepted\n");
              tracker = 0;
              current_id = -1;

              current_id = packet.id;
              tracker++;

              int place = packet.index * MAX_PACKET_SIZE;
              memcpy(context.prev_frame + place, packet.data, packet.payload_size);
            }
        }

        if (final) {
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
    int recv_size, slen = sizeof(context->addr), recv_index = 0, i = 0;
    char recv_buf[BUFLEN];

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
        if ((recv_size = recvfrom(context->s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context->addr), &slen)) > 0) {
            SDL_QueueAudio(dev, recv_buf, recv_size);
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
    // initialize the windows socket library if this is a windows client
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        return -1;
    }

    SDL_Event msg;
    SDL_Window *screen;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch, slen, repeat = 1;
    struct SDLVideoContext SDLVideoContext = {0};
    FractalMessage fmsg = {0};

    struct SocketContext InputContext = {0};
    if(CreateUDPContext(&InputContext, "C", "168.61.54.231", -1) < 0) {
        exit(1);
    }

    struct SocketContext VideoReceiveContext = {0};
    if(CreateUDPContext(&VideoReceiveContext, "C", "168.61.54.231", -1) < 0) {
        exit(1);
    }

    struct SocketContext AudioReceiveContext = {0};
    if(CreateUDPContext(&AudioReceiveContext, "C", "168.61.54.231", -1) < 0) {
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

    SDLVideoContext.yPlane = yPlane;
    SDLVideoContext.sws = sws_ctx;
    SDLVideoContext.uPlane = uPlane;
    SDLVideoContext.vPlane = vPlane;
    SDLVideoContext.uvPitch = uvPitch;
    SDLVideoContext.Renderer = renderer;
    SDLVideoContext.Texture = texture;
    SDLVideoContext.socketContext = VideoReceiveContext;

    SDL_Thread *receive_video = SDL_CreateThread(ReceiveVideo, "ReceiveVideo", &SDLVideoContext);
    SDL_Thread *receive_audio = SDL_CreateThread(ReceiveAudio, "ReceiveAudio", &AudioReceiveContext);

    while (repeat)
    {
        if(SDL_WaitEvent(&msg)) {
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
        }
        memset(&fmsg, 0, sizeof(fmsg));
    }

    closesocket(InputContext.s);
    closesocket(VideoReceiveContext.s);
    closesocket(AudioReceiveContext.s);
    WSACleanup();

    return 0;
}
