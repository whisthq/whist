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
#include "../include/linkedlist.h" // header file for audio decoder

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
    int id;
    int packets_received;
    int num_packets;
};

struct RTPPacket {
  uint8_t data[MAX_PACKET_SIZE];
  int index;
  int payload_size;
  int id;
  bool is_ending;
};

struct gll_t *root;

volatile static int sillymutex = 0;

uint32_t Hash(char *key, size_t len)
{
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static int32_t RenderScreen(void *opaque) {
  struct SDLVideoContext context = *(struct SDLVideoContext *) opaque;
  //printf("Render: %d\n", context.id);

  //printf("Id: %d\nSize: %d\nHash: %d\n\n", context.id, context.frame_size, Hash(context.prev_frame, context.frame_size));

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

  sillymutex = 0;

  return 0;
}

static int32_t ReceiveVideo(void *opaque) {
    root = gll_init();

    struct SDLVideoContext context = *(struct SDLVideoContext *) opaque;
    int recv_size, slen = sizeof(context.socketContext.addr), recv_index = 0, i = 0, intercepts = 0;
    char recv_buf[MAX_PACKET_SIZE];

    struct RTPPacket packet = {0};
    // init decoder
    context.decoder = create_video_decoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, OUTPUT_WIDTH, OUTPUT_HEIGHT, OUTPUT_WIDTH * 12000);

    if (SendAck(&context.socketContext, 5) < 0)
        printf("Could not send video ACK\n");

    struct SDLVideoContext* pending_ctx = NULL;
    struct SDLVideoContext* rendered_ctx = NULL;

    while(1)
    {
        if (sillymutex == 0) {
          if (rendered_ctx != NULL) {
            free(rendered_ctx->prev_frame);
            free(rendered_ctx);
            rendered_ctx = NULL;
          }
          if (pending_ctx != NULL) {
            //printf("Render: %d\n", pending_ctx->id);
            sillymutex = 1;
            SDL_Thread *render_screen = SDL_CreateThread(RenderScreen, "RenderScreen", pending_ctx);
            rendered_ctx = pending_ctx;
            pending_ctx = NULL;
          }
        }

        recv_size = recvfrom(context.socketContext.s, &packet, sizeof(packet), 0, (struct sockaddr*)(&context.socketContext.addr), &slen);

        // Find frame in linked list that matches the id
        if(recv_size > 0) {
          //printf("ID: %d, Index: %d\n", packet.id, packet.index);

          struct SDLVideoContext* gllctx = NULL;
          int gllindex = -1;
          
          //printf("Start For Loop %d\n", root->size);
          struct gll_node_t *node = root->first;
          for(int i = 0; i < root->size; i++) {
            struct SDLVideoContext* ctx = node->data;
            if (ctx->id == packet.id) {
              //printf("Context found for id %d\n", packet.id);
              gllctx = ctx;
              gllindex = i;
              break;
            }
            node = node->next;
          }
          //printf("Done with for loop\n");
          

          // Could not find frame in linked list, add a new node to the linked list
          if (gllctx == NULL) {
              //printf("Could not find context for id %d\n", packet.id);
              struct SDLVideoContext* threadContext = malloc(sizeof(struct SDLVideoContext));
              memcpy(threadContext, &context, sizeof(struct SDLVideoContext));
              threadContext->id = packet.id;
              threadContext->prev_frame = malloc(sizeof(char) * BUFLEN);
              threadContext->packets_received = 0;
              threadContext->num_packets = -1;
              gll_push_end(root, threadContext);
              gllctx = threadContext;
              gllindex = root->size - 1;
          }

          gllctx->packets_received++;

          // Copy packet data
          int place = packet.index * MAX_PACKET_SIZE;
          memcpy(gllctx->prev_frame + place, packet.data, packet.payload_size);
          gllctx->frame_size += packet.payload_size;

          // Keep track of how many packets are necessary
          if (packet.is_ending) {
            gllctx->num_packets = packet.index + 1;
            //printf("Packet with id %d is ending, with %d packets\n", packet.id, gllctx->num_packets);
          }

          // If we received all of the packets
          if (gllctx->packets_received == gllctx->num_packets) {
            //printf("Received all packets for id %d, getting ready to render\n", packet.id);
            gll_remove(root, gllindex);

            //printf("ID: %d\n", gllctx->id);

            // Wipe out the out of date linked list
            int keepers = 0;
            while (root->size > keepers) {
              //printf("Root ID: %d\n", ((struct SDLVideoContext*)(root->first->data))->id);
              //printf("Keepers: %d, Size: %d\n", keepers, root->size);
              struct SDLVideoContext *linkedlistctx = gll_find_node(root, keepers)->data;

              //printf("Root First: %p\n", root->first);
              //printf("find_node(0): %p\n", gll_find_node(root, 0));

              //printf("ll ID %d\n", linkedlistctx->id);
              if (linkedlistctx->id < gllctx->id) {
                //printf("Remove\n");
                //printf("Removing old frame with id %d, in preference of completed frame id %d\n", linkedlistctx->id, gllctx->id);
                //printf("Free :%d\n", linkedlistctx->id);
                free(linkedlistctx->prev_frame);
                free(linkedlistctx);
                gll_remove(root, keepers);
              } else {
                //printf("Keep\n");
                keepers++;
              }

              //printf("Done Recvd %d\n", );
            }

            //printf("Done While\n");
            if (pending_ctx != NULL) {
              //printf("Free: %d\n", pending_ctx->id);
              free(pending_ctx->prev_frame);
              free(pending_ctx);
            }

            pending_ctx = gllctx;
            //printf("Pending: %d\n", pending_ctx->id);
          }

          if (root->size > 10) {
            printf("Too many frames!\n");

            // Wipe Array
            while(root->size > 0) {
              struct SDLVideoContext *linkedlistctx = gll_find_node(root, 0)->data;
              free(linkedlistctx->prev_frame);
              free(linkedlistctx);
              gll_remove(root, 0);
            }
          }

          //printf("\n");
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
    struct RTPPacket recv_buf = {0};

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
            SDL_QueueAudio(dev, recv_buf.data, recv_buf.payload_size);
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

    char* ip = "40.121.132.26";

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
