#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

// specific headers for audio processing
#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>

// Something for threads
#include<avrt.h>

#include "../include/fractal.h"
#include "../include/videocapture.h"
#include "../include/audiocapture.h"
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

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(IID_IAudioCaptureClient, 0xc8adbd64, 0xe71e, 0x48a0, 0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17);

struct audio_capture_device {
    IMMDevice *device;
    IMMDeviceEnumerator *pMMDeviceEnumerator;
    IAudioClient *pAudioClient;
    REFERENCE_TIME hnsDefaultDevicePeriod;
    WAVEFORMATEX *pwfx;
    IAudioCaptureClient *pAudioCaptureClient;
    HANDLE hWakeUp;
    BYTE *pData;
    LONG lBytesToWrite;
    UINT32 nNumFramesToRead;
    DWORD dwWaitResult;
};

HRESULT *CreateAudioDevice(struct audio_capture_device *audio_device) {
    HRESULT hr = CoInitialize(NULL);
    memset(audio_device, 0, sizeof(struct audio_capture_device));

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,  &IID_IMMDeviceEnumerator, (void**)&audio_device->pMMDeviceEnumerator);
    if (FAILED(hr)) {
        printf(L"CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
        return hr;
    }

    // get the default render endpoint
    hr = audio_device->pMMDeviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(audio_device->pMMDeviceEnumerator, eRender, eConsole, &audio_device->device);
    if (FAILED(hr)) {
        printf("Failed to get default audio endpoint.\n");
        return hr;
    }

    hr = audio_device->device->lpVtbl->Activate(audio_device->device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&audio_device->pAudioClient);
    if (FAILED(hr)) {
        printf(L"IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", hr);
        return hr;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetDevicePeriod(audio_device->pAudioClient, &audio_device->hnsDefaultDevicePeriod, NULL);
    if (FAILED(hr)) {
        printf(L"IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
        return hr;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetMixFormat(
        audio_device->pAudioClient,
        &audio_device->pwfx);
    if (FAILED(hr)) {
        printf(L"IAudioClient::GetMixFormat failed: hr = 0x%08x", hr);
        return hr;
    }

    hr = audio_device->pAudioClient->lpVtbl->Initialize(audio_device->pAudioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, audio_device->pwfx, 0);
    if (FAILED(hr)) {
        printf(L"IAudioClient::Initialize failed: hr = 0x%08x", hr);
        return hr;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetService(audio_device->pAudioClient, &IID_IAudioCaptureClient, (void**)&audio_device->pAudioCaptureClient);

    if (FAILED(hr)) {
        printf(L"IAudioClient::GetService(IAudioCaptureClient) failed: hr = 0x%08x", hr);
        return hr;
    }

    return audio_device;
}

HRESULT *StartAudioDevice(struct audio_capture_device *audio_device) {
    HRESULT hr = CoInitialize(NULL);
    audio_device->hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);
    if (audio_device->hWakeUp == NULL) {
        DWORD dwErr = GetLastError();
        printf(L"CreateWaitableTimer failed: last error = %u", dwErr);
        return HRESULT_FROM_WIN32(dwErr);
    }

    LARGE_INTEGER liFirstFire;
    liFirstFire.QuadPart = -1 * audio_device->hnsDefaultDevicePeriod / 2; // negative means relative time
    LONG lTimeBetweenFires = (LONG)audio_device->hnsDefaultDevicePeriod / 2 / 10000; // convert to milliseconds
    BOOL bOK = SetWaitableTimer(
        audio_device->hWakeUp,
        &liFirstFire,
        lTimeBetweenFires,
        NULL, NULL, FALSE
    );


    hr = audio_device->pAudioClient->lpVtbl->Start(audio_device->pAudioClient);
    if (FAILED(hr)) {
        printf(L"IAudioClient::Start failed: hr = 0x%08x", hr);
        return hr;
    }

    return S_OK;
}

HRESULT *DestroyAudioDevice(struct audio_capture_device *audio_device) {
    audio_device->pAudioClient->lpVtbl->Stop(audio_device->pAudioClient);
    audio_device->pAudioCaptureClient->lpVtbl->Release(audio_device->pAudioCaptureClient);
    CoTaskMemFree(audio_device->pwfx);
    audio_device->pAudioClient->lpVtbl->Release(audio_device->pAudioClient);
    audio_device->device->lpVtbl->Release(audio_device->device);
    audio_device->pMMDeviceEnumerator->lpVtbl->Release(audio_device->pMMDeviceEnumerator);
    CoUninitialize();
}

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
        // send packet
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

  struct audio_capture_device *audio_device = (struct audio_capture_device *) malloc(sizeof(struct audio_capture_device));
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

          audio_device->lBytesToWrite = nNumFramesToRead * nBlockAlign;

          if (audio_device->lBytesToWrite != 0) {
            if (SendPacket(&context, audio_device->pData, audio_device->lBytesToWrite) < 0) {
                printf("Could not send video frame\n");
            }
          }

          hr = audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(
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