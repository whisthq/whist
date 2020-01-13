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
#define BITRATE 25000
#define USE_GPU 0
#define USE_MONITOR 0

struct RTPPacket {
  uint8_t data[MAX_PACKET_SIZE];
  int index;
  int payload_size;
  int id;
  bool is_ending;
};

struct ScreenshotContainer {
  IDXGIResource *desktop_resource;
  ID3D11Texture2D *final_texture;
  DXGI_MAPPED_RECT mapped_rect;
  IDXGISurface *surface;
};

struct CaptureDevice {
  D3D11_BOX Box;
  ID3D11Device *D3D11device;
  ID3D11DeviceContext *D3D11context;
  IDXGIOutputDuplication *duplication;
  ID3D11Texture2D *staging_texture;
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  DXGI_OUTDUPL_DESC duplication_desc;
  int counter;
};

struct DisplayHardware {
  IDXGIAdapter1 *adapter;
  IDXGIOutput* output;
  DXGI_OUTPUT_DESC final_output_desc;
};

void CreateDisplayHardware(struct DisplayHardware *hardware, struct CaptureDevice *device) {
  int num_adapters = 0, num_outputs = 0, i = 0, j = 0;
  IDXGIFactory1 *factory;
  IDXGIOutput *outputs[10];
  IDXGIAdapter *adapters[10];
  DXGI_OUTPUT_DESC output_desc;
  D3D_FEATURE_LEVEL FeatureLevel;

  HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void**)(&factory));

  // GET ALL GPUS
  while(factory->lpVtbl->EnumAdapters1(factory, num_adapters, &hardware->adapter) != DXGI_ERROR_NOT_FOUND) {
    adapters[num_adapters] = hardware->adapter;
    ++num_adapters;
  }

  // GET GPU DESCRIPTIONS
  for(i = 0; i < num_adapters; i++) {
    DXGI_ADAPTER_DESC1 desc;
    hardware->adapter = adapters[i];
    hr = hardware->adapter->lpVtbl->GetDesc1(hardware->adapter, &desc);
    // printf("Adapter: %s\n", desc.Description);
  }

  // GET ALL MONITORS
  for(i = 0; i < num_adapters; i++) {
    int this_adapter_outputs = 0;
    hardware->adapter = adapters[i];
    while(hardware->adapter->lpVtbl->EnumOutputs(hardware->adapter, this_adapter_outputs, &hardware->output) != DXGI_ERROR_NOT_FOUND) {
      // printf("Found monitor %d on adapter %lu\n", this_adapter_outputs, i);
      outputs[num_outputs] = hardware->output;
      ++this_adapter_outputs;
      ++num_outputs;
    }  
  }

  // GET MONITOR DESCRIPTIONS
  for(i = 0; i < num_outputs; i++) {
    hardware->output = outputs[i];
    hr = hardware->output->lpVtbl->GetDesc(hardware->output, &output_desc);
    // printf("Monitor: %s\n", output_desc.DeviceName);
  }

  hardware->adapter = adapters[USE_GPU];
  hardware->output = outputs[USE_MONITOR];


  hr = D3D11CreateDevice(hardware->adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, NULL, NULL, 0,
    D3D11_SDK_VERSION, &device->D3D11device, &FeatureLevel, &device->D3D11context);

}

void CreateTexture(struct DisplayHardware *hardware, struct CaptureDevice *device) {
  D3D11_TEXTURE2D_DESC tDesc;
  IDXGIOutput1* output1;

  hardware->output->lpVtbl->QueryInterface(hardware->output, &IID_IDXGIOutput1, (void**)&output1);
  output1->lpVtbl->DuplicateOutput(output1, device->D3D11device, &device->duplication);
  hardware->output->lpVtbl->GetDesc(hardware->output, &hardware->final_output_desc);

  // Texture to store GPU pixels
  tDesc.Width = hardware->final_output_desc.DesktopCoordinates.right;
  tDesc.Height = hardware->final_output_desc.DesktopCoordinates.bottom;
  tDesc.MipLevels = 1;
  tDesc.ArraySize = 1;
  tDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  tDesc.SampleDesc.Count = 1;
  tDesc.SampleDesc.Quality = 0;
  tDesc.Usage = D3D11_USAGE_STAGING;
  tDesc.BindFlags = 0;
  tDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  tDesc.MiscFlags = 0;

  device->Box.top = hardware->final_output_desc.DesktopCoordinates.top;
  device->Box.left = hardware->final_output_desc.DesktopCoordinates.left;
  device->Box.right = hardware->final_output_desc.DesktopCoordinates.right;
  device->Box.bottom = hardware->final_output_desc.DesktopCoordinates.bottom;
  device->Box.front = 0;
  device->Box.back = 1;

  device->D3D11device->lpVtbl->CreateTexture2D(device->D3D11device, &tDesc, NULL, &device->staging_texture);

  device->duplication->lpVtbl->GetDesc(device->duplication, &device->duplication_desc);
  printf("Desktop image in memory: %d\n", device->duplication_desc.DesktopImageInSystemMemory);
}

void CaptureScreen(struct CaptureDevice *device, struct ScreenshotContainer *screenshot) {
  int frameCaptured;
  HRESULT hr;

  device->duplication->lpVtbl->ReleaseFrame(device->duplication);

  if (NULL != screenshot->final_texture) {
    screenshot->final_texture->lpVtbl->Release(screenshot->final_texture);
    screenshot->final_texture = NULL;
  }
  
  if (NULL != screenshot->desktop_resource) {
    screenshot->desktop_resource->lpVtbl->Release(screenshot->desktop_resource);
    screenshot->desktop_resource = NULL;
  }

  hr = device->duplication->lpVtbl->AcquireNextFrame(device->duplication, 0, &device->frame_info, &screenshot->desktop_resource);
  if(FAILED(hr)) {
    frameCaptured = 0;
  } else {
    frameCaptured = 1;
  }

  if(frameCaptured) {
    device->counter++;
    hr = screenshot->desktop_resource->lpVtbl->QueryInterface(screenshot->desktop_resource, &IID_ID3D11Texture2D, (void**)&screenshot->final_texture);
    hr = device->duplication->lpVtbl->MapDesktopSurface(device->duplication, &screenshot->mapped_rect);
    if(hr == DXGI_ERROR_UNSUPPORTED) {
      device->D3D11context->lpVtbl->CopySubresourceRegion(device->D3D11context, (ID3D11Resource*)device->staging_texture, 0, 0, 0, 0,
                                              (ID3D11Resource*)screenshot->final_texture, 0, &device->Box);

      hr = device->staging_texture->lpVtbl->QueryInterface(device->staging_texture, &IID_IDXGISurface, (void**)&screenshot->surface);
      hr = screenshot->surface->lpVtbl->Map(screenshot->surface, &screenshot->mapped_rect, DXGI_MAP_READ);
    }
  }
}


void ReleaseScreen(struct CaptureDevice *device, struct ScreenshotContainer *screenshot) {
  device->duplication->lpVtbl->UnMapDesktopSurface(device->duplication);
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

  struct DisplayHardware *hardware = (struct DisplayHardware *) malloc(sizeof(struct DisplayHardware));
  memset(hardware, 0, sizeof(struct DisplayHardware));

  struct CaptureDevice *device = (struct CaptureDevice *) malloc(sizeof(struct CaptureDevice));
  memset(device, 0, sizeof(struct CaptureDevice));

  struct ScreenshotContainer *screenshot = (struct ScreenshotContainer *) malloc(sizeof(struct ScreenshotContainer));
  memset(screenshot, 0, sizeof(struct ScreenshotContainer));

  CreateDisplayHardware(hardware, device);
  CreateTexture(hardware, device);

  device->counter = 0;

  encoder_t *encoder;
  encoder = create_video_encoder(
    CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH, CAPTURE_HEIGHT, CAPTURE_WIDTH * BITRATE);

  while(1) {
    CaptureScreen(device, screenshot);
    video_encoder_encode(encoder, screenshot->mapped_rect.pBits);
    if (encoder->packet.size != 0) {
      if (SendPacket(&context, encoder->packet.data, encoder->packet.size, id) < 0) {
          printf("Could not send video frame\n");
      } 
    }
    ReleaseScreen(device, screenshot);
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