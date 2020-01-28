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

// define encoder struct to pass as a type
typedef struct {
  IDXGIResource * resource;
  IDXGIOutputDuplication *duplication;
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  DXGI_MAPPED_RECT frame_data;
  int width;
  int height;
} DXGIDevice;

ID3D11Device *createDirect3D11Device(IDXGIAdapter1 *pOutputAdapter);

IDXGIOutput1 *findAttachedOutput(IDXGIFactory1 *factory);

int CreateDXGIDevice(DXGIDevice *device);

int DestroyDXGIDevice(DXGIDevice* device);

HRESULT CaptureScreen(DXGIDevice *device);

HRESULT ReleaseScreen(DXGIDevice *device);

#endif // DXGI_CAPTURE_H
