/*
 * This file contains the headers and definitions of the DXGI screen capture functions.

 Protocol version: 1.0
 Last modification: 1/15/20

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef DXGI_CAPTURE_H
#define DXGI_CAPTURE_H

#include "fractal.h" // contains all the headers

/*
 * This file contains the implementation of DXGI screen capture.

 Protocol version: 1.0
 Last modification: 1/15/2020

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2020
*/

struct ScreenshotContainer {
  IDXGIResource *desktop_resource;
  ID3D11Texture2D *final_texture;
  ID3D11Texture2D* staging_texture;
  DXGI_MAPPED_RECT mapped_rect;
  D3D11_MAPPED_SUBRESOURCE mapped_subresource;
  IDXGISurface *surface;
};

struct DisplayHardware {
    IDXGIAdapter1* adapter;
    IDXGIOutput* output;
    DXGI_OUTPUT_DESC final_output_desc;
};

struct CaptureDevice {
  D3D11_BOX Box;
  ID3D11Device *D3D11device;
  ID3D11DeviceContext *D3D11context;
  IDXGIOutputDuplication *duplication;
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  DXGI_OUTDUPL_DESC duplication_desc;
  int counter;
  int width;
  int height;
  char* frame_data;
  struct ScreenshotContainer screenshot;
  bool did_use_map_desktop_surface;
  struct DisplayHardware *hardware;
};

int CreateCaptureDevice(struct CaptureDevice **device, int width, int height);

int CaptureScreen(struct CaptureDevice *device);

void ReleaseScreen(struct CaptureDevice *device);

void DestroyCaptureDevice(struct CaptureDevice* device);

#endif