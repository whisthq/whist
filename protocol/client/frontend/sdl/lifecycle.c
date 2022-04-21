#include "common.h"
#include "native.h"
#include <whist/utils/atomic.h>
#include <whist/utils/command_line.h>

/*
============================
Command-line options
============================
*/

static bool skip_taskbar;
static char* icon_png_filename;
COMMAND_LINE_BOOL_OPTION(
    skip_taskbar, 0, "sdl-skip-taskbar",
    "Launch the protocol without displaying an icon in the taskbar (SDL frontend only).")
COMMAND_LINE_STRING_OPTION(
    icon_png_filename, 0, "sdl-icon", WHIST_ARGS_MAXLEN,
    "Set the protocol window icon from a 64x64 pixel png file (SDL frontend only).")

/*
============================
Public Function Implementations
============================
*/

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
        sdl_native_hide_taskbar();
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
    memset(context, 0, sizeof(SDLFrontendContext));
    frontend->context = context;

    context->audio_device = 0;
    context->key_state = SDL_GetKeyboardState(&context->key_count);
    context->file_drag_event_id = SDL_RegisterEvents(1);
    context->video_has_rendered = false;
    context->window_has_shown = false;

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

    frontend->call->paint_solid(frontend, color);
    frontend->call->render(frontend);
    frontend->call->set_titlebar_color(frontend, color);

    context->texture =
        SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING,
                          MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
    if (context->texture == NULL) {
        LOG_ERROR("Could not create texture: %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

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

    // Safe to set these post-initialization.
    sdl_native_init_window_options(context->window);
    SDL_SetWindowMinimumSize(context->window, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);

    while (SDL_PollEvent(&event)) {
        // Pump the event loop until the window fully finishes loading.
    }

    // The window is fully loaded, but we hold off on showing it until we actually start rendering.
    // The first call to sdl_paint_avframe() will set context->video_has_rendered to true. Then,
    // the next call to sdl_render() will show the window and set context->window_has_shown to true.

    return WHIST_SUCCESS;
}

void sdl_destroy(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;

    if (!context) {
        return;
    }

    if (context->texture != NULL) {
        SDL_DestroyTexture(context->texture);
        context->texture = NULL;
    }

    if (context->renderer != NULL) {
        SDL_DestroyRenderer(context->renderer);
        context->renderer = NULL;
    }

    if (context->window != NULL) {
        LOG_INFO("Destroying window");
        SDL_DestroyWindow(context->window);
        context->window = NULL;
    }

    sdl_native_destroy_external_drag_handler(frontend);

    free(context);
}