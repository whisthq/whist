//
// Created by mathieu on 28/03/2020.
//

#ifndef CAPTURE_X11CAPTURE_H
#define CAPTURE_X11CAPTURE_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>
#include "stdbool.h"
/*
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
};*/

struct CaptureDevice
{
    Display* display;
    XImage* image;
    XShmSegmentInfo segment;
    Window root;
    int counter;
    int width;
    int height;
    char* frame_data;
    Damage damage;
    int event;
    //struct ScreenshotContainer screenshot;
    bool did_use_map_desktop_surface;
    //struct DisplayHardware *hardware;
    bool released;
};
typedef unsigned int UINT;

int CreateCaptureDevice( struct CaptureDevice* device, UINT width, UINT height );

int CaptureScreen( struct CaptureDevice* device );

void ReleaseScreen( struct CaptureDevice* device );

void DestroyCaptureDevice( struct CaptureDevice* device );


#endif //CAPTURE_X11CAPTURE_H