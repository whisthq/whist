#include "frontend.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>

typedef struct SDLFrontendContext {
    SDL_AudioDeviceID audio_device;
    SDL_Window* window;
} SDLFrontendContext;

static atomic_int sdl_initialized = ATOMIC_VAR_INIT(0);

static int sdl_init_frontend(WhistFrontend* frontend) {
    // We only need to initialize SDL once.
    if (atomic_fetch_or(&sdl_initialized, 1) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
            LOG_FATAL("Could not initialize SDL - %s", SDL_GetError());
        }
        // If we have initialized SDL, then on exit we should clean up.
        atexit(SDL_Quit);
    }

    SDLFrontendContext* context = safe_malloc(sizeof(SDLFrontendContext));

    // Zero out fields that need to be zeroed.
    context->audio_device = 0;

    frontend->context = context;
    return 0;
}

static void sdl_destroy_frontend(WhistFrontend* frontend) { free(frontend->context); }

static void sdl_open_audio(WhistFrontend* frontend, unsigned int frequency, unsigned int channels) {
    SDLFrontendContext* context = frontend->context;
    // Verify that the device does not already exist.
    FATAL_ASSERT(context->audio_device == 0);
    SDL_AudioSpec desired_spec = {
        .freq = frequency,
        .format = AUDIO_F32SYS,
        .channels = channels,
        // The docs require a power of 2 here; the value should not
        // matter since we don't use the callback for audio ingress.
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

static int sdl_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    SDLFrontendContext* context = frontend->context;
    if (!sdl_audio_is_open(frontend)) {
        return -1;
    }
    if (SDL_QueueAudio(context->audio_device, data, size) < 0) {
        LOG_ERROR("Could not queue audio - %s", SDL_GetError());
        return -1;
    }
    return 0;
}

static size_t sdl_get_audio_buffer_size(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    if (!sdl_audio_is_open(frontend)) {
        return 0;
    }
    return SDL_GetQueuedAudioSize(context->audio_device);
}

static int sdl_get_window_info(WhistFrontend* frontend, FrontendWindowInfo* info) {
    SDLFrontendContext* context = frontend->context;
    if (context->window == NULL) {
        return -1;
    }
    SDL_GL_GetDrawableSize(context->window, &info->pixel_size.width, &info->pixel_size.height);
    SDL_GetWindowSize(context->window, &info->virtual_size.width, &info->virtual_size.height);
    SDL_GetWindowPosition(context->window, &info->position.x, &info->position.y);
    return 0;
}

static void temp_sdl_set_window(WhistFrontend* frontend, void* window) {
    SDLFrontendContext* context = frontend->context;
    context->window = (SDL_Window*)window;
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
};

const WhistFrontendFunctionTable* sdl_get_function_table(void) { return &sdl_function_table; }
