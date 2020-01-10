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

#define SDL_MAIN_HANDLED
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 1000
#define RECV_BUFFER_LEN 38 // exact user input packet line to prevent clumping
#define FRAME_BUFFER_SIZE (1024 * 1024)
#define MAX_PACKET_SIZE 1000

static int SendPacket(struct SocketContext *context, uint8_t *data, int len) {
  int sent_size, payload_size, slen = sizeof(context->addr), i = 0;
  uint8_t payload[MAX_PACKET_SIZE];
  int curr_index = 0;

  while (curr_index < len) {
    payload_size = min((MAX_PACKET_SIZE), (len - curr_index));
    if((sent_size = sendto(context->s, (data + curr_index), payload_size, 0, (struct sockaddr*)(&context->addr), slen)) < 0) {
      return -1;
    } else {
      curr_index += payload_size;
    }
  }
  return 0;
}

static int32_t SendVideo(void *opaque) {
    struct SocketContext context = *(struct SocketContext *) opaque;
    int slen = sizeof(context.addr);

    // get window
    HWND window = NULL;
    window = GetDesktopWindow();
    frame_area frame = {0, 0, 0, 0}; // init  frame area

    // init screen capture device
    video_capture_device *device;
    device = create_video_capture_device(window, frame);

    // set encoder parameters
    int width = device->width; // in and out from the capture device
    int height = device->height; // in and out from the capture device
    int bitrate = width * 30000; // estimate bit rate based on output size

    // init encoder
    encoder_t *encoder;
    encoder = create_video_encoder(width, height, CAPTURE_WIDTH, CAPTURE_HEIGHT, bitrate);

    // video variables
    void *capturedframe; // var to hold captured frame, as a void* to RGB pixels
    int sent_size; // var to keep track of size of packets sent

    // while stream is on
    while(1) {
      // capture a frame
      capturedframe = capture_screen(device);

      video_encoder_encode(encoder, capturedframe);

      if (encoder->packet.size != 0) {
        // printf("Sending packet of size %d\n", encoder->packet.size);
        if (SendPacket(&context, encoder->packet.data, encoder->packet.size) < 0) {
            printf("Could not send video frame\n");
        }
      }
  }

  // exited while loop, stream done let's close everything
  destroy_video_encoder(encoder); // destroy encoder
  destroy_video_capture_device(device); // destroy capture device
  return 0;
}

static int32_t SendAudio(void *opaque) {
  struct SocketContext context = *(struct SocketContext *) opaque;
  int slen = sizeof(context.addr);

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
            if (SendPacket(&context, audio_device->pData, audio_device->audioBufSize) < 0) {
                printf("Could not send audio frame\n");
            }
          }

          audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(
              audio_device->pAudioCaptureClient,
              nNumFramesToRead);
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