/*
 * This file contains the implementation of DXGI screen capture.

 Protocol version: 1.0
 Last modification: 1/15/2020

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2020
*/

#include "dxgicapture.h"

#define USE_GPU 0
#define USE_MONITOR 0

void CreateTexture(struct CaptureDevice* device);

int CreateCaptureDevice(struct CaptureDevice *device, int width, int height) {
  device->hardware = (struct DisplayHardware*) malloc(sizeof(struct DisplayHardware));
  memset(device->hardware, 0, sizeof(struct DisplayHardware));

  struct DisplayHardware* hardware = device->hardware;

  D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                    D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_1 };
  UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
  D3D_FEATURE_LEVEL FeatureLevel;

  int num_adapters = 0, num_outputs = 0, i = 0, j = 0;
  IDXGIFactory1 *factory;

#define MAX_NUM_ADAPTERS 10
#define MAX_NUM_OUTPUTS 10
  IDXGIOutput *outputs[MAX_NUM_OUTPUTS];
  IDXGIAdapter *adapters[MAX_NUM_ADAPTERS];
  DXGI_OUTPUT_DESC output_desc;

  HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void**)(&factory));
  if (FAILED(hr)) {
      mprintf("Failed CreateDXGIFactory1: 0x%X %d", hr, WSAGetLastError());
      return -1;
  }

  // GET ALL GPUS
  while(factory->lpVtbl->EnumAdapters1(factory, num_adapters, &hardware->adapter) != DXGI_ERROR_NOT_FOUND) {
    if (num_adapters == MAX_NUM_ADAPTERS) {
      mprintf("Too many adaters!\n");
      break;
    }
    adapters[num_adapters] = hardware->adapter;
    ++num_adapters;
  }

  // GET GPU DESCRIPTIONS
  for(i = 0; i < num_adapters; i++) {
    DXGI_ADAPTER_DESC1 desc;
    hardware->adapter = adapters[i];
    hr = hardware->adapter->lpVtbl->GetDesc1(hardware->adapter, &desc);
    //mprintf("Adapter %d: %s\n", i, desc.Description);
  }

  // Set used GPU
  if (USE_GPU >= num_adapters) {
      mprintf("No GPU with ID %d, only %d adapters\n", USE_GPU, num_adapters);
      return -1;
  }
  hardware->adapter = adapters[USE_GPU];

  // GET ALL MONITORS
  for (int i = 0; i < num_adapters; i++) {
      for (int j = 0; hardware->adapter->lpVtbl->EnumOutputs(adapters[i], j, &hardware->output) != DXGI_ERROR_NOT_FOUND; j++) {
          //mprintf("Found monitor %d on adapter %lu\n", j, i);
          if (i == USE_GPU) {
              if (j == MAX_NUM_OUTPUTS) {
                  mprintf("Too many adapters!\n");
                  break;
              }
              else {
                  outputs[j] = hardware->output;
                  num_outputs++;
              }
          }

      }
  }

  // GET MONITOR DESCRIPTIONS
  for(i = 0; i < num_outputs; i++) {
    hardware->output = outputs[i];
    hr = hardware->output->lpVtbl->GetDesc(hardware->output, &output_desc);
    //mprintf("Monitor %d: %s\n", i, output_desc.DeviceName);
  }

  // Set used output
  if (USE_MONITOR >= num_outputs) {
      mprintf("No Monitor with ID %d, only %d adapters\n", USE_MONITOR, num_outputs);
      return -1;
  }
  hardware->output = outputs[USE_MONITOR];

  hr = D3D11CreateDevice(hardware->adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, NULL, NULL, 0,
    D3D11_SDK_VERSION, &device->D3D11device, &FeatureLevel, &device->D3D11context);
  if (FAILED(hr)) {
      mprintf("Failed D3D11CreateDevice: 0x%X %d", hr, WSAGetLastError());
      return -1;
  }

  IDXGIOutput1* output1;

  DEVMODE dm;
  memset(&dm, 0, sizeof(dm));
  dm.dmSize = sizeof(dm);

  if (0 != EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
  {
      dm.dmPelsWidth = width;
      dm.dmPelsHeight = height;
      dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

      ChangeDisplaySettings(&dm, 0);
  }

  hardware->output->lpVtbl->QueryInterface(hardware->output, &IID_IDXGIOutput1, (void**)&output1);
  output1->lpVtbl->DuplicateOutput(output1, device->D3D11device, &device->duplication);
  hardware->output->lpVtbl->GetDesc(hardware->output, &hardware->final_output_desc);

  device->width = hardware->final_output_desc.DesktopCoordinates.right;
  device->height = hardware->final_output_desc.DesktopCoordinates.bottom;

  return 0;
}

void CreateTexture(struct CaptureDevice *device) {
  struct DisplayHardware* hardware = device->hardware;

  D3D11_TEXTURE2D_DESC tDesc;

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
}

HRESULT CaptureScreen(struct CaptureDevice *device, struct ScreenshotContainer *screenshot) {
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

  //mprintf("Acquiring...\n");
  hr = device->duplication->lpVtbl->AcquireNextFrame(device->duplication, 1, &device->frame_info, &screenshot->desktop_resource);
  //mprintf("Acquired: 0x%X\n", hr);
  if(FAILED(hr)) {
    if (hr != DXGI_ERROR_WAIT_TIMEOUT) {
        mprintf("Failed to Acquire Next Frame! 0x%X %d\n", hr, WSAGetLastError());
    }
    return hr;
  }

  int accumulated_frames = device->frame_info.AccumulatedFrames;
  if (accumulated_frames > 1) {
      mprintf("Accumulated Frames: %d\n", accumulated_frames);
  }

    device->counter++;
    hr = screenshot->desktop_resource->lpVtbl->QueryInterface(screenshot->desktop_resource, &IID_ID3D11Texture2D, (void**)&screenshot->final_texture);
    if (FAILED(hr)) {
        mprintf("Query Interface Failed!\n");
        return hr;
    }
    hr = device->duplication->lpVtbl->MapDesktopSurface(device->duplication, &screenshot->mapped_rect);
    device->did_use_map_desktop_surface = true;
    if(hr == DXGI_ERROR_UNSUPPORTED) {
        CreateTexture(device);
        device->D3D11context->lpVtbl->CopySubresourceRegion(device->D3D11context, (ID3D11Resource*)device->staging_texture, 0, 0, 0, 0,
                                                (ID3D11Resource*)screenshot->final_texture, 0, &device->Box);

        hr = device->staging_texture->lpVtbl->QueryInterface(device->staging_texture, &IID_IDXGISurface, (void**)&screenshot->surface);
        if (FAILED(hr)) {
            mprintf("Query Interface Failed!\n");
            return hr;
        }
        hr = screenshot->surface->lpVtbl->Map(screenshot->surface, &screenshot->mapped_rect, DXGI_MAP_READ);
        if (FAILED(hr)) {
            mprintf("Map Failed!\n");
            return hr;
        }
        device->did_use_map_desktop_surface = false;
    } else if (FAILED(hr)) {
        mprintf("MapDesktopSurface Failed!\n");
        return hr;
    }
    if (FAILED(hr)) {
        mprintf("Something went wrong?\n");
        return hr;
    }
    return hr;
}


void ReleaseScreen(struct CaptureDevice *device, struct ScreenshotContainer *screenshot) {
  if (device->did_use_map_desktop_surface) {
    device->duplication->lpVtbl->UnMapDesktopSurface(device->duplication);
  }
}

void DestroyCaptureDevice(struct CaptureDevice* device) {
    if (device->hardware) {
        free(device->hardware);
    }
    if (device->duplication) {
        device->duplication->lpVtbl->Release(device->duplication);
        device->duplication = NULL;
    }
}
