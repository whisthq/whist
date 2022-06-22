#include <whist/core/whist.h>
extern "C" {
#include "common.h"
#include "native.h"
#include "stdin_parser.h"
#include <whist/utils/command_line.h>
}

#include "sdl_struct.hpp"
#include <whist/utils/atomic.h>

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
    if (!strcmp(context->render_driver_name, "direct3d11") && context->video.decode_device) {
        // The device setup for D3D11 was already done while creating
        // the first window, so nothing else needed here.
        return;
    }

    // Defaults: no special device, all video must be downloaded to CPU
    // memory and then uploaded again.
    context->video.decode_device = NULL;
    context->video.decode_format = AV_PIX_FMT_NONE;

    // Texture sharing between Core Video decode and Metal rendering on
    // macOS. Disabled on ARM due to freeze issues (still enabled on x86).

    bool allow_metal_texture_sharing = true;

    if (ARCH_IS(ARCH_ARM_64) && !FIX_M1_FREEZE_WITH_LOCK) {
        allow_metal_texture_sharing = false;
    }

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
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;

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
    /*
     * Initialize an SDL frontend. Open an initial window with the given dimensions, title, and
     * titlebar color, and start the video and audio.
     */
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

#if OS_IS(OS_WIN32)
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
#endif  // Windows

    // Allow the screensaver to activate while the frontend is running
    SDL_EnableScreenSaver();

    if (skip_taskbar) {
        // Hide the taskbar on macOS directly
        sdl_native_hide_taskbar();
    }

    // Initialize the SDLFrontendContext
    SDLFrontendContext* context = new SDLFrontendContext();
    frontend->context = context;

    context->audio_device = 0;
    context->key_state = SDL_GetKeyboardState(&context->key_count);

    context->cursor.state = CURSOR_STATE_VISIBLE;
    context->cursor.hash = 0;
    context->cursor.last_visible_position.x = 0;
    context->cursor.last_visible_position.y = 0;
    context->cursor.handle = NULL;

    context->internal_event_id = SDL_RegisterEvents(1);
    FATAL_ASSERT(context->internal_event_id != (uint32_t)-1);

    // Initialize the default window
    SDLWindowContext* window_context = new SDLWindowContext();
    window_context->to_be_created = true;
    window_context->x = SDL_WINDOWPOS_CENTERED;
    window_context->y = SDL_WINDOWPOS_CENTERED;
    window_context->width = width;
    window_context->height = height;
    window_context->title = std::string(title == NULL ? "Whist" : title);
    window_context->color = {17, 24, 39};
    window_context->is_fullscreen = false;
    window_context->is_resizable = true;
    context->windows[0] = window_context;

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
        // Pump the event loop until the window fully finishes loading. (mainly MacOS)
    }

#if OS_IS(OS_WIN32)
    sdl_native_init_notifications(frontend);
#endif

    // The window is fully loaded, but we hold off on showing it until we actually start rendering.
    // The first call to sdl_paint_avframe() will set context->video_has_rendered to true. Then,
    // the next call to sdl_render() will show the window and set context->window_has_shown to true.

    context->kill_stdin_parser = false;
    // This thread reads entries of the form `key?value\n` or `key\n` from stdin and
    // reroutes them as SDL user events. Then, our `sdl/events.cpp` handling translates
    // these to appropriate WhistFrontend events.
    context->stdin_parser_thread = whist_create_thread(
        [](void* opaque) -> int {
            SDLFrontendContext* ctx = (SDLFrontendContext*)opaque;
            SDL_Event e = {0};
            e.type = ctx->internal_event_id;
            e.user.code = SDL_FRONTEND_EVENT_STDIN_EVENT;
            int ret = 0;
            // If ret is -1, we push a NULL key/value event before quitting, which is
            // propagated as a broken pipe or error by handlers.
            while (!ctx->kill_stdin_parser && ret != -1) {
                char* key = NULL;
                char* value = NULL;

                ret = get_next_piped_argument(&key, &value);

                if (ret == 1) {
                    whist_sleep(100);
                    continue;
                }

                // `key` and `value` are owned by the event handler, so don't free them here
                e.user.data1 = key;
                e.user.data2 = value;
                SDL_PushEvent(&e);
            }
            return 0;
        },
        "stdin_parser_thread", context);
    return WHIST_SUCCESS;
}

WhistStatus sdl_create_window(WhistFrontend* frontend, int id) {
    /*
     * Create an SDL window and renderer with the given id, initially hidden until the first render.
     * This function assumes that the corresponding SDLWindowContext* has already been filled in
     * with the desired metadata
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;

    // Create a window and renderer with the given parameters, add it to context->windows

    // If the id is already present in windows, LOG_ERROR
    if (context->windows.contains(id)) {
        if (!context->windows[id]->to_be_created) {
            LOG_ERROR("Tried to make a window with duplicate ID %d!", id);
            return WHIST_ERROR_ALREADY_SET;
        }
    } else {
        LOG_ERROR(
            "Tried to create a window with ID %d, but found no SDLWindowContext. Window will not "
            "be created.",
            id);
        return WHIST_ERROR_NOT_FOUND;
    }

    SDLWindowContext* window_context = context->windows[id];

    window_context->to_be_created = false;
    window_context->texture = NULL;

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

    // If width or height is 0, update width/height to use the entire display,
    // And set the MAXIMIZED window flag
    if (window_context->width == 0 || window_context->height == 0) {
        SDL_DisplayMode display_info;
        if (SDL_GetDesktopDisplayMode(0, &display_info) != 0) {
            LOG_ERROR("Could not get display mode: %s", SDL_GetError());
            return WHIST_ERROR_UNKNOWN;
        }

        window_context->width = display_info.w;
        window_context->height = display_info.h;
        window_flags |= SDL_WINDOW_MAXIMIZED;
    }

    // Renderer flags
    uint32_t renderer_flags = 0;
    renderer_flags |= SDL_RENDERER_ACCELERATED;
    if (VSYNC_ON) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    // Create the window and renderer
    window_context->window =
        SDL_CreateWindow(window_context->title.c_str(), window_context->x, window_context->y,
                         window_context->width, window_context->height, window_flags);
    if (window_context->window == NULL) {
        LOG_ERROR("Could not create window: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    window_context->window_id = SDL_GetWindowID(window_context->window);
    if (window_context->window_id == 0) {
        LOG_ERROR("Could not get window ID: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

    window_context->renderer = SDL_CreateRenderer(window_context->window, -1, renderer_flags);
    if (window_context->renderer == NULL) {
        LOG_ERROR("Could not create renderer: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    SDL_SetRenderDrawBlendMode(window_context->renderer, SDL_BLENDMODE_BLEND);

    // Set renderer name if not set
    if (!context->render_driver_name) {
        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(window_context->renderer, &info) != 0) {
            LOG_ERROR("Could not get renderer info: %s", SDL_GetError());
            return WHIST_ERROR_UNKNOWN;
        }
        LOG_INFO("Using renderer: %s", info.name);
        context->render_driver_name = info.name;
    }

#if OS_IS(OS_WIN32)
    if (!strcmp(context->render_driver_name, "direct3d11")) {
        sdl_d3d11_init_window_data(context, id);
    }
#endif  // Windows

    // We don't need to do this if we don't initialize the window until we get frames from the
    // server window starts solid color
    frontend->call->paint_solid(frontend, id, &window_context->color);
    frontend->call->set_titlebar_color(frontend, id, &window_context->color);

    // Safe to set these post-initialization.
    sdl_native_init_window_options(window_context->window);
    SDL_SetWindowMaximumSize(window_context->window, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);

    return WHIST_SUCCESS;
}

void sdl_destroy_window(WhistFrontend* frontend, int id) {
    /*
     * Destroy the window and renderer with given id if it exists.
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        SDLWindowContext* window_context = context->windows[id];
        // destroy video texture
        if (window_context->texture != NULL) {
            SDL_DestroyTexture(window_context->texture);
            window_context->texture = NULL;
        }
        // destroy the D3D11 device and context if Windows
#if OS_IS(OS_WIN32)
        sdl_d3d11_destroy_window_data(context, id);
#endif  // Windows
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
        context->windows.erase(id);
        delete window_context;
    } else {
        LOG_ERROR("Tried to destroy window with ID %d, but no window with that ID was found!", id);
    }
}

void sdl_destroy(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    // Context can be null, if SDL couldn't initialize
    if (context == NULL) {
        return;
    }

    if (context->stdin_parser_thread != NULL) {
        context->kill_stdin_parser = true;
        whist_wait_thread(context->stdin_parser_thread, NULL);
        context->stdin_parser_thread = NULL;
    }

    // Destroy all the windows
    while (!context->windows.empty()) {
        // Destroying a window erases it from the map, so we can't a for-each loop
        sdl_destroy_window(frontend, context->windows.begin()->first);
    }

    // Destroy video objects
    av_buffer_unref(&context->video.decode_device);
    av_frame_free(&context->video.frame_reference);

#if OS_IS(OS_WIN32)
    sdl_d3d11_destroy(context);
#endif

    sdl_native_destroy_external_drag_handler(frontend);

    frontend->context = NULL;
    delete context;
}
