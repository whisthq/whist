#include "frontend.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>

// TODO: Refactor filesystem to remove upwards include.
#include "../native_window_utils.h"

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

static bool sdl_poll_event_timeout(WhistFrontend* frontend, WhistFrontendEvent* event,
                                   uint32_t timeout_ms) {
    SDL_Event sdl_event;

    // Poll/Wait event, getting the SDL_Event in response
    if (timeout_ms == 0) {
        // Only pollevent, if we don't want a timeout
        if (!SDL_PollEvent(&sdl_event)) {
            return false;
        }
    } else {
        if (!SDL_WaitEventTimeout(&sdl_event, timeout_ms)) {
            return false;
        }
    }

    // If the caller doesn't care about the event,
    // We can skip filling out the WhistFrontendEvent struct
    if (event == NULL) {
        return true;
    }
    // Otherwise, we fill out the event struct

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
    .temp_set_window = temp_sdl_set_window,
    .set_title = sdl_set_title,
    .poll_event_timeout = sdl_poll_event_timeout,
    .get_global_mouse_position = sdl_get_global_mouse_position,
};

const WhistFrontendFunctionTable* sdl_get_function_table(void) { return &sdl_function_table; }
