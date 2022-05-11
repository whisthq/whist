#include "sdl_struct.hpp"

void sdl_open_audio(WhistFrontend* frontend, unsigned int frequency, unsigned int channels) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    // Verify that the device does not already exist.
    FATAL_ASSERT(context->audio_device == 0);
    SDL_AudioSpec desired_spec = {
        .freq = (int)frequency,
        .format = AUDIO_F32SYS,
        .channels = (Uint8)channels,
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

bool sdl_audio_is_open(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    return context->audio_device != 0;
}

void sdl_close_audio(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (frontend->call->audio_is_open(frontend)) {
        SDL_CloseAudioDevice(context->audio_device);
        context->audio_device = 0;
    }
}

WhistStatus sdl_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (!frontend->call->audio_is_open(frontend)) {
        return WHIST_ERROR_NOT_FOUND;
    }
    if (SDL_QueueAudio(context->audio_device, data, (int)size) < 0) {
        LOG_ERROR("Could not queue audio - %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    return WHIST_SUCCESS;
}

size_t sdl_get_audio_buffer_size(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (!frontend->call->audio_is_open(frontend)) {
        return 0;
    }
    return SDL_GetQueuedAudioSize(context->audio_device);
}
