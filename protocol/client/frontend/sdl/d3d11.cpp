/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file d3d11.c
 * @brief D3D11 helper functons for SDL client frontend.
 */

#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#include <whist/core/whist.h>
extern "C" {
#include <libavutil/hwcontext_d3d11va.h>
#include "common.h"
}

#include "sdl_struct.hpp"

// A D3D11 device and context pair. Each pair must only be used on one thread.
typedef struct SDLD3D11Context {
    /**
     * D3D11 device renderer.
     */
    ID3D11Device *d3d11_device;
    /**
     * Immediate context used by the renderer.
     *
     * This can only be used on the renderer thread.
     */
    ID3D11DeviceContext *d3d11_context;
} SDLD3D11Context;

WhistStatus sdl_d3d11_init_decoder(SDLFrontendContext *context, ID3D11Device *render_device);

void sdl_d3d11_wait(SDLWindowContext *window_context) {
    SDLD3D11Context *d3d11 = (SDLD3D11Context *)window_context->private_data;
    HRESULT hr;

    D3D11_QUERY_DESC desc = {
        .Query = D3D11_QUERY_EVENT,
        .MiscFlags = 0,
    };
    ID3D11Query *query;
    hr = d3d11->d3d11_device->CreateQuery(&desc, &query);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create query for render completion: %#x.", hr);
        return;
    }

    d3d11->d3d11_context->End((ID3D11Asynchronous *)query);

    // Wait for up to 12ms for this to complete.  If it doesn't finish
    // in that time then continue anyway - a visual artifact is possible
    // here, but we want to keep up the framerate first.
    BOOL done = FALSE;
    for (int i = 0; !done && i < 5; i++) {
        if (i > 0) {
            Sleep(3);
        }
        hr = d3d11->d3d11_context->GetData((ID3D11Asynchronous *)query, &done, sizeof(done), 0);
        if (FAILED(hr)) {
            LOG_ERROR("Failed to query render completion: %#x.", hr);
            break;
        }
    }

    query->Release();
}

SDL_Texture *sdl_d3d11_create_texture(SDLWindowContext *window_context, AVFrame *frame) {
    SDLD3D11Context *d3d11 = (SDLD3D11Context *)window_context->private_data;

    FATAL_ASSERT(frame->format == AV_PIX_FMT_D3D11);

    // This is a pointer to a texture, but it exists on the video decode
    // device rather than the render device.  Make a new reference on
    // the render device to give to SDL.
    ID3D11Texture2D *texture = (ID3D11Texture2D *)frame->data[0];
    int index = (intptr_t)frame->data[1];

    HRESULT hr;
    IDXGIResource1 *resource;
    hr = texture->QueryInterface(&resource);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get DXGI resource for video texture: %#x.", hr);
        return NULL;
    }
    HANDLE shared_handle;
    hr = resource->CreateSharedHandle(NULL, DXGI_SHARED_RESOURCE_READ, NULL, &shared_handle);
    resource->Release();
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create shared handle for video texture: %#x.", hr);
        return NULL;
    }

    ID3D11Device1 *device1;
    hr = d3d11->d3d11_device->QueryInterface(&device1);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get D3D11 device1 for device: %#x.", hr);
        CloseHandle(shared_handle);
        return NULL;
    }

    ID3D11Texture2D *render_texture;
    hr = device1->OpenSharedResource1(shared_handle, IID_PPV_ARGS(&render_texture));
    device1->Release();
    CloseHandle(shared_handle);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to open shared handle for renderer texture: %#x.", hr);
        return NULL;
    }

    SDL_TextureHandleD3D11 d3d11_handle = {
        .texture = render_texture,
        .index = index,
    };

    // The texture dimensions match the coded size of the video, not
    // the cropped size.
    int texture_width = FFALIGN(frame->width, 16);
    int texture_height = FFALIGN(frame->height, 16);

    SDL_Texture *sdl_texture;
    sdl_texture = SDL_CreateTextureFromHandle(window_context->renderer, SDL_PIXELFORMAT_NV12,
                                              SDL_TEXTUREACCESS_STATIC, texture_width,
                                              texture_height, &d3d11_handle);
    if (sdl_texture == NULL) {
        LOG_ERROR("Failed to import video texture: %s.", SDL_GetError());
    }

    // Release the render texture - the SDL texture now contains a
    // reference to it.
    render_texture->Release();

    return sdl_texture;
}

WhistStatus sdl_d3d11_create_window(SDLFrontendContext *context, int id) {
    static bool first_call = true;
    if (!context->windows.contains(id)) {
        LOG_ERROR("Window with ID %d not found!");
        return WHIST_ERROR_NOT_FOUND;
    }

    SDLWindowContext *window_context = context->windows[id];

    SDLD3D11Context *d3d11 = (SDLD3D11Context *)safe_zalloc(sizeof(*d3d11));
    window_context->private_data = (void *)d3d11;

    d3d11->d3d11_device = SDL_RenderGetD3D11Device(window_context->renderer);
    if (d3d11->d3d11_device == NULL) {
        LOG_WARNING("Failed to fetch D3D11 device: %s.", SDL_GetError());
        return WHIST_ERROR_NOT_FOUND;
    }

    d3d11->d3d11_device->GetImmediateContext(&d3d11->d3d11_context);
    if (first_call) {
        sdl_d3d11_init_decoder(context, d3d11->d3d11_device);
        first_call = false;
    }
    return WHIST_SUCCESS;
}

// HELPER: initialize the d3d11 decoder device and context. This is called during the first call to
// sdl_d3d11_create_window, and assumes that all devices created use the same adapter.
WhistStatus sdl_d3d11_init_decoder(SDLFrontendContext *context, ID3D11Device *render_device) {
    int err;
    HRESULT hr;

    SDLD3D11Context *d3d11 = (SDLD3D11Context *)safe_zalloc(sizeof(SDLD3D11Context));
    context->video.private_data = (void *)d3d11;

    // Make an independent device on the same adapter so that we have a
    // separate immediate context to use on the video thread.

    IDXGIDevice *dxgi_device;
    hr = render_device->QueryInterface(&dxgi_device);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to get DXGI device from D3D11 device: %#x.", hr);
        return WHIST_ERROR_EXTERNAL;
    }

    IDXGIAdapter *dxgi_adapter;
    hr = dxgi_device->GetAdapter(&dxgi_adapter);
    dxgi_device->Release();
    if (FAILED(hr)) {
        LOG_WARNING("Failed to get DXGI adapter from DXGI device: %#x.", hr);
        return WHIST_ERROR_EXTERNAL;
    }

    UINT device_flags = 0;
#ifndef NDEBUG
    // Enable the debug layer in debug builds.
    device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    hr = D3D11CreateDevice(dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, device_flags, NULL, 0,
                           D3D11_SDK_VERSION, &d3d11->d3d11_device, NULL, &d3d11->d3d11_context);
    dxgi_adapter->Release();
    if (FAILED(hr)) {
        LOG_WARNING("Failed to create D3D11 device for video: %#x.", hr);
        return WHIST_ERROR_EXTERNAL;
    }

    AVBufferRef *dev_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
    if (!dev_ref) {
        LOG_WARNING("Failed to create D3D11 hardware device for video.");
        return WHIST_ERROR_EXTERNAL;
    }

    AVHWDeviceContext *dev = (AVHWDeviceContext *)dev_ref->data;
    AVD3D11VADeviceContext *hwctx = (AVD3D11VADeviceContext *)dev->hwctx;

    d3d11->d3d11_device->AddRef();
    hwctx->device = d3d11->d3d11_device;
    d3d11->d3d11_context->AddRef();
    hwctx->device_context = d3d11->d3d11_context;

    err = av_hwdevice_ctx_init(dev_ref);
    if (err < 0) {
        char err_buf[1024];
        av_strerror(err, err_buf, sizeof(err_buf));
        LOG_WARNING("Failed to init D3D11 hardware device for video: %s.", err_buf);
        av_buffer_unref(&dev_ref);
        return WHIST_ERROR_EXTERNAL;
    }

    LOG_INFO("Using D3D11 device from SDL renderer for video.");

    context->video.decode_device = dev_ref;
    context->video.decode_format = AV_PIX_FMT_D3D11;
    return WHIST_SUCCESS;
}

void sdl_d3d11_destroy_window(SDLFrontendContext *context, int id) {
    if (!context->windows.contains(id)) {
        // Nothing to do, probably a race condition
        LOG_ERROR("Tried to destroy D3D11 context for window %d that doesn't exist!", id);
        return;
    }
    SDLWindowContext *window_context = context->windows[id];
    SDLD3D11Context *d3d11 = (SDLD3D11Context *)window_context->private_data;

    if (!d3d11) {
        // Nothing to do (init probably failed).
        return;
    }

    if (d3d11->d3d11_device) {
        d3d11->d3d11_device->Release();
        d3d11->d3d11_device = NULL;
    }
    if (d3d11->d3d11_context) {
        d3d11->d3d11_context->Release();
        d3d11->d3d11_context = NULL;
    }
}

void sdl_d3d11_destroy(SDLFrontendContext *context) {
    SDLD3D11Context *d3d11 = (SDLD3D11Context *)context->video.private_data;

    if (!d3d11) {
        // Nothing to do (init probably failed).
        return;
    }

    if (d3d11->d3d11_device) {
        d3d11->d3d11_device->Release();
        d3d11->d3d11_device = NULL;
    }
    if (d3d11->d3d11_context) {
        d3d11->d3d11_context->Release();
        d3d11->d3d11_context = NULL;
    }

#ifndef NDEBUG
    // In debug mode, enumerate all live D3D11 objects since we should
    // have destroyed them all by now.
    IDXGIDebug *dxgi_debug;
    HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug));
    if (FAILED(hr)) {
        // Ignore - probably missing some debug feature (e.g. being run
        // on a device without the SDK installed).
    } else {
        dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        dxgi_debug->Release();
    }
#endif
}
