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
#define MAX_PACKET_SIZE 1400
#define BITRATE 35000
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


ID3D11Device * createDirect3D11Device(IDXGIAdapter1 * pOutputAdapter) {
  D3D_FEATURE_LEVEL aFeatureLevels[] =
    {
      D3D_FEATURE_LEVEL_11_0
    };

  ID3D11Device * pDevice;
  D3D_FEATURE_LEVEL featureLevel;

  D3D_DRIVER_TYPE DriverTypes[] =
    {
      D3D_DRIVER_TYPE_UNKNOWN
    };
  ID3D11DeviceContext * pDeviceContext;
  HRESULT hCreateDevice;
  for (int driverTypeIndex = 0 ; driverTypeIndex < ARRAYSIZE(DriverTypes) ; driverTypeIndex++) {
    D3D_DRIVER_TYPE driverType = DriverTypes[driverTypeIndex];

    hCreateDevice = D3D11CreateDevice(pOutputAdapter,
                driverType,
                NULL,
                0,
                aFeatureLevels,
                ARRAYSIZE(aFeatureLevels),
                D3D11_SDK_VERSION,
                &pDevice,
                &featureLevel,
                &pDeviceContext);
    if (hCreateDevice == S_OK) {
      printf("created D3D11 adapter at %p\n", pDevice);
      printf("used driver type: %i\n", driverType);

      return pDevice;
    } else {
      printf("error creating D3D11 device using %i\n", driverType);
      printf("error creating D3D11 device: 0x%X\n", hCreateDevice);
    }
  }

  return -1;
}

IDXGIOutput1 * findAttachedOutput(IDXGIFactory1 * pFactory) {
  IDXGIAdapter1* pAdapter;

  UINT adapterIndex = 0;
  while (pFactory->lpVtbl->EnumAdapters1(pFactory, adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
    IDXGIAdapter2* pAdapter2 = (IDXGIAdapter2*) pAdapter;
        
    UINT outputIndex = 0;
    IDXGIOutput * pOutput;

    DXGI_ADAPTER_DESC aDesc;
    pAdapter2->lpVtbl->GetDesc(pAdapter2, &aDesc);

    HRESULT hEnumOutputs = pAdapter->lpVtbl->EnumOutputs(pAdapter, outputIndex, &pOutput);
    while(hEnumOutputs != DXGI_ERROR_NOT_FOUND) {
      if (hEnumOutputs != S_OK) {
        printf("output enumeration error: 0x%X\n", hEnumOutputs);
        return -1;
      }

      DXGI_OUTPUT_DESC oDesc;
      pOutput->lpVtbl->GetDesc(pOutput, &oDesc);

      if (oDesc.AttachedToDesktop) {
        return (IDXGIOutput1*)pOutput;
      }

      ++outputIndex;
      hEnumOutputs = pAdapter->lpVtbl->EnumOutputs(pAdapter, outputIndex, &pOutput);
    }

    ++adapterIndex;
  }

  return -1;
}

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
  int slen = sizeof(context.addr);

  int frameCaptured = 0, counter = 0, id = 0, i;
  HRESULT hr;

  //Initialize the factory pointer
  IDXGIFactory2* pFactory;
  
  //Actually create it
  HRESULT hCreateFactory = CreateDXGIFactory1(&IID_IDXGIFactory2, (void**)(&pFactory));
  if (hCreateFactory != S_OK) {
    printf("ERROR: 0x%X\n", hCreateFactory);

    return 1;
  }

  IDXGIOutput1 *pAttachedOutput = findAttachedOutput(pFactory);

  void * pOutputParent;
  if (pAttachedOutput->lpVtbl->GetParent(pAttachedOutput, &IID_IDXGIAdapter1, &pOutputParent) != S_OK) {
    return -1;
  }

  IDXGIAdapter1 * pAttachedOutputAdapter = (IDXGIAdapter1 *)pOutputParent;
  ID3D11Device * pD3Device = createDirect3D11Device(pAttachedOutputAdapter);

  DXGI_OUTPUT_DESC oDesc;
  pAttachedOutput->lpVtbl->GetDesc(pAttachedOutput, &oDesc);

  DXGI_ADAPTER_DESC aDesc;
  pAttachedOutputAdapter->lpVtbl->GetDesc(pAttachedOutputAdapter, &aDesc);

  printf("duplicating '%S' attached to '%S'\n", oDesc.DeviceName, aDesc.Description);
  
  IDXGIOutputDuplication * pOutputDuplication;
  HRESULT hDuplicateOutput = pAttachedOutput->lpVtbl->DuplicateOutput(pAttachedOutput, pD3Device, &pOutputDuplication);
  if (hDuplicateOutput != S_OK) {
    printf("error duplicating output: 0x%X\n", hDuplicateOutput);

    return 1;
  }

  DXGI_OUTDUPL_DESC odDesc;
  pOutputDuplication->lpVtbl->GetDesc(pOutputDuplication, &odDesc);

  printf("able to duplicate a %ix%i desktop\n", odDesc.ModeDesc.Width, odDesc.ModeDesc.Height);

  DXGI_OUTDUPL_FRAME_INFO frameInfo;
  IDXGIResource * pDesktopResource;

  encoder_t *encoder;
  encoder = create_video_encoder(
    CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH * BITRATE);

  int sent_frames = 0;

  while(1) {
    HRESULT hNextFrame = pOutputDuplication->lpVtbl->AcquireNextFrame(pOutputDuplication, 17, &frameInfo, &pDesktopResource);
    
    DXGI_MAPPED_RECT frameData;
    HRESULT hMapDesktopSurface = pOutputDuplication->lpVtbl->MapDesktopSurface(pOutputDuplication, &frameData);
    
    if (hMapDesktopSurface == S_OK) {
      video_encoder_encode(encoder, frameData.pBits);
      if (encoder->packet.size != 0) {
        if (SendPacket(&context, encoder->packet.data, encoder->packet.size, id) < 0) {
            printf("Could not send video frame\n");
        } 
        sent_frames++;
      }
      id++;
    }
    
    HRESULT hUnMapDesktopSurface = pOutputDuplication->lpVtbl->UnMapDesktopSurface(pOutputDuplication);
    HRESULT hReleaseFrame = pOutputDuplication->lpVtbl->ReleaseFrame(pOutputDuplication);
  }
  return 0;
}

static int32_t SendAudio(void *opaque) {
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