#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

#include "../include/fractal.h"
#include "../include/videocapture.h"
#include "../include/wasapicapture.h"
#include "../include/videoencode.h"
#include "../include/audioencode.h"
#include "../include/dxgicapture.h"

#define SDL_MAIN_HANDLED
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 1000
#define RECV_BUFFER_LEN 38 // exact user input packet line to prevent clumping
#define FRAME_BUFFER_SIZE (1024 * 1024)
#define MAX_PACKET_SIZE 1400
#define BITRATE 30000
#define USE_GPU 0
#define USE_MONITOR 0

LARGE_INTEGER frequency;
LARGE_INTEGER start;
LARGE_INTEGER end;
double interval;

struct RTPPacket {
  uint8_t data[MAX_PACKET_SIZE];
  int index;
  int payload_size;
  int id;
  bool is_ending;
};

static int SendPacket(struct SocketContext *context, uint8_t *data, int len, int id) {
  int sent_size, payload_size, slen = sizeof(context->addr);
  int curr_index = 0, i = 0;

  while (curr_index < len) {
    struct RTPPacket packet = {0};
    payload_size = min(MAX_PACKET_SIZE, (len - curr_index));
    memcpy(packet.data, data + curr_index, payload_size);

    packet.index = i;
    packet.payload_size = payload_size;
    packet.id = id;
    packet.is_ending = curr_index + payload_size == len;

    if((sent_size = sendto(context->s, &packet, sizeof(packet), 0, (struct sockaddr*)(&context->addr), slen)) < 0) {
      return -1;
    } else {
      i++;
      curr_index += payload_size;
    }
  }
  return 0;
}


static int32_t SendVideo(void *opaque) {
  struct SocketContext context = *(struct SocketContext *) opaque;
  int slen = sizeof(context.addr), id = 0;

  double current_fps = 0;
  double current_bitrate = 0.0;

  encoder_t *encoder;
  encoder = create_video_encoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, 
    CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH * BITRATE);

  DXGIDevice *device = (DXGIDevice *) malloc(sizeof(DXGIDevice));
  memset(device, 0, sizeof(DXGIDevice));
  CreateDXGIDevice(device);

  clock previousFrameTime;
  int previousFrameSize = 0;

  while(1) {
    HRESULT hr = CaptureScreen(device);
    if (hr == S_OK) {
      video_encoder_encode(encoder, device->frame_data.pBits);
      if (encoder->packet.size != 0) {
        if (previousFrameSize > 0) {
            double mbps = previousFrameSize / GetTimer(previousFrameTime);
            printf("MBPS: %f\n", mbps);
        }
        if (SendPacket(&context, encoder->packet.data, encoder->packet.size, id) < 0) {
          printf("Could not send video frame\n");
        } else {
          printf("Sent size %d\n", encoder->packet.size);
          previousFrameSize = encoder->packet.size;
          StartTimer(&previousFrameTime);
        }
      }

      id++;
      ReleaseScreen(device);

      frames++;
    }
    else if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        continue;
    }
    else {
        printf("ERROR\n");
        break;
    }
  }
  return 0;
}

static int32_t SendAudio(void *opaque) {
  return 0;
  struct SocketContext context = *(struct SocketContext *) opaque;
  int slen = sizeof(context.addr), id = 0;

  wasapi_device *audio_device = (wasapi_device *) malloc(sizeof(struct wasapi_device));
  audio_device = CreateAudioDevice(audio_device);
  StartAudioDevice(audio_device);

  HRESULT hr = CoInitialize(NULL);
  DWORD dwWaitResult;
  UINT32 nNumFramesToRead, nNextPacketSize, nBlockAlign = audio_device->pwfx->nBlockAlign;
  DWORD dwFlags;

  while(1) {
      for (hr = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(audio_device->pAudioCaptureClient, &nNextPacketSize);
           SUCCEEDED(hr) && nNextPacketSize > 0;
           hr = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(audio_device->pAudioCaptureClient, &nNextPacketSize)
      ) {
          audio_device->pAudioCaptureClient->lpVtbl->GetBuffer(
              audio_device->pAudioCaptureClient,
              &audio_device->pData, &nNumFramesToRead,
              &dwFlags, NULL, NULL);

          audio_device->audioBufSize = nNumFramesToRead * nBlockAlign;

          if (audio_device->audioBufSize != 0) {
            if (SendPacket(&context, audio_device->pData, audio_device->audioBufSize, id) < 0) {
                printf("Could not send audio frame\n");
            }
          }

          audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(
              audio_device->pAudioCaptureClient,
              nNumFramesToRead);

          id++;
      }
      dwWaitResult =  WaitForSingleObject(audio_device->hWakeUp, INFINITE);
  } 
  DestroyAudioDevice(audio_device);
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

    struct SocketContext InputReceiveContext = {0};
    if(CreateUDPContext(&InputReceiveContext, "S", "", -1) < 0) {
        exit(1);
    }

    struct SocketContext VideoContext = {0};
    if(CreateUDPContext(&VideoContext, "S", "", 100) < 0) {
        exit(1);
    }

    struct SocketContext AudioContext = {0};
    if(CreateUDPContext(&AudioContext, "S", "", 100) < 0) {
        exit(1);
    }

    SendAck(&InputReceiveContext, 5); 
    SDL_Thread *send_video = SDL_CreateThread(SendVideo, "SendVideo", &VideoContext);
    SDL_Thread *send_audio = SDL_CreateThread(SendAudio, "SendAudio", &AudioContext);

    struct FractalMessage fmsgs[6];
    struct FractalMessage fmsg;
    int slen = sizeof(InputReceiveContext.addr), i = 0, j = 0, active = 0, repeat = 1;
    FractalStatus status;

    while(repeat) {
        if (recvfrom(InputReceiveContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&InputReceiveContext.addr), &slen) > 0) {
          if(fmsg.type == MESSAGE_KEYBOARD) {
            if(active) {
              fmsgs[j] = fmsg;
              if(fmsg.keyboard.pressed) {
                if(fmsg.keyboard.code != fmsgs[j - 1].keyboard.code) {
                  j++;
                }
              } else {
                status = ReplayUserInput(fmsgs, j + 1);
                active = 0;
                j = 0;
              }
            } else {
              fmsgs[0] = fmsg;
              if(fmsg.keyboard.pressed && (fmsg.keyboard.code >= 224 && fmsg.keyboard.code <= 231)) {
                active = 1;
                j++;
              } else {
                status = ReplayUserInput(fmsgs, 1);
              }    
            }
          } else {
            fmsgs[0] = fmsg;
            status = ReplayUserInput(fmsgs, 1);
          }
        }
        if(i % (30 * 60) == 0) {
          SendAck(&InputReceiveContext, 1);
        }
        if(status != FRACTAL_OK) {
          repeat = 0;
        }
    }

    closesocket(InputReceiveContext.s);
    closesocket(VideoContext.s);
    closesocket(AudioContext.s);

    WSACleanup();

    return 0;
}