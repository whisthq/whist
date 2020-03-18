/*
 * This file contains the implementation of DXGI screen capture.

 Protocol version: 1.0
 Last modification: 1/15/2020

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2020
*/

#include "dxgicapture.h"

#include <windows.h>
#include <psapi.h>
#include <processthreadsapi.h>

void PrintMemoryInfo()
{
    DWORD processID = GetCurrentProcessId();
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    // Print information about the memory usage of the process.

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, processID);
    if (NULL == hProcess)
        return;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
        mprintf("\tPeakWorkingSetSize: %lld\n", (long long)pmc.PeakWorkingSetSize);
        mprintf("\tWorkingSetSize: %lld\n", (long long)pmc.WorkingSetSize);
    }

    CloseHandle(hProcess);
}

#include "dxgicapture.h"

#define USE_GPU 0
#define USE_MONITOR 0

int CreateCaptureDevice(struct CaptureDevice *device, int width, int height) {
  memset(device, 0, sizeof(struct CaptureDevice));

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
  IDXGIAdapter1 *adapters[MAX_NUM_ADAPTERS];
  DXGI_OUTPUT_DESC output_desc;

  HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void**)(&factory));
  if (FAILED(hr)) {
      mprintf("Failed CreateDXGIFactory1: 0x%X %d", hr, GetLastError());
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

  UINT num = 0;
  DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
  UINT flags = 0;
  hr = hardware->output->lpVtbl->GetDisplayModeList( hardware->output, format, flags, &num, 0 );
  if( FAILED( hr ) )
  {
      mprintf( "hr: %X\n", hr );
  }
  mprintf( "hr: %d\n", hr );
  mprintf( "Num: %d\n", num );
  DXGI_MODE_DESC* pDescs = malloc(sizeof( DXGI_MODE_DESC ) * num);
  DXGI_MODE_DESC* finalDesc = NULL;
  hardware->output->lpVtbl->GetDisplayModeList( hardware->output, format, flags, &num, pDescs );
  for( int i = 0; i < num; i++ )
  {
      mprintf( "Possible Resolution: %dx%d\n", pDescs[i].Width, pDescs[i].Height );
      if( pDescs[i].Width == width && pDescs[i].Height == height )
      {
          mprintf( "Match found for %dx%d!\n", width, height );
          finalDesc = &pDescs[i];
      }
  }

  /*
  DEVMODE dm;
  memset( &dm, 0, sizeof( dm ) );
  dm.dmSize = sizeof( dm );

  if( 0 != EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ) )
  {
      dm.dmPelsWidth = width;
      dm.dmPelsHeight = height;
      dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

      int ret = ChangeDisplaySettings( &dm, 0 );
      mprintf( "ChangeDisplaySettingsCode: %d\n", ret );
  } else
  {
      mprintf( "Failed to update DisplaySettings\n" );
  }
  */

  //hr = D3D11CreateDevice(hardware->adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, NULL, NULL, 0,
  //  D3D11_SDK_VERSION, &device->D3D11device, &FeatureLevel, &device->D3D11context);

  // CREATE SWAP CHAIN DESC
  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  ZeroMemory( &swapChainDesc, sizeof( swapChainDesc ) );
  swapChainDesc.BufferCount = 1;


  swapChainDesc.BufferDesc.Width = 1920;
  swapChainDesc.BufferDesc.Height = 1080;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

  if( finalDesc )
  {
      mprintf( "USING FINAL DESC!\n" );
      swapChainDesc.BufferDesc = *finalDesc;
  }


  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.OutputWindow = GetDesktopWindow();
  swapChainDesc.Windowed = false;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.SampleDesc.Quality = 0;
  // END CREATE SWAP CHAIN DESC

  IDXGISwapChain* swapChain = NULL;

  hr = D3D11CreateDevice(
      hardware->adapter,
      D3D_DRIVER_TYPE_UNKNOWN,
      NULL,
      NULL,
      NULL,
      0,
      D3D11_SDK_VERSION,
      //&swapChainDesc,
      //&swapChain,
      &device->D3D11device,
      &FeatureLevel,
      &device->D3D11context
  );

  if (FAILED(hr)) {
      mprintf("Failed D3D11CreateDevice: 0x%X %d", hr, GetLastError());
      return -1;
  }

  if( swapChain )
  {
      swapChain->lpVtbl->Release( swapChain );
  }

  IDXGIOutput1* output1;

  hr = hardware->output->lpVtbl->QueryInterface(hardware->output, &IID_IDXGIOutput1, (void**)&output1);
  if (FAILED(hr)) {
      mprintf("Failed to query interface of output: 0x%X %d\n", hr, GetLastError());
      return -1;
  }
  hr = output1->lpVtbl->DuplicateOutput(output1, device->D3D11device, &device->duplication);
  if (FAILED(hr)) {
      mprintf("Failed to duplicate output: 0x%X %d\n", hr, GetLastError());
      return -1;
  }
  hr = hardware->output->lpVtbl->GetDesc(hardware->output, &hardware->final_output_desc);
  if (FAILED(hr)) {
      mprintf("Failed to getdesc of output: 0x%X %d\n", hr, GetLastError());
      return -1;
  }

  device->width = hardware->final_output_desc.DesktopCoordinates.right;
  device->height = hardware->final_output_desc.DesktopCoordinates.bottom;

  return 0;
}

ID3D11Texture2D* CreateTexture(struct CaptureDevice *device) {
  HRESULT hr;

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

  ID3D11Texture2D *texture;
  hr = device->D3D11device->lpVtbl->CreateTexture2D(device->D3D11device, &tDesc, NULL, &texture);
  if (FAILED(hr)) {
      mprintf("Failed to create Texture2D 0x%X %d\n", hr, GetLastError());
      return NULL;
  }

  device->duplication->lpVtbl->GetDesc(device->duplication, &device->duplication_desc);

  return texture;
}

void ReleaseScreenshot(struct ScreenshotContainer* screenshot) {
    if (screenshot->final_texture != NULL) {
        screenshot->final_texture->lpVtbl->Release(screenshot->final_texture);
        screenshot->final_texture = NULL;
    }

    if (screenshot->desktop_resource != NULL) {
        screenshot->desktop_resource->lpVtbl->Release(screenshot->desktop_resource);
        screenshot->desktop_resource = NULL;
    }

    if (screenshot->staging_texture != NULL) {
        screenshot->staging_texture->lpVtbl->Release(screenshot->staging_texture);
        screenshot->staging_texture = NULL;
    }

    if (screenshot->surface != NULL) {
        screenshot->surface->lpVtbl->Release(screenshot->surface);
        screenshot->surface = NULL;
    }
}

int CaptureScreen(struct CaptureDevice *device) {
  HRESULT hr;

  struct ScreenshotContainer* screenshot = &device->screenshot;

  hr = device->duplication->lpVtbl->ReleaseFrame(device->duplication);

  ReleaseScreenshot(screenshot);

  hr = device->duplication->lpVtbl->AcquireNextFrame(device->duplication, 1, &device->frame_info, &screenshot->desktop_resource);

  if(FAILED(hr)) {
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return 0;
    }
    else if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_INVALID_CALL) {
        mprintf("CaptureScreen returned DXGI_ERROR_ACCESS_LOST or DXGI_ERROR_INVALID_CALL (0x%X)! Recreating device\n", hr);
        SDL_Delay(1);
        return -1;
    }
    else {
        mprintf("Failed to Acquire Next Frame! 0x%X %d\n", hr, GetLastError());
        return -1;
    }
  }

  hr = screenshot->desktop_resource->lpVtbl->QueryInterface(screenshot->desktop_resource, &IID_ID3D11Texture2D, (void**)&screenshot->final_texture);
  if (FAILED(hr)) {
      mprintf("Query Interface Failed!\n");
      return -1;
  }

    int accumulated_frames = device->frame_info.AccumulatedFrames;

    device->counter++;
    hr = device->duplication->lpVtbl->MapDesktopSurface(device->duplication, &screenshot->mapped_rect);

    // If MapDesktopSurface doesn't work, then do it manually
    if(hr == DXGI_ERROR_UNSUPPORTED) {
        screenshot->staging_texture = CreateTexture(device);
        if (screenshot->staging_texture == NULL) {
            // Error already printed inside of CreateTexture
            return -1;
        }
        device->D3D11context->lpVtbl->CopySubresourceRegion(device->D3D11context, (ID3D11Resource*)screenshot->staging_texture, 0, 0, 0, 0,
                                                (ID3D11Resource*)screenshot->final_texture, 0, &device->Box);

        hr = screenshot->staging_texture->lpVtbl->QueryInterface(screenshot->staging_texture, &IID_IDXGISurface, (void**)&screenshot->surface);
        if (FAILED(hr)) {
            mprintf("Query Interface Failed! 0x%X %d\n", hr, GetLastError());
            return -1;
        }
        hr = screenshot->surface->lpVtbl->Map(screenshot->surface, &screenshot->mapped_rect, DXGI_MAP_READ);
        if (FAILED(hr)) {
            mprintf("Map Failed!\n");
            return -1;
        }
        device->did_use_map_desktop_surface = false;
    }
    else if (FAILED(hr)) {
        mprintf("MapDesktopSurface Failed! 0x%X %d\n", hr, GetLastError());
        return -1;
    }
    else {
        device->did_use_map_desktop_surface = true;
    }

    device->frame_data = screenshot->mapped_rect.pBits;
    return accumulated_frames;
}


void ReleaseScreen(struct CaptureDevice *device) {
    HRESULT hr;
  if (device->did_use_map_desktop_surface) {
    hr = device->duplication->lpVtbl->UnMapDesktopSurface(device->duplication);
    if (FAILED(hr)) {
        mprintf("Failed to unmap duplication's desktop surface 0x%X %d\n", hr, GetLastError());
    }
  }
  else {
      struct ScreenshotContainer* screenshot = &device->screenshot;
      hr = screenshot->surface->lpVtbl->Unmap(screenshot->surface);
      if (FAILED(hr)) {
          mprintf("Failed to unmap screenshot surface 0x%X %d\n", hr, GetLastError());
      }
  }
}

void DestroyCaptureDevice(struct CaptureDevice* device) {
    HRESULT hr;

    hr = device->duplication->lpVtbl->ReleaseFrame(device->duplication);

    ReleaseScreenshot(&device->screenshot);

    if (device->duplication) {
        device->duplication->lpVtbl->Release(device->duplication);
        device->duplication = NULL;
    }
    if (device->hardware) {
        free(device->hardware);
        device->hardware = NULL;
    }
}
