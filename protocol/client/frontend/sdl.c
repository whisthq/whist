#include "frontend.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>

typedef struct SDLFrontendContext {
    SDL_AudioDeviceID audio_device;
    SDL_Window* window;
} SDLFrontendContext;

static atomic_int sdl_atexit_initialized = ATOMIC_VAR_INIT(0);

static WhistStatus sdl_init_frontend(WhistFrontend* frontend) {
    // Only the first init does anything; subsequent ones update an internal
    // refcount.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        LOG_ERROR("Could not initialize SDL - %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }

    // set the window minimum size
    SDL_SetWindowMinimumSize(sdl_window, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);

    // Make sure that ctrl+click is processed as a right click on Mac
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");

    // Allow inactive protocol to trigger the screensaver
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

    // Initialize the renderer
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, VSYNC_ON ? "1" : "0");

#ifdef _WIN32
    // Ensure that Windows uses the D3D11 driver rather than D3D9.
    // (The D3D9 driver does work, but it does not support the NV12
    // textures that we use, so performance with it is terrible.)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#endif

    // We only need to SDL_Quit once, regardless of the subsystem refcount.
    // SDL_QuitSubSystem() would require us to register an atexit for each
    // call.
    if (atomic_fetch_or(&sdl_atexit_initialized, 1) == 0) {
        // If we have initialized SDL, then on exit we should clean up.
        atexit(SDL_Quit);
    }
    SDLFrontendContext* context = safe_malloc(sizeof(SDLFrontendContext));

    // Zero out fields that need to be zeroed.
    context->audio_device = 0;

    frontend->context = context;
    return WHIST_SUCCESS;
}

static void sdl_destroy_frontend(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    SDL_DestroyWindow(context->window);
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
    if (sdl_audio_is_open(frontend)) {
        SDL_CloseAudioDevice(context->audio_device);
        context->audio_device = 0;
    }
}

static WhistStatus sdl_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    SDLFrontendContext* context = frontend->context;
    if (!sdl_audio_is_open(frontend)) {
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
    if (!sdl_audio_is_open(frontend)) {
        return 0;
    }
    return SDL_GetQueuedAudioSize(context->audio_device);
}

static WhistStatus sdl_get_window_info(WhistFrontend* frontend, FrontendWindowInfo* info) {
    SDLFrontendContext* context = frontend->context;
    if (context->window == NULL) {
        return WHIST_ERROR_NOT_FOUND;
    }
    SDL_GL_GetDrawableSize(context->window, &info->pixel_size.width, &info->pixel_size.height);
    SDL_GetWindowSize(context->window, &info->virtual_size.width, &info->virtual_size.height);
    SDL_GetWindowPosition(context->window, &info->position.x, &info->position.y);
    info->display_index = SDL_GetWindowDisplayIndex(context->window);
    int window_flags = SDL_GetWindowFlags(context->window);
    info->minimized = (window_flags & SDL_WINDOW_MINIMIZED) != 0;
    info->occluded = (window_flags & SDL_WINDOW_OCCLUDED) != 0;
    return WHIST_SUCCESS;
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

static const WhistFrontendFunctionTable sdl_function_table = {
    .init = sdl_init_frontend,
    .destroy = sdl_destroy_frontend,
    .open_audio = sdl_open_audio,
    .audio_is_open = sdl_audio_is_open,
    .close_audio = sdl_close_audio,
    .queue_audio = sdl_queue_audio,
    .get_audio_buffer_size = sdl_get_audio_buffer_size,
    .get_window_info = sdl_get_window_info,
    .temp_set_window = temp_sdl_set_window,
    .set_title = sdl_set_title,
};

const WhistFrontendFunctionTable* sdl_get_function_table(void) { return &sdl_function_table; }
