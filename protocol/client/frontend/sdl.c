#include "frontend.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>
#include <whist/utils/command_line.h>
#include <whist/utils/lodepng.h>

// TODO: Refactor filesystem to remove upwards include.
#include "../native_window_utils.h"

// Little-endian RGBA masks
#define RGBA_R 0xff000000
#define RGBA_G 0x00ff0000
#define RGBA_B 0x0000ff00
#define RGBA_A 0x000000ff

/*
============================
Command-line options
============================
*/

static bool skip_taskbar;
static char* icon_png_filename;
COMMAND_LINE_BOOL_OPTION(skip_taskbar, 0, "sdl-skip-taskbar",
                         "Launch the protocol without displaying an icon in the taskbar.")
COMMAND_LINE_STRING_OPTION(icon_png_filename, 'i', "icon", WHIST_ARGS_MAXLEN,
                           "Set the protocol window icon from a 64x64 pixel png file.")

/*
============================
Public Function Implementations
============================
*/

typedef struct SDLFrontendContext {
    SDL_AudioDeviceID audio_device;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
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
} SDLFrontendContext;

static atomic_int sdl_atexit_initialized = ATOMIC_VAR_INIT(0);
static const WhistRGBColor background_color = {17, 24, 39};  // #111827 (thanks copilot)

static WhistStatus sdl_init_frontend(WhistFrontend* frontend, int width, int height,
                                     const char* title) {
    // Only the first init does anything; subsequent ones update an internal
    // refcount.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        LOG_ERROR("Could not initialize SDL - %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

    // We only need to SDL_Quit once, regardless of the subsystem refcount.
    // SDL_QuitSubSystem() would require us to register an atexit for each
    // call.
    if (atomic_fetch_or(&sdl_atexit_initialized, 1) == 0) {
        // If we have initialized SDL, then on exit we should clean up.
        atexit(SDL_Quit);
    }

    // Make sure that ctrl+click is processed as a right click on Mac
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");

    // Allow inactive protocol to trigger the screensaver
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

    // Initialize the renderer
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, VSYNC_ON ? "1" : "0");

#ifdef _WIN32
    // Tell Windows that the content we're rendering is aware of the system's set DPI.
    // Note: This fails on subsequent calls, but that's just a no-op so it's fine.
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

    // Ensure that Windows uses the D3D11 driver rather than D3D9.
    // (The D3D9 driver does work, but it does not support the NV12
    // textures that we use, so performance with it is terrible.)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#endif  // _WIN32

    // Allow the screensaver to activate while the frontend is running
    SDL_EnableScreenSaver();

    bool start_maximized = (width == 0 && height == 0);

    // Grab the default display dimensions. If width or height are 0, then
    // we'll set that dimension to half of the display's dimension. If the
    // window starts maximized and the user then unmaximizes (double click
    // the titlebar on macOS), then the window will be restored to these
    // dimensions.
    SDL_DisplayMode display_info;
    if (SDL_GetDesktopDisplayMode(0, &display_info) != 0) {
        LOG_ERROR("Could not get display mode: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

    if (width == 0) {
        width = display_info.w / 2;
    }
    if (height == 0) {
        height = display_info.h / 2;
    }

    if (skip_taskbar) {
        // Hide the taskbar on macOS directly
        hide_native_window_taskbar();
    }

    uint32_t window_flags = 0;
    window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    window_flags |= SDL_WINDOW_OPENGL;
    window_flags |= SDL_WINDOW_RESIZABLE;
    // Avoid glitchy-looking titlebar-content combinations while the
    // window is loading, and glitches caused by early user interaction.
    window_flags |= SDL_WINDOW_HIDDEN;
    if (start_maximized) {
        window_flags |= SDL_WINDOW_MAXIMIZED;
    }
    if (skip_taskbar) {
        // Hide the taskbar on Windows/Linux
        window_flags |= SDL_WINDOW_SKIP_TASKBAR;
    }

    if (title == NULL) {
        // Default window title is "Whist"
        title = "Whist";
    }

    uint32_t renderer_flags = 0;
    renderer_flags |= SDL_RENDERER_ACCELERATED;
    if (VSYNC_ON) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    SDLFrontendContext* context = safe_malloc(sizeof(SDLFrontendContext));
    frontend->context = context;

    context->audio_device = 0;
    context->key_state = SDL_GetKeyboardState(&context->key_count);

    context->cursor.state = CURSOR_STATE_VISIBLE;
    context->cursor.hash = 0;
    context->cursor.last_visible_position.x = 0;
    context->cursor.last_visible_position.y = 0;
    context->cursor.handle = NULL;

    context->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                                       height, window_flags);
    if (context->window == NULL) {
        LOG_ERROR("Could not create window: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

    context->renderer = SDL_CreateRenderer(context->window, -1, renderer_flags);
    if (context->renderer == NULL) {
        LOG_ERROR("Could not create renderer: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    SDL_SetRenderDrawBlendMode(context->renderer, SDL_BLENDMODE_BLEND);

    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(context->renderer, &info) != 0) {
        LOG_ERROR("Could not get renderer info: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    LOG_INFO("Using renderer: %s", info.name);

    frontend->call->paint_solid(frontend, &background_color);
    frontend->call->render(frontend);
    frontend->call->set_titlebar_color(frontend, &background_color);

    context->texture =
        SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING,
                          MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
    if (context->texture == NULL) {
        LOG_ERROR("Could not create texture: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

    // TODO: Build out this functionality.
    // if (icon_png_filename != NULL) {
    //     SDL_Surface* icon_surface = sdl_surface_from_png_file(icon_filename);
    //     SDL_SetWindowIcon(sdl_window, icon_surface);
    //     sdl_free_png_file_rgb_surface(icon_surface);
    // }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Pump the event loop until the window is actually initialized (mainly macOS).
    }

    // Safe to set these post-initialization.
    init_native_window_options(context->window);
    SDL_SetWindowMinimumSize(context->window, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);

    while (SDL_PollEvent(&event)) {
        // Pump the event loop until the window fully finishes loading.
    }

    // Show the window now that it's fully loaded.
    SDL_ShowWindow(context->window);

    return WHIST_SUCCESS;
}

static void sdl_destroy_frontend(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;

    if (context->texture != NULL) {
        SDL_DestroyTexture(context->texture);
        context->texture = NULL;
    }

    if (context->renderer != NULL) {
        SDL_DestroyRenderer(context->renderer);
        context->renderer = NULL;
    }

    if (context->window != NULL) {
        SDL_DestroyWindow(context->window);
        context->window = NULL;
    }

    free(context);
}

static void sdl_open_audio(WhistFrontend* frontend, unsigned int frequency, unsigned int channels) {
    SDLFrontendContext* context = frontend->context;
    // Verify that the device does not already exist.
    FATAL_ASSERT(context->audio_device == 0);
    SDL_AudioSpec desired_spec = {
        .freq = frequency,
        .format = AUDIO_F32SYS,
        .channels = channels,
        // Must be a power of two. The value doesn't matter since
        // it only affects the size of the callback buffer, which
        // we don't use.
        .samples = 1024,
        .callback = NULL,
        .userdata = NULL,
    };
    SDL_AudioSpec obtained_spec;
    context->audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &obtained_spec, 0);
    if (context->audio_device == 0) {
        LOG_ERROR("Could not open audio device - %s", SDL_GetError());
        return;
    }

    // Verify that the obtained spec matches the desired spec, as
    // we currently do not perform resampling on ingress.
    FATAL_ASSERT(obtained_spec.freq == desired_spec.freq);
    FATAL_ASSERT(obtained_spec.format == desired_spec.format);
    FATAL_ASSERT(obtained_spec.channels == desired_spec.channels);

    // Start the audio device.
    SDL_PauseAudioDevice(context->audio_device, 0);
}

static bool sdl_audio_is_open(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    return context->audio_device != 0;
}

static void sdl_close_audio(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    if (frontend->call->audio_is_open(frontend)) {
        SDL_CloseAudioDevice(context->audio_device);
        context->audio_device = 0;
    }
}

static WhistStatus sdl_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    SDLFrontendContext* context = frontend->context;
    if (!frontend->call->audio_is_open(frontend)) {
        return WHIST_ERROR_NOT_FOUND;
    }
    if (SDL_QueueAudio(context->audio_device, data, (int)size) < 0) {
        LOG_ERROR("Could not queue audio - %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    return WHIST_SUCCESS;
}

static size_t sdl_get_audio_buffer_size(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    if (!frontend->call->audio_is_open(frontend)) {
        return 0;
    }
    return SDL_GetQueuedAudioSize(context->audio_device);
}

static void sdl_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height) {
    SDLFrontendContext* context = frontend->context;
    SDL_GL_GetDrawableSize(context->window, width, height);
}

static void sdl_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height) {
    SDLFrontendContext* context = frontend->context;
    SDL_GetWindowSize(context->window, width, height);
}

static void sdl_get_window_position(WhistFrontend* frontend, int* x, int* y) {
    SDLFrontendContext* context = frontend->context;
    SDL_GetWindowPosition(context->window, x, y);
}

static WhistStatus sdl_get_window_display_index(WhistFrontend* frontend, int* index) {
    SDLFrontendContext* context = frontend->context;
    int ret = SDL_GetWindowDisplayIndex(context->window);
    if (ret < 0) {
        LOG_ERROR("Could not get window display index - %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    if (index != NULL) {
        *index = ret;
    }
    return WHIST_SUCCESS;
}

static int sdl_get_window_dpi(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    return get_native_window_dpi(context->window);
}

static bool sdl_is_window_visible(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    return !(SDL_GetWindowFlags(context->window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED));
}

static void temp_sdl_set_window(WhistFrontend* frontend, void* window) {
    SDLFrontendContext* context = frontend->context;
    context->window = (SDL_Window*)window;
}

static WhistStatus sdl_set_title(WhistFrontend* frontend, const char* title) {
    SDLFrontendContext* context = frontend->context;
    if (context->window == NULL) {
        return WHIST_ERROR_NOT_FOUND;
    }
    SDL_SetWindowTitle(context->window, title);
    return WHIST_SUCCESS;
}

static bool sdl_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) {
    if (!event) {
        return SDL_PollEvent(NULL) != 0;
    }

    // We cannot use SDL_WaitEventTimeout here, because
    // Linux seems to treat a 1ms timeout as an infinite timeout
    SDL_Event sdl_event;
    if (!SDL_PollEvent(&sdl_event)) {
        return false;
    }

    memset(event, 0, sizeof(WhistFrontendEvent));

    switch (sdl_event.type) {
        case SDL_WINDOWEVENT: {
            switch (sdl_event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                    event->type = FRONTEND_EVENT_RESIZE;
                    event->resize.width = sdl_event.window.data1;
                    event->resize.height = sdl_event.window.data2;
                    break;
                }
                case SDL_WINDOWEVENT_LEAVE: {
                    event->type = FRONTEND_EVENT_MOUSE_LEAVE;
                    break;
                }
#ifdef __APPLE__
                case SDL_WINDOWEVENT_OCCLUDED:
                case SDL_WINDOWEVENT_UNOCCLUDED:
#else
                case SDL_WINDOWEVENT_MINIMIZED:
                case SDL_WINDOWEVENT_RESTORED:
#endif  // defined(__APPLE__)
                {
                    event->type = FRONTEND_EVENT_VISIBILITY;
                    event->visibility.visible =
                        (sdl_event.window.event == SDL_WINDOWEVENT_UNOCCLUDED ||
                         sdl_event.window.event == SDL_WINDOWEVENT_RESTORED);
                    break;
                }
                default: {
                    event->type = FRONTEND_EVENT_UNHANDLED;
                    break;
                }
            }
            break;
        }
        case SDL_AUDIODEVICEADDED:
        case SDL_AUDIODEVICEREMOVED: {
            event->type = FRONTEND_EVENT_AUDIO_UPDATE;
            break;
        }
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            event->type = FRONTEND_EVENT_KEYPRESS;
            event->keypress.code = (WhistKeycode)sdl_event.key.keysym.scancode;
            event->keypress.pressed = (sdl_event.type == SDL_KEYDOWN);
            event->keypress.mod = sdl_event.key.keysym.mod;
            break;
        }
        case SDL_MOUSEMOTION: {
            event->type = FRONTEND_EVENT_MOUSE_MOTION;
            event->mouse_motion.absolute.x = sdl_event.motion.x;
            event->mouse_motion.absolute.y = sdl_event.motion.y;
            event->mouse_motion.relative.x = sdl_event.motion.xrel;
            event->mouse_motion.relative.y = sdl_event.motion.yrel;
            event->mouse_motion.relative_mode = SDL_GetRelativeMouseMode();
            break;
        }
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: {
            event->type = FRONTEND_EVENT_MOUSE_BUTTON;
            event->mouse_button.button = sdl_event.button.button;
            event->mouse_button.pressed = (sdl_event.type == SDL_MOUSEBUTTONDOWN);
            if (event->mouse_button.button == MOUSE_L) {
                // Capture the mouse while the left mouse button is pressed.
                // This lets SDL track the mouse position even when the drag
                // extends outside the window.
                SDL_CaptureMouse(event->mouse_button.pressed);
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            event->type = FRONTEND_EVENT_MOUSE_WHEEL;
            event->mouse_wheel.momentum_phase =
                (WhistMouseWheelMomentumType)sdl_event.wheel.momentum_phase;
            event->mouse_wheel.delta.x = sdl_event.wheel.x;
            event->mouse_wheel.delta.y = sdl_event.wheel.y;
            event->mouse_wheel.precise_delta.x = sdl_event.wheel.preciseX;
            event->mouse_wheel.precise_delta.y = sdl_event.wheel.preciseY;
            break;
        }
        case SDL_MULTIGESTURE: {
            event->type = FRONTEND_EVENT_GESTURE;
            event->gesture.num_fingers = sdl_event.mgesture.numFingers;
            event->gesture.delta.theta = sdl_event.mgesture.dTheta;
            event->gesture.delta.dist = sdl_event.mgesture.dDist;
            event->gesture.center.x = sdl_event.mgesture.x;
            event->gesture.center.y = sdl_event.mgesture.y;
            event->gesture.type = MULTIGESTURE_NONE;
            break;
        }
        case SDL_PINCH: {
            event->type = FRONTEND_EVENT_GESTURE;
            event->gesture.num_fingers = 2;
            event->gesture.delta.theta = 0;
            event->gesture.delta.dist = sdl_event.pinch.scroll_amount;
            event->gesture.center.x = 0;
            event->gesture.center.y = 0;
            event->gesture.type = MULTIGESTURE_NONE;
            if (sdl_event.pinch.magnification < 0) {
                event->gesture.type = MULTIGESTURE_PINCH_CLOSE;
            } else if (sdl_event.pinch.magnification > 0) {
                event->gesture.type = MULTIGESTURE_PINCH_OPEN;
            }
            break;
        }
        case SDL_DROPFILE: {
            event->type = FRONTEND_EVENT_FILE_DROP;
            event->file_drop.filename = strdup(sdl_event.drop.file);
            SDL_free(sdl_event.drop.file);
            // Get the global mouse position of the drop event.
            SDL_CaptureMouse(true);
            SDL_GetMouseState(&event->file_drop.position.x, &event->file_drop.position.y);
            SDL_CaptureMouse(false);
            break;
        }
        case SDL_QUIT: {
            event->type = FRONTEND_EVENT_QUIT;
            event->quit.quit_application = sdl_event.quit.quit_app;
            break;
        }
        default: {
            event->type = FRONTEND_EVENT_UNHANDLED;
            break;
        }
    }

    return true;
}

static void sdl_get_global_mouse_position(WhistFrontend* frontend, int* x, int* y) {
    SDL_GetGlobalMouseState(x, y);
}

static void sdl_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {
    SDLFrontendContext* context = frontend->context;
    if (cursor == NULL || cursor->hash == context->cursor.hash) {
        return;
    }

    if (cursor->cursor_state != context->cursor.state) {
        if (cursor->cursor_state == CURSOR_STATE_HIDDEN) {
            SDL_GetGlobalMouseState(&context->cursor.last_visible_position.x,
                                    &context->cursor.last_visible_position.y);
            SDL_SetRelativeMouseMode(true);
        } else {
            SDL_SetRelativeMouseMode(false);
            SDL_WarpMouseGlobal(context->cursor.last_visible_position.x,
                                context->cursor.last_visible_position.y);
        }
        context->cursor.state = cursor->cursor_state;
    }

    if (context->cursor.state == CURSOR_STATE_HIDDEN) {
        return;
    }

    SDL_Cursor* sdl_cursor = NULL;
    if (cursor->using_png) {
        uint8_t* rgba = whist_cursor_info_to_rgba(cursor);
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            rgba, cursor->png_width, cursor->png_height, sizeof(uint32_t) * 8,
            sizeof(uint32_t) * cursor->png_width, RGBA_R, RGBA_G, RGBA_B, RGBA_A);
        // This allows for correct blit-resizing.
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

#ifdef _WIN32
        // Windows SDL cursors don't scale to DPI.
        // TODO: https://github.com/libsdl-org/SDL/pull/4927 may change this.
        const int dpi = 96;
#else
        const int dpi = sdl_get_window_dpi(frontend);
#endif  // defined(_WIN32)

        SDL_Surface* scaled = SDL_CreateRGBSurface(
            0, cursor->png_width * 96 / dpi, cursor->png_height * 96 / dpi,
            surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask,
            surface->format->Bmask, surface->format->Amask);

        SDL_BlitScaled(surface, NULL, scaled, NULL);
        SDL_FreeSurface(surface);
        free(rgba);

        // TODO: Consider SDL_SetSurfaceBlendMode here since X11 cursor image
        // is pre-alpha-multiplied.

        sdl_cursor = SDL_CreateColorCursor(scaled, cursor->png_hot_x * 96 / dpi,
                                           cursor->png_hot_y * 96 / dpi);
        SDL_FreeSurface(scaled);
    } else {
        sdl_cursor = SDL_CreateSystemCursor((SDL_SystemCursor)cursor->cursor_id);
    }
    SDL_SetCursor(sdl_cursor);

    // SDL_SetCursor requires that the currently set cursor still exists.
    if (context->cursor.handle != NULL) {
        SDL_FreeCursor(context->cursor.handle);
    }
    context->cursor.handle = sdl_cursor;
    context->cursor.hash = cursor->hash;
}

static void sdl_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state,
                                   int* key_count, int* mod_state) {
    SDLFrontendContext* context = frontend->context;

    // We could technically call SDL_PumpEvents here, but it's not needed unless we
    // find that it gives a latency or performance improvement.

    if (key_state != NULL) {
        *key_state = context->key_state;
    }
    if (key_count != NULL) {
        *key_count = context->key_count;
    }
    if (mod_state != NULL) {
        *mod_state = SDL_GetModState();
    }
}

static void sdl_restore_window(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    SDL_RestoreWindow(context->window);
}

static void sdl_paint_png(WhistFrontend* frontend, const char* filename, int output_width,
                          int output_height, int x, int y) {
    SDLFrontendContext* context = frontend->context;
    unsigned int w, h;
    uint8_t* image;

    // Decode to 32-bit RGBA
    unsigned int error = lodepng_decode32_file(&image, &w, &h, filename);
    if (error) {
        LOG_ERROR("Failed to decode PNG: %s", lodepng_error_text(error));
        return;
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        image, w, h, sizeof(uint32_t) * 8, sizeof(uint32_t) * w, RGBA_R, RGBA_G, RGBA_B, RGBA_A);
    if (surface == NULL) {
        LOG_ERROR("Failed to create surface from PNG: %s", SDL_GetError());
        free(image);
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(context->renderer, surface);
    SDL_FreeSurface(surface);
    free(image);
    if (texture == NULL) {
        LOG_ERROR("Failed to create texture from PNG: %s", SDL_GetError());
        return;
    }

    // TODO: Formalize window position constants.
    if (x == -1) {
        // Center horizontally
        x = (output_width - w) / 2;
    }
    if (y == -1) {
        // Place at bottom
        y = output_height - h;
    }
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderCopy(context->renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
}

static void sdl_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen) {
    SDLFrontendContext* context = frontend->context;
    SDL_SetWindowFullscreen(context->window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

static void sdl_paint_solid(WhistFrontend* frontend, const WhistRGBColor* color) {
    SDLFrontendContext* context = frontend->context;
    SDL_SetRenderDrawColor(context->renderer, color->red, color->green, color->blue,
                           SDL_ALPHA_OPAQUE);
    SDL_RenderClear(context->renderer);
}

// On macOS & Windows, SDL outputs the texture with the last pixel on the bottom and
// right sides without data, rendering it green (NV12 color format). We're not sure
// why that is the case, but in the meantime, clipping that pixel makes the visual
// look seamless.
#if defined(__APPLE__) || defined(_WIN32)
#define CLIPPED_PIXELS 1
#else
#define CLIPPED_PIXELS 0
#endif

static void sdl_paint_avframe(WhistFrontend* frontend, AVFrame* frame, int output_width,
                              int output_height) {
    SDLFrontendContext* context = frontend->context;
    // The texture object we allocate is larger than the frame,
    // so we only copy the valid section of the frame into the texture.
    SDL_Rect texture_rect = {
        .x = 0,
        .y = 0,
        .w = frame->width,
        .h = frame->height,
    };

    // Update the SDLTexture with the given nvdata
    int res;
    if (frame->format == AV_PIX_FMT_NV12) {
        // A normal NV12 frame in CPU memory.
        res = SDL_UpdateNVTexture(context->texture, &texture_rect, frame->data[0],
                                  frame->linesize[0], frame->data[1], frame->linesize[1]);
    } else if (frame->format == AV_PIX_FMT_VIDEOTOOLBOX) {
        // A CVPixelBufferRef containing both planes in GPU memory.
        res = SDL_UpdateNVTexture(context->texture, &texture_rect, frame->data[3], frame->width,
                                  frame->data[3], frame->width);
    } else {
        LOG_ERROR("Unsupported frame format: %s", av_get_pix_fmt_name(frame->format));
        return;
    }
    if (res < 0) {
        LOG_ERROR("Failed to update texture: %s", SDL_GetError());
        return;
    }

    // Take the subsection of texture that should be rendered to screen,
    // and draw it on the renderer
    SDL_Rect output_rect = {
        .x = 0,
        .y = 0,
        .w = min(output_width, texture_rect.w) - CLIPPED_PIXELS,
        .h = min(output_height, texture_rect.h) - CLIPPED_PIXELS,
    };
    SDL_RenderCopy(context->renderer, context->texture, &output_rect, NULL);
}

static void sdl_render(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    SDL_RenderPresent(context->renderer);
}

static void sdl_resize_window(WhistFrontend* frontend, int width, int height) {
    SDLFrontendContext* context = frontend->context;
    SDL_SetWindowSize(context->window, width, height);
}

static void sdl_set_titlebar_color(WhistFrontend* frontend, const WhistRGBColor* color) {
    SDLFrontendContext* context = frontend->context;
    // TODO: Only pass the reference, not the whole struct
    set_native_window_color(context->window, *color);
}

static const WhistFrontendFunctionTable sdl_function_table = {
    .init = sdl_init_frontend,
    .destroy = sdl_destroy_frontend,
    .open_audio = sdl_open_audio,
    .audio_is_open = sdl_audio_is_open,
    .close_audio = sdl_close_audio,
    .queue_audio = sdl_queue_audio,
    .get_audio_buffer_size = sdl_get_audio_buffer_size,
    .get_window_pixel_size = sdl_get_window_pixel_size,
    .get_window_virtual_size = sdl_get_window_virtual_size,
    .get_window_position = sdl_get_window_position,
    .get_window_display_index = sdl_get_window_display_index,
    .get_window_dpi = sdl_get_window_dpi,
    .is_window_visible = sdl_is_window_visible,
    .get_keyboard_state = sdl_get_keyboard_state,
    .temp_set_window = temp_sdl_set_window,
    .set_title = sdl_set_title,
    .poll_event = sdl_poll_event,
    .get_global_mouse_position = sdl_get_global_mouse_position,
    .set_cursor = sdl_set_cursor,
    .restore_window = sdl_restore_window,
    .set_window_fullscreen = sdl_set_window_fullscreen,
    .paint_png = sdl_paint_png,
    .paint_solid = sdl_paint_solid,
    .paint_avframe = sdl_paint_avframe,
    .set_titlebar_color = sdl_set_titlebar_color,
    .render = sdl_render,
    .resize_window = sdl_resize_window,
};

const WhistFrontendFunctionTable* sdl_get_function_table(void) { return &sdl_function_table; }
