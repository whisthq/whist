#ifndef DXGI_CAPTURE_H
#define DXGI_CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file dxgicapture.h
 * @brief This file contains the code to capture a Windows screen in the GPU via Windows DXGI API.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Custom types
============================
*/

typedef struct ScreenshotContainer {
  IDXGIResource* desktop_resource;
  ID3D11Texture2D* final_texture;
  ID3D11Texture2D* staging_texture;
  DXGI_MAPPED_RECT mapped_rect;
  D3D11_MAPPED_SUBRESOURCE mapped_subresource;
  IDXGISurface* surface;
} ScreenshotContainer;

typedef struct DisplayHardware {
  IDXGIAdapter1* adapter;
  IDXGIOutput* output;
  DXGI_OUTPUT_DESC final_output_desc;
} DisplayHardware;

typedef struct CaptureDevice {
  D3D11_BOX Box;
  ID3D11Device* D3D11device;
  ID3D11DeviceContext* D3D11context;
  IDXGIOutputDuplication* duplication;
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  DXGI_OUTDUPL_DESC duplication_desc;
  int counter;
  int width;
  int height;
  int pitch;
  char* frame_data;
  struct ScreenshotContainer screenshot;
  bool did_use_map_desktop_surface;
  struct DisplayHardware* hardware;
  bool released;
  MONITORINFOEXW monitorInfo;
  char* bitmap;
} CaptureDevice;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Create a DXGI screen capture device using D3D11
 *
 * @param device                   Capture device struct to hold the capture device
 * @param width                    Width of the screen to capture, in pixels
 * @param height                   Height of the screen to capture, in pixels
 *
 * @returns                        0 if succeeded, else -1     
 */
int CreateCaptureDevice(CaptureDevice* device, UINT width, UINT height);

/**
 * @brief                          Capture a bitmap snapshot of the screen, in the GPU, using DXGI
 *
 * @param device                   The device used to capture the screen
 *
 * @returns                        0 if succeeded, else -1     
 */
int CaptureScreen(CaptureDevice* device);

/**
 * @brief                          Release a captured screen bitmap snapshot
 *
 * @param fmsg                     The Windows screencapture device holding the screen object captured
 */
void ReleaseScreen(CaptureDevice* device);

/**
 * @brief                          Destroys and frees the memory of a Windows screencapture device
 *
 * @param device                   The Windows screencapture device to destroy and free the memory of
 */
void DestroyCaptureDevice(CaptureDevice* device);

#endif  // DXGI_CAPTURE_H