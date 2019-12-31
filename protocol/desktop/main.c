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

#define BUFLEN 160000
#define SDL_AUDIO_BUFFER_SIZE 1024;

struct SDLVideoContext {
    Uint8 *yPlane;
    Uint8 *uPlane;
    Uint8 *vPlane;
    int uvPitch;
    SDL_Renderer* Renderer;
    SDL_Texture* Texture;
    struct SocketContext socketContext;
    struct SwsContext* sws;
};

static int32_t SendUserInput(void *opaque) {
    struct SocketContext context = *(struct SocketContext *) opaque;
    int i, slen = sizeof(context.addr);
    char *message = "Keyboard Input";

    for(i = 0; i < 60000; i++) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0) {
            printf("Could not send packet\n");
        }
    }

    return 0;
}

static int32_t ReceiveVideo(void *opaque) {
    struct SDLVideoContext context = *(struct SDLVideoContext *) opaque;
    int i, recv_size, slen = sizeof(context.socketContext.addr), recv_index = 0;
    char recv_buf[BUFLEN];

    // init decoder
    decoder_t *decoder;
    decoder = create_video_decoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, OUTPUT_WIDTH, OUTPUT_HEIGHT, OUTPUT_WIDTH * 12000);

    for(i = 0; i < 60000; i++)
    {
        if ((recv_size = recvfrom(context.socketContext.s, recv_buf + recv_index, (BUFLEN - recv_index), 0, (struct sockaddr*)(&context.socketContext.addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            recv_index += recv_size;
            if(recv_size != 1000) {
                video_decoder_decode(decoder, recv_buf, recv_index);

                AVPicture pict;
                pict.data[0] = context.yPlane;
                pict.data[1] = context.uPlane;
                pict.data[2] = context.vPlane;
                pict.linesize[0] = OUTPUT_WIDTH;
                pict.linesize[1] = context.uvPitch;
                pict.linesize[2] = context.uvPitch;
                sws_scale(context.sws, (uint8_t const * const *) decoder->frame->data,
                     decoder->frame->linesize, 0, decoder->context->height, pict.data,
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
                recv_index = 0;
                memset(recv_buf, 0, BUFLEN);
            }
        }
    }

    return 0;
}

static int32_t ReceiveAudio(void *opaque) {
    // cast socket and SDL variables back to their data type for usage
    struct SocketContext* context = (struct SocketContext *) opaque;
    int i, recv_size, slen = sizeof(context->addr), recv_index = 0;
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

    dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if(dev == 0) {
        printf("Failed to open audio\n");
        exit(1);
    }
    SDL_PauseAudioDevice(dev, 0);

    for(i = 0; i < 60000; i++) {
        if ((recv_size = recvfrom(context->s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context->addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            audio_decoder_decode(audio_decoder, recv_buf, recv_size);
            SDL_QueueAudio(dev, audio_decoder->frame->data[0], audio_decoder->frame->linesize[0]);
        }
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
    printf("Winsock initialized successfully.\n");

    struct sockaddr_in receive_address;
    int recv_size, slen=sizeof(receive_address);
    char recv_buf[BUFLEN];

    SDL_Event msg;
    SDL_Window *screen;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;
    struct SDLVideoContext SDLVideoContext = {0};

    struct SocketContext InputContext = {0};
    if(CreateUDPContext(&InputContext, "C", "40.121.132.26", 100) < 0) {
        exit(1);
    }

    struct SocketContext VideoReceiveContext = {0};
    if(CreateUDPContext(&VideoReceiveContext, "C", "40.121.132.26", -1) < 0) {
        exit(1);
    }

    struct SocketContext AudioReceiveContext = {0};
    if(CreateUDPContext(&AudioReceiveContext, "C", "40.121.132.26", -1) < 0) {
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

          OUTPUT_WIDTH, // width
          OUTPUT_HEIGHT, // height
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
          OUTPUT_WIDTH, // width
          OUTPUT_HEIGHT // height
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

    // TODO LATER: function to call to adapt window size to client

    SDLVideoContext.yPlane = yPlane;
    SDLVideoContext.sws = sws_ctx;
    SDLVideoContext.uPlane = uPlane;
    SDLVideoContext.vPlane = vPlane;
    SDLVideoContext.uvPitch = uvPitch;
    SDLVideoContext.Renderer = renderer;
    SDLVideoContext.Texture = texture;
    SDLVideoContext.socketContext = VideoReceiveContext;

    SDL_Thread *send_input = SDL_CreateThread(SendUserInput, "SendUserInput", &InputContext);
    SDL_Thread *receive_video = SDL_CreateThread(ReceiveVideo, "ReceiveVideo", &SDLVideoContext);
    SDL_Thread *receive_audio = SDL_CreateThread(ReceiveAudio, "ReceiveAudio", &AudioReceiveContext);

    while (1)
    {
        if (SendAck(&VideoReceiveContext.s) < 0)
            printf("Could not send packet\n");
        if (SendAck(&AudioReceiveContext.s) < 0)
            printf("Could not send packet\n");
        if ((recv_size = recvfrom(InputContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&InputContext.addr), &slen)) < 0)
            printf("Packet not received \n");
        Sleep(2000);
    }
 
    // Actually, we never reach this point...

    closesocket(InputContext.s);
    closesocket(VideoReceiveContext.s);
    closesocket(AudioReceiveContext.s);

    WSACleanup();

    return 0;
}