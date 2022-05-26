#ifndef WHIST_CLIENT_FRONTEND_SDL_STRUCT_H
#define WHIST_CLIENT_FRONTEND_SDL_STRUCT_H

#include <map>
#include <whist/core/whist.h>
#include "../api.h"
#include "../frontend.h"

typedef struct SDLFrontendVideoContext {
    /**
     * Platform-specific private data associated with the renderer.
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
    int width;
    int height;
    const char* title;
    WhistRGBColor color;
    bool is_fullscreen;
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
        WhistCursorState state;
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
void sdl_d3d11_wait(SDLFrontendContext* context);

/**
 * Create an SDL texture from a D3D11 texture.
 *
 * Moves a D3D11 texture from the decode device to the render device and
 * wraps it in an SDL texture.  This never copies the actual data.
 *
 * @param context  Frontend context containing the devices.
 * @param renderer The renderer that the texture will be bound to.
 * @param frame    Frame containing the D3D11 texture to use.
 * @return  The new SDL texture.
 */
SDL_Texture* sdl_d3d11_create_texture(SDLFrontendContext* context, SDL_Renderer* renderer,
                                      AVFrame* frame);

/**
 * Initialise D3D11 devices for render and decode.
 *
 * Creates the two connected devices and device contexts to use for
 * rendering (with SDL) and decoding (with FFmpeg).
 *
 * @param context  Frontend context containing the SDL renderer.
 * @return  Success or error code.  If this fails, rendering will have
 *          to copy frames rather than using textures directly.
 */
WhistStatus sdl_d3d11_init(SDLFrontendContext* context);

/**
 * Destroy D3D11 devices and clean up.
 *
 * In a debug build this also enumerates all live D3D11 objects to
 * detect memory leaks.
 *
 * @param context  Frontend context containing the devices.
 */
void sdl_d3d11_destroy(SDLFrontendContext* context);

#endif  // WHIST_CLIENT_FRONTEND_SDL_STRUCT_H
