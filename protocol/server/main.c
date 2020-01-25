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

#define BITRATE 10000
#define USE_GPU 0
#define USE_MONITOR 0
#define ENCODE_TYPE SOFTWARE_ENCODE

LARGE_INTEGER frequency;
LARGE_INTEGER start;
LARGE_INTEGER end;
double interval;

static int SendPacket(struct SocketContext *context, uint8_t *data, int len, int id, double time) {
  int sent_size, payload_size, slen = sizeof(context->addr);
  int curr_index = 0, i = 0;

  clock packet_timer;
  StartTimer(&packet_timer);

  while (curr_index < len) {
    struct RTPPacket packet = {0};
    payload_size = min(MAX_PACKET_SIZE, (len - curr_index));

    memcpy(packet.data, data + curr_index, payload_size);
    packet.index = i;
    packet.payload_size = payload_size;
    packet.id = id;
    packet.is_ending = curr_index + payload_size == len;
    packet.hash = Hash(packet.data, packet.payload_size);

    if((sent_size = sendto(context->s, &packet, sizeof(packet), 0, (struct sockaddr*)(&context->addr), slen)) < 0) {
      return -1;
    } else {
      i++;
      curr_index += payload_size;
      while(time > 0.0 && GetTimer(packet_timer) / time < 1.0 * curr_index / len);
    }
  }
  return 0;
}


static int32_t SendVideo(void *opaque) {
  struct SocketContext context = *(struct SocketContext *) opaque;
  int slen = sizeof(context.addr), id = 0;

  int current_bitrate = BITRATE;
  encoder_t *encoder;
  encoder = create_video_encoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, 
    CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH * current_bitrate, ENCODE_TYPE);

  DXGIDevice *device = (DXGIDevice *) malloc(sizeof(DXGIDevice));
  memset(device, 0, sizeof(DXGIDevice));
  CreateDXGIDevice(device);

  double current_max_mbps = 100.0;
  bool update_bitrate = false;

  double worst_fps = 20.0;
  int ideal_bitrate = current_bitrate;
  int bitrate_tested_frames = 0;

  clock previous_frame_time;
  int previous_frame_size = 0;

  while(1) {
    if (current_max_mbps != GetMaxMBPS()) {
      current_max_mbps = GetMaxMBPS();
    }
    HRESULT hr = CaptureScreen(device);
    if (hr == S_OK) {
      if (update_bitrate) {
        destroy_video_encoder(encoder);
        encoder = create_video_encoder(CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH * current_bitrate, ENCODE_TYPE);
        update_bitrate = false;
      }

      bitrate_tested_frames++;

      video_encoder_encode(encoder, device->frame_data.pBits);

      if (encoder->packet.size != 0) {
        double delay = -1.0;

        if (previous_frame_size > 0) {
            double frame_time = GetTimer(previous_frame_time);
            double mbps = previous_frame_size * 8.0 / 1024.0 / 1024.0 / frame_time;
            // previousFrameSize * 8.0 / 1024.0 / 1024.0 / IdealTime = current_max_mbps
            // previousFrameSize * 8.0 / 1024.0 / 1024.0 / current_max_mbps = IdealTime
            double ideal_time = previous_frame_size * 8.0 / 1024.0 / 1024.0 / current_max_mbps;
            // printf("Size: %d, MBPS: %f, VS MAX MBPS: %f, Time: %f, Ideal Time: %f, Wait Time: %f\n", previous_frame_size, mbps, current_max_mbps, frame_time, ideal_time, ideal_time - frame_time);
            if (ideal_time > frame_time) {
              // Ceiling of how much time to delay
              delay = ideal_time - frame_time;
            }

            double previous_fps = 1.0 / ideal_time;
            if ((previous_fps < worst_fps || ideal_bitrate > current_bitrate) && bitrate_tested_frames > 20) {
              // Rather than having lower than the worst acceptable fps, find the ratio for what the bitrate should be
              double ratio_bitrate = previous_fps / worst_fps;
              int new_bitrate = (int) (ratio_bitrate * current_bitrate);
              if (abs(new_bitrate - current_bitrate) / new_bitrate > 0.05) {
                printf("Updating bitrate from %d to %d\n", current_bitrate, new_bitrate);
                current_bitrate = new_bitrate;
                update_bitrate = true;
                bitrate_tested_frames = 0;
              }
            }
        }

        if (SendPacket(&context, encoder->packet.data, encoder->packet.size, id, delay) < 0) {
          printf("Could not send video frame\n");
        } else {
          printf("Sent size %d\n", encoder->packet.size);
          previous_frame_size = encoder->packet.size;
          StartTimer(&previous_frame_time);
        }
      }

      id++;
      ReleaseScreen(device);
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
            if (SendPacket(&context, audio_device->pData, audio_device->audioBufSize, id, -1.0) < 0) {
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