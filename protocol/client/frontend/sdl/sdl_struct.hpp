#ifndef WHIST_CLIENT_FRONTEND_SDL_STRUCT_H
#define WHIST_CLIENT_FRONTEND_SDL_STRUCT_H

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

typedef struct SDLFrontendContext {
    SDL_AudioDeviceID audio_device;
    SDL_Window* window;
    SDL_Renderer* renderer;
    /**
     * Name of the render driver.
     */
    const char* render_driver_name;
    /**
     * Video rendering state.
     */
    SDLFrontendVideoContext video;
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
    uint32_t file_drag_event_id;
    void* file_drag_data;
    bool video_has_rendered;
    bool window_has_shown;
} SDLFrontendContext;

#endif // WHIST_CLIENT_FRONTEND_SDL_STRUCT_H
