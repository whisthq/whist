#ifndef DXGI_CAPTURE_H
#define DXGI_CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file dxgicapture.h
 * @brief This file contains the code to capture a Windows screen in the GPU via
 *        Windows DXGI API.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#pragma warning(disable : 4201)
#include <D3D11.h>
#include <D3d11_1.h>
#include <DXGITYPE.h>
#include <dxgi1_2.h>

#include <fractal/core/fractal.h>
#include <fractal/utils/color.h>

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
    bool texture_on_gpu;
    struct DisplayHardware* hardware;
    bool released;
    MONITORINFOEXW monitorInfo;
    char* bitmap;
    bool using_nvidia;
    bool dxgi_cuda_available;
    FractalRGBColor corner_color;
} CaptureDevice;

#endif  // DXGI_CAPTURE_H
