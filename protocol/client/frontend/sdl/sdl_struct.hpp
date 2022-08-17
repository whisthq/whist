#ifndef WHIST_CLIENT_FRONTEND_SDL_STRUCT_H
#define WHIST_CLIENT_FRONTEND_SDL_STRUCT_H

#include <whist/core/whist.h>

extern "C" {
#include "../api.h"
#include "../frontend.h"
}

#include <map>
#include <string>

typedef struct SDLFrontendVideoContext {
    /**
     * Platform-specific private data associated with the video decoder.
     */
    void* private_data;
    /**
     * Video device compatible with the renderer.
     *
     * If set, decode must happen on this device for video textures to
     * be used directly in rendering.
     */
    AVBufferRef* decode_device;
    /**
     * Video format supported natively.
     *
     * If set, the decode format in which the renderer can accept
     * textures to be used directly.
     */
    enum AVPixelFormat decode_format;
    /**
     * The width of the frame in the current video texture.
     */
    int frame_width;
    /**
     * The height of the frame in the current video texture.
     */
    int frame_height;
    /**
     * A reference to the current video frame data.
     *
     * If the frame is in GPU memory then this reference exists to stop
     * the decoder from reusing the frame.  If frames are in CPU memory
     * then this is not used, because the data is all copied.
     */
    AVFrame* frame_reference;
} SDLFrontendVideoContext;

// All the information needed for the frontend to render a specific window
typedef struct SDLWindowContext {
    /**
     * Platform-specific private data associated with the video decoder.
     */
    void* private_data;
    // flags for Whist
    bool to_be_created;
    bool to_be_destroyed;
    // TODO: are these needed now that we only open windows on demand?
    bool video_has_rendered;
    bool window_has_shown;
    // window specific data
    Uint32 window_id;  // the SDL window ID for SDL window events
    SDL_Window* window;
    SDL_Renderer* renderer;
    /**
     * The current video texture.
     *
     * When the video is updated this either copies the frame data to
     * the existing texture (if the data is in CPU memory), or it
     * destroys the existing texture and creates a new one corresponding
     * the frame (if the data is already in GPU memroy).
     */
    SDL_Texture* texture;
    /**
     * The format of the current video texture.
     */
    SDL_PixelFormatEnum texture_format;
    // TODO: dump this into a WhistWindowData
    int x;
    int y;
    // the width and height of the window, as communicated by the server (actual pixels)
    int width;
    int height;
    // the width and height of the SDL window (actual pixels)
    int sdl_width;
    int sdl_height;
    std::string title;
    WhistRGBColor color;
    bool is_fullscreen;
    bool has_titlebar;
    bool is_resizable;
} SDLWindowContext;

typedef struct SDLFrontendContext {
    SDL_AudioDeviceID audio_device;

    std::map<int, SDLWindowContext*> windows;
    /**
     * Name of the render driver.
     */
    const char* render_driver_name;
    /**
     * Video rendering state.
     */
    SDLFrontendVideoContext video;
    /**
     * Event ID used for internal events;
     */
    uint32_t internal_event_id;
    struct {
        WhistMouseMode mode;
        struct {
            int x;
            int y;
        } last_visible_position;
        uint32_t hash;
        SDL_Cursor* handle;
    } cursor;
    const uint8_t* key_state;
    int key_count;
    void* file_drag_data;
    bool video_has_rendered;
    bool window_has_shown;
    /**
     * The cached pixel width of the window, updated on resize events.
     */
    int latest_pixel_width;
    /**
     * The cached pixel height of the window, updated on resize events.
     */
    int latest_pixel_height;
    /**
     * A reference to the thread which parses STDIN for parameter and URL events.
     */
    WhistThread stdin_parser_thread;
    /**
     * A boolean signal to kill the stdin parser thread.
     */
    bool kill_stdin_parser;
} SDLFrontendContext;

// D3D11 helper functions (Windows only).

/**
 * Wait for asynchronous rendering to finish.
 *
 * The D3D11 SDL render path is entirely asynchronous, with no
 * synchronisation points at all.  If we want to do anything on another
 * thread (using a different device context), we need some way to ensure
 * that a render involving a texture has completed before we can touch
 * it elsewhere.
 *
 * @param context  Frontend context containing renderer to wait for.
 */
void sdl_d3d11_wait(SDLWindowContext* window_context);

/**
 * Create an SDL texture from a D3D11 texture.
 *
 * Moves a D3D11 texture from the decode device to the render device and
 * wraps it in an SDL texture.  This never copies the actual data.
 *
 * @param context  Frontend context containing the devices.
 * @param window_context The window that the texture will be bound to.
 * @param frame    Frame containing the D3D11 texture to use.
 * @return  The new SDL texture.
 */
SDL_Texture* sdl_d3d11_create_texture(SDLWindowContext* window_context, AVFrame* frame);

/**
 * Initialise D3D11 device and device context for rendering window id.
 * All devices and device contexts should use the same adapter.
 * The first call to this function will also create the video decoder device and context.
 *
 * Creates the device and device context to use for
 * rendering (with SDL).
 *
 * @param context  Frontend context to initialize D3D11 with.
 * @param id The ID of the window for the device and context.
 * @return  Success or error code.  If this fails, rendering will have
 *          to copy frames rather than using textures directly.
 */
WhistStatus sdl_d3d11_init_window_data(SDLFrontendContext* context, int id);

/**
 * Destroy D3D11 device and context for given window.
 *
 * @param context  Frontend context containing the device.
 * @param id       ID of the window containing the device.
 */
void sdl_d3d11_destroy_window_data(SDLFrontendContext* context, int id);

/**
 * Destroy D3D11 video device and context and clean up. This device should be the last to go.
 *
 * In a debug build this also enumerates all live D3D11 objects to
 * detect memory leaks.
 *
 * @param context  Frontend context containing the devices.
 */
void sdl_d3d11_destroy(SDLFrontendContext* context);

WhistStatus sdl_create_window(WhistFrontend* frontend, int id);

void sdl_destroy_window(WhistFrontend* frontend, int id);

int sdl_get_dpi_scale(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_SDL_STRUCT_H
