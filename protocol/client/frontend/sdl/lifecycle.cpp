extern "C" {
#include "native.h"
#include <whist/utils/command_line.h>
}

#include <whist/utils/atomic.h>
#include "sdl_struct.hpp"

static bool skip_taskbar;
static char* icon_png_filename;
COMMAND_LINE_BOOL_OPTION(
    skip_taskbar, 0, "sdl-skip-taskbar",
    "Launch the protocol without displaying an icon in the taskbar (SDL frontend only).")
COMMAND_LINE_STRING_OPTION(
    icon_png_filename, 0, "sdl-icon", WHIST_ARGS_MAXLEN,
    "Set the protocol window icon from a 64x64 pixel png file (SDL frontend only).")

static const char* sdl_render_driver;
COMMAND_LINE_STRING_OPTION(sdl_render_driver, 0, "sdl-render-driver", 16,
                           "Set hint for the SDL render driver type; this may be ignored if "
                           "the given driver does not work (SDL frontend only).")

static void sdl_init_video_device(SDLFrontendContext* context) {
    // Defaults: no special device, all video must be downloaded to CPU
    // memory and then uploaded again.
    context->video.decode_device = NULL;
    context->video.decode_format = AV_PIX_FMT_NONE;

#ifdef _WIN32
    if (!strcmp(context->render_driver_name, "direct3d11")) {
        sdl_d3d11_init(context);
    }
#endif  // _WIN32

    // Texture sharing between Core Video decode and Metal rendering on
    // macOS.
    bool allow_metal_texture_sharing = true;
#ifdef __aarch64__
    // Disabled on ARM due to freeze issues (still enabled on x86).
    allow_metal_texture_sharing = false;
#endif
    if (allow_metal_texture_sharing && !strcmp(context->render_driver_name, "metal")) {
        // No device required; renderer can use Core Video pixel buffers
        // from Video Toolbox directly as textures.
        context->video.decode_format = AV_PIX_FMT_VIDEOTOOLBOX;
    }

    // More:
    // * OpenGL on Linux can work with VAAPI via DRM.
    // * D3D9 can work with DXVA2.
    // * Nvidia devices can work with NVDEC.
}

void sdl_get_video_device(WhistFrontend* frontend, AVBufferRef** device,
                          enum AVPixelFormat* format) {
    SDLFrontendContext* context = (SDLFrontendContext*) frontend->context;

    if (device) {
        *device = context->video.decode_device;
    }
    if (format) {
        *format = context->video.decode_format;
    }
}

static atomic_int sdl_atexit_initialized = ATOMIC_VAR_INIT(0);

// TODO: We could factor this out to initialize by component -- e.g. audio, window, renderer, etc.
WhistStatus sdl_init(WhistFrontend* frontend, int width, int height, const char* title,
                     const WhistRGBColor* color) {
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

    if (sdl_render_driver) {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, sdl_render_driver);
    }

#ifdef _WIN32
    // Tell Windows that the content we're rendering is aware of the system's set DPI.
    // Note: This fails on subsequent calls, but that's just a no-op so it's fine.
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

    if (!sdl_render_driver) {
        // Ensure that Windows uses the D3D11 driver rather than D3D9.
        // (The D3D9 driver does work, but it does not support the NV12
        // textures that we use, so performance with it is terrible.)
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
    }

#ifndef NDEBUG
    // Enable the D3D11 debug layer in debug builds.
    SDL_SetHint(SDL_HINT_RENDER_DIRECT3D11_DEBUG, "1");
#endif
#endif  // _WIN32

    // Allow the screensaver to activate while the frontend is running
    SDL_EnableScreenSaver();

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
        width = display_info.w;
    }
    if (height == 0) {
        height = display_info.h;
    }

    if (skip_taskbar) {
        // Hide the taskbar on macOS directly
        sdl_native_hide_taskbar();
    }

    SDLFrontendContext* context = (SDLFrontendContext*) safe_malloc(sizeof(SDLFrontendContext));
    memset(context, 0, sizeof(SDLFrontendContext));
    frontend->context = context;

    context->audio_device = 0;
    context->key_state = SDL_GetKeyboardState(&context->key_count);
    // context->file_drag_event_id = SDL_RegisterEvents(1);

    context->cursor.state = CURSOR_STATE_VISIBLE;
    context->cursor.hash = 0;
    context->cursor.last_visible_position.x = 0;
    context->cursor.last_visible_position.y = 0;
    context->cursor.handle = NULL;
    
    // create dummy window
    SDLWindowContext* dummy_context = (SDLWindowContext*) safe_malloc(sizeof(SDLWindowContext));
    dummy_context->to_be_created = true;
    dummy_context->x = SDL_WINDOWPOS_CENTERED;
    dummy_context->y = SDL_WINDOWPOS_CENTERED;
    dummy_context->width = width;
    dummy_context->height = height;
    dummy_context->title = "Dummy";
    dummy_context->color = {17, 24, 39};
    dummy_context->is_fullscreen = false;
    dummy_context->is_resizable = false;
    context->windows[0] = dummy_context;
    // create a dummy window to get resolution
    sdl_create_window(frontend, 0);

    sdl_init_video_device(context);

    // TODO: Rebuild this functionality.
    // if (icon_png_filename != NULL) {
    //     SDL_Surface* icon_surface = sdl_surface_from_png_file(icon_filename);
    //     SDL_SetWindowIcon(sdl_window, icon_surface);
    //     sdl_free_png_file_rgb_surface(icon_surface);
    // }

    sdl_native_init_external_drag_handler(frontend);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Pump the event loop until the window is actually initialized (mainly macOS).
    }

    while (SDL_PollEvent(&event)) {
        // Pump the event loop until the window fully finishes loading.
    }

    // The window is fully loaded, but we hold off on showing it until we actually start rendering.
    // The first call to sdl_paint_avframe() will set context->video_has_rendered to true. Then,
    // the next call to sdl_render() will show the window and set context->window_has_shown to true.

    return WHIST_SUCCESS;
}

WhistStatus sdl_create_window(WhistFrontend* frontend, int id) {
    SDLFrontendContext* context = (SDLFrontendContext*) frontend->context;

    // Create a window and renderer with the given parameters, add it to context->windows
    
    // If the id is already present in windows, LOG_ERROR
    if (context->windows.find(id) != context->windows.end()) {
        if (!context->windows[id]->to_be_created) {
            LOG_ERROR("Tried to make a window with duplicate ID %d!", id);
            // IDK what this means but it looks legit
            return WHIST_ERROR_ALREADY_SET;
        }
    } else {
        LOG_ERROR("Tried to create a window with ID %d, but found no SDLWindowContext", id);
    }

    context->windows[id]->to_be_created = false;

    // Window flags
    uint32_t window_flags = 0;
    window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    window_flags |= SDL_WINDOW_OPENGL;
    window_flags |= SDL_WINDOW_RESIZABLE;
    // Avoid glitchy-looking titlebar-content combinations while the
    // window is loading, and glitches caused by early user interaction.
    window_flags |= SDL_WINDOW_HIDDEN;
    if (skip_taskbar) {
        // Hide the taskbar on Windows/Linux
        window_flags |= SDL_WINDOW_SKIP_TASKBAR;
    }

    if (context->windows[id]->title == NULL) {
        // Default window title is "Whist"
        context->windows[id]->title = "Whist";
    }
    
    // Renderer flags
    uint32_t renderer_flags = 0;
    renderer_flags |= SDL_RENDERER_ACCELERATED;
    if (VSYNC_ON) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    // Create the window and renderer
    context->windows[id]->window = SDL_CreateWindow(context->windows[id]->title, context->windows[id]->x, context->windows[id]->y, context->windows[id]->width,
                                       context->windows[id]->height, window_flags);
    if (context->windows[id]->window == NULL) {
        LOG_ERROR("Could not create window: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

    context->windows[id]->renderer = SDL_CreateRenderer(context->windows[id]->window, -1, renderer_flags);
    if (context->windows[id]->renderer == NULL) {
        LOG_ERROR("Could not create renderer: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    SDL_SetRenderDrawBlendMode(context->windows[id]->renderer, SDL_BLENDMODE_BLEND);

    // Set renderer name if not set
    if (!context->render_driver_name) {
        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(context->windows[id]->renderer, &info) != 0) {
            LOG_ERROR("Could not get renderer info: %s", SDL_GetError());
            return WHIST_ERROR_UNKNOWN;
        }
        LOG_INFO("Using renderer: %s", info.name);
        context->render_driver_name = info.name;
    }

    // We don't need to do this if we don't initialize the window until we get frames from the server
    /*
    // window starts solid color
    frontend->call->paint_solid(frontend, color);
    frontend->call->render(frontend);
    frontend->call->set_titlebar_color(frontend, color);
    */

    // Safe to set these post-initialization.
    sdl_native_init_window_options(context->windows[id]->window);
    SDL_SetWindowMinimumSize(context->windows[id]->window, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);
    return WHIST_SUCCESS;
}

void sdl_destroy_window(WhistFrontend* frontend, int id) {
    SDLFrontendContext* context = (SDLFrontendContext*) frontend->context;
    if (context->windows.find(id) != context->windows.end()) {
        SDLWindowContext* window_context = context->windows[id];
        // destroy video texture
        if (window_context->texture != NULL) {
            SDL_DestroyTexture(window_context->texture);
            window_context->texture = NULL;
        }
        // destroy renderer
        if (window_context->renderer != NULL) {
            SDL_DestroyRenderer(window_context->renderer);
        }
        // destroy window
        if (window_context->window != NULL) {
            LOG_INFO("Destroying window with ID %d", id);
            SDL_DestroyWindow(window_context->window);
            window_context->window = NULL;
        }
        free(window_context);
        context->windows.erase(id);
    } else {
        LOG_ERROR("Tried to destroy window with ID %d, but no window with that ID was found!", id);
    }
}

// update x/y/w/h/etc of all windows according to window_data
// any new windows will be marked as to_be_created
// any old windows not included in window_data will be marked as to_be_destroyed
void sdl_update_window_data(WhistFrontend* frontend, WhistWindowData* window_data) {
    SDLFrontendContext* context = (SDLFrontendContext*) frontend->context;

    // mark all windows as to be destroyed
    for (const auto& pair : context->windows) {
        SDLWindowContext* window_context = pair.second;
        window_context->to_be_destroyed = true;
    }
    for (int i = 0; i < MAX_WINDOWS; i++) {
        int id = window_data[i].id;
        if (id != 0) {
            if (context->windows.find(id) != context->windows.end()) {
                // malloc a new SDLWindowContext but DON'T CREATE THE ACTUAL WIDNOW
                context->windows[id] = (SDLWindowContext*) safe_malloc(sizeof(SDLWindowContext));
                context->windows[id]->to_be_created = true;
            }
            // ideally we would memcpy, except WhistWindowdata has ID.
            // TODO: make this better
            context->windows[id]->x = window_data[i].x;
            context->windows[id]->y = window_data[i].y;
            context->windows[id]->width = window_data[i].width;
            context->windows[id]->height = window_data[i].height;
            context->windows[id]->title = ""; // window_data[i].title;
            context->windows[id]->color = window_data[i].corner_color;
            context->windows[id]->is_fullscreen = window_data[i].is_fullscreen;
            context->windows[id]->is_resizable = window_data[i].is_resizable;
            context->windows[id]->to_be_destroyed = false;
        }
    }
}

// TODO: better name needed
void sdl_update_windows(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*) frontend->context;

    // destroy to be destroyed windows
    for (const auto& pair : context->windows) {
        int id = pair.first;
        SDLWindowContext* window_context = pair.second;
        if (window_context->to_be_destroyed) {
            sdl_destroy_window(frontend, id);
        }
        if (window_context->to_be_created) {
            sdl_create_window(frontend, id);
        }
    }
}

void sdl_destroy(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*) frontend->context;

    if (!context) {
        return;
    }
    // destroy all windows
    for (const auto& pair : context->windows) {
        int id = pair.first;
        sdl_destroy_window(frontend, id);
    }
    av_buffer_unref(&context->video.decode_device);
    av_frame_free(&context->video.frame_reference);

#ifdef _WIN32
    sdl_d3d11_destroy(context);
#endif

    sdl_native_destroy_external_drag_handler(frontend);

    free(context);
}
