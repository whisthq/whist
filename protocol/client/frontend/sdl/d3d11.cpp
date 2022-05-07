/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file d3d11.c
 * @brief D3D11 helper functons for SDL client frontend.
 */

extern "C" {
#ifdef _WIN32
#define COBJMACROS
#include <libavutil/hwcontext_d3d11va.h>
#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#endif
}

#include "common.h"
#include "sdl_struct.hpp"

typedef struct SDLRenderD3D11Context {
    /**
     * D3D11 device used by the renderer.
     */
    ID3D11Device *render_device;
    /**
     * Immediate context used by the renderer.
     *
     * This can only be used on the renderer thread.
     */
    ID3D11DeviceContext *render_context;
    /**
     * D3D11 device used by the video decoder.
     */
    ID3D11Device *video_device;
    /**
     * Immediate context used by the renderer.
     *
     * This can only be used on the video thread.
     */
    ID3D11DeviceContext *video_context;
} SDLRenderD3D11Context;

void sdl_d3d11_wait(void *context) {
    SDLRenderD3D11Context *d3d11 = ((SDLFrontendContext*) context)->video.private_data;
    HRESULT hr;

    D3D11_QUERY_DESC desc = {
        .Query = D3D11_QUERY_EVENT,
        .MiscFlags = 0,
    };
    ID3D11Query *query;
    hr = ID3D11Device_CreateQuery(d3d11->render_device, &desc, &query);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create query for render completion: %#x.", hr);
        return;
    }

    ID3D11DeviceContext_End(d3d11->render_context, (ID3D11Asynchronous *)query);

    // Wait for up to 12ms for this to complete.  If it doesn't finish
    // in that time then continue anyway - a visual artifact is possible
    // here, but we want to keep up the framerate first.
    BOOL done = FALSE;
    for (int i = 0; !done && i < 5; i++) {
        if (i > 0) {
            Sleep(3);
        }
        hr = ID3D11DeviceContext_GetData(d3d11->render_context, (ID3D11Asynchronous *)query, &done,
                                         sizeof(done), 0);
        if (FAILED(hr)) {
            LOG_ERROR("Failed to query render completion: %#x.", hr);
            break;
        }
    }

    ID3D11Query_Release(query);
}

// TODO: either make this only make a texture with the first window or figure something else out
SDL_Texture *sdl_d3d11_create_texture(void *context, AVFrame *frame) {
    SDLRenderD3D11Context *d3d11 = ((SDLFrontendContext*) context)->video.private_data;

    // This is a pointer to a texture, but it exists on the video decode
    // device rather than the render device.  Make a new reference on
    // the render device to give to SDL.
    ID3D11Texture2D *texture = (ID3D11Texture2D *)frame->data[0];
    int index = (intptr_t)frame->data[1];

    HRESULT hr;
    IDXGIResource1 *resource;
    hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGIResource1, (void **)&resource);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get DXGI resource for video texture: %#x.", hr);
        return NULL;
    }
    HANDLE shared_handle;
    hr = IDXGIResource1_CreateSharedHandle(resource, NULL, DXGI_SHARED_RESOURCE_READ, NULL,
                                           &shared_handle);
    IDXGIResource1_Release(resource);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create shared handle for video texture: %#x.", hr);
        return NULL;
    }

    ID3D11Device1 *device1;
    hr = ID3D11Device_QueryInterface(d3d11->render_device, &IID_ID3D11Device1, (void **)&device1);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get D3D11 device1 for device: %#x.", hr);
        CloseHandle(shared_handle);
        return NULL;
    }

    ID3D11Texture2D *render_texture;
    hr = ID3D11Device1_OpenSharedResource1(device1, shared_handle, &IID_ID3D11Texture2D,
                                           (void **)&render_texture);
    ID3D11Device1_Release(device1);
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
    // TODO: FIX THIS and determine which renderer we should use
    sdl_texture = SDL_CreateTextureFromHandle(((SDLFrontendContext*) context)->windows[0]->renderer, SDL_PIXELFORMAT_NV12,
                                              SDL_TEXTUREACCESS_STATIC, texture_width,
                                              texture_height, &d3d11_handle);
    if (sdl_texture == NULL) {
        LOG_ERROR("Failed to import video texture: %s.", SDL_GetError());
    }

    // Release the render texture - the SDL texture now contains a
    // reference to it.
    ID3D11Texture2D_Release(render_texture);

    return sdl_texture;
}

// TODO: FIX RENDERER CHOICE HERE TOO
WhistStatus sdl_d3d11_init(void *context) {
    int err;
    HRESULT hr;

    SDLRenderD3D11Context *d3d11 = safe_zalloc(sizeof(*d3d11));
    ((SDLFrontendContext*) context)->video.private_data = d3d11;

    d3d11->render_device = SDL_RenderGetD3D11Device(((SDLFrontendContext*) context)->windows[0]->renderer);
    if (d3d11->render_device == NULL) {
        LOG_WARNING("Failed to fetch D3D11 device: %s.", SDL_GetError());
        return WHIST_ERROR_NOT_FOUND;
    }

    ID3D11Device_GetImmediateContext(d3d11->render_device, &d3d11->render_context);

    // Make an independent device on the same adapter so that we have a
    // separate immediate context to use on the video thread.

    IDXGIDevice *dxgi_device;
    hr = ID3D11Device_QueryInterface(d3d11->render_device, &IID_IDXGIDevice, (void **)&dxgi_device);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to get DXGI device from D3D11 device: %#x.", hr);
        return WHIST_ERROR_EXTERNAL;
    }

    IDXGIAdapter *dxgi_adapter;
    hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
    IDXGIDevice_Release(dxgi_device);
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
                           D3D11_SDK_VERSION, &d3d11->video_device, NULL, &d3d11->video_context);
    IDXGIAdapter_Release(dxgi_adapter);
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
    AVD3D11VADeviceContext *hwctx = dev->hwctx;

    ID3D11Device_AddRef(d3d11->video_device);
    hwctx->device = d3d11->video_device;
    ID3D11Device_AddRef(d3d11->video_context);
    hwctx->device_context = d3d11->video_context;

    err = av_hwdevice_ctx_init(dev_ref);
    if (err < 0) {
        LOG_WARNING("Failed to init D3D11 hardware device for video: %s.", av_err2str(err));
        av_buffer_unref(&dev_ref);
        return WHIST_ERROR_EXTERNAL;
    }

    LOG_INFO("Using D3D11 device from SDL renderer for video.");

    context->video.decode_device = dev_ref;
    context->video.decode_format = AV_PIX_FMT_D3D11;
    return WHIST_SUCCESS;
}

void sdl_d3d11_destroy(void *context) {
    SDLRenderD3D11Context *d3d11 = ((SDLFrontendContext*) context)->video.private_data;

    if (!d3d11) {
        // Nothing to do (init probably failed).
        return;
    }

    if (d3d11->render_device) {
        ID3D11Device_Release(d3d11->render_device);
        d3d11->render_device = NULL;
    }
    if (d3d11->render_context) {
        ID3D11DeviceContext_Release(d3d11->render_context);
        d3d11->render_context = NULL;
    }

    if (d3d11->video_device) {
        ID3D11Device_Release(d3d11->video_device);
        d3d11->video_device = NULL;
    }
    if (d3d11->video_context) {
        ID3D11DeviceContext_Release(d3d11->video_context);
        d3d11->video_context = NULL;
    }

#ifndef NDEBUG
    // In debug mode, enumerate all live D3D11 objects since we should
    // have destroyed them all by now.
    IDXGIDebug *dxgi_debug;
    HRESULT hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug, (void **)&dxgi_debug);
    if (FAILED(hr)) {
        // Ignore - probably missing some debug feature (e.g. being run
        // on a device without the SDK installed).
    } else {
        IDXGIDebug_ReportLiveObjects(dxgi_debug, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        IDXGIDebug_Release(dxgi_debug);
    }
#endif
}
