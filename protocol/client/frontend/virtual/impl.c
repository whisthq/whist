// So that SDL sees symbols such as memcpy
#include <whist/core/platform.h>
#if OS_IS(OS_WIN32)
#include <windows.h>
#else
#include <string.h>
#endif

// include SDL
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <whist/core/whist.h>
#include "common.h"
// This is a crutch. Once video is callback-ized we won't need it anymore.
#define FROM_WHIST_PROTOCOL true
#include "interface.h"
#include <whist/utils/queue.h>
#include <whist/utils/atomic.h>

static atomic_int sdl_atexit_initialized = ATOMIC_VAR_INIT(0);

extern QueueContext* events_queue;
extern void* callback_context;
extern OnFileUploadCallback on_file_upload;
extern OnCursorChangeCallback on_cursor_change;
extern OnFileDownloadStart on_file_download_start;
extern OnFileDownloadUpdate on_file_download_update;
extern OnFileDownloadComplete on_file_download_complete;
extern OnNotificationCallback on_notification_callback_ptr;
extern GetModifierKeyState get_modifier_key_state;

static void update_internal_state(WhistFrontend* frontend, WhistFrontendEvent* event) {
    VirtualFrontendContext* context = (VirtualFrontendContext*)frontend->context;
    switch (event->type) {
        case FRONTEND_EVENT_RESIZE: {
            context->width = event->resize.width;
            context->height = event->resize.height;
            context->dpi = event->resize.dpi;
            break;
        }
        case FRONTEND_EVENT_KEYPRESS: {
            // TODO : This code is only a temporary workaround for modifier keys to function
            // correctly. The real solution should be implemented by fetching the keyboard state
            // from the OS or Chrome, to get the intended functionality and benefit of sending
            // keyboard states. This method of maintaining states works only for modifier keys, as
            // chrome doesn't send "KeyUp" event for other keys during key combinations(such as
            // Ctrl + C etc.,).
            if (event->keypress.code >= FK_LCTRL && event->keypress.code <= FK_RGUI) {
                if (event->keypress.pressed) {
                    context->key_state[event->keypress.code] = 1;
                } else {
                    context->key_state[event->keypress.code] = 0;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

WhistStatus virtual_init(WhistFrontend* frontend, int width, int height, const char* title,
                         const WhistRGBColor* color) {
    frontend->context = safe_zalloc(sizeof(VirtualFrontendContext));
    VirtualFrontendContext* context = (VirtualFrontendContext*)frontend->context;
    context->width = width ? width : 1920;
    context->height = height ? height : 1080;
    context->dpi = 96;
    context->sdl_audio_device = 0;

    // Ensure that SDL doesn't override our signal handlers
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    // We only need to initialize SDL for the audio system
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
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

    return WHIST_SUCCESS;
}

void virtual_destroy(WhistFrontend* frontend) {
    VirtualFrontendContext* context = frontend->context;

    if (!context) {
        return;
    }

    free(context);
}

// We use SDL for the audio system for now. Eventually, we will implement
// audio using the virtual interface, similar to video.
void virtual_open_audio(WhistFrontend* frontend, unsigned int frequency, unsigned int channels) {
    VirtualFrontendContext* context = frontend->context;
    SDL_AudioSpec desired_spec = {
        .freq = (int)frequency,
        .format = AUDIO_F32SYS,
        .channels = (uint8_t)channels,
        // Must be a power of two. The value doesn't matter since
        // it only affects the size of the callback buffer, which
        // we don't use.
        .samples = 1024,
        .callback = NULL,
        .userdata = NULL,
    };
    SDL_AudioSpec obtained_spec;
    context->sdl_audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &obtained_spec, 0);
    if (context->sdl_audio_device == 0) {
        LOG_ERROR("Could not open audio device - %s", SDL_GetError());
        return;
    }

    // Verify that the obtained spec matches the desired spec, as
    // we currently do not perform resampling on ingress.
    FATAL_ASSERT(obtained_spec.freq == desired_spec.freq);
    FATAL_ASSERT(obtained_spec.format == desired_spec.format);
    FATAL_ASSERT(obtained_spec.channels == desired_spec.channels);

    // Start the audio device.
    SDL_PauseAudioDevice(context->sdl_audio_device, 0);
}

bool virtual_audio_is_open(WhistFrontend* frontend) {
    VirtualFrontendContext* context = frontend->context;
    return context->sdl_audio_device != 0;
}

void virtual_close_audio(WhistFrontend* frontend) {
    VirtualFrontendContext* context = frontend->context;
    if (frontend->call->audio_is_open(frontend)) {
        SDL_CloseAudioDevice(context->sdl_audio_device);
        context->sdl_audio_device = 0;
    }
}

WhistStatus virtual_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    VirtualFrontendContext* context = frontend->context;
    if (!frontend->call->audio_is_open(frontend)) {
        return WHIST_ERROR_NOT_FOUND;
    }
    if (SDL_QueueAudio(context->sdl_audio_device, data, (int)size) < 0) {
        LOG_ERROR("Could not queue audio - %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    return WHIST_SUCCESS;
}

size_t virtual_get_audio_buffer_size(WhistFrontend* frontend) {
    VirtualFrontendContext* context = frontend->context;
    if (!frontend->call->audio_is_open(frontend)) {
        return 0;
    }
    return SDL_GetQueuedAudioSize(context->sdl_audio_device);
}

WhistStatus virtual_get_window_pixel_size(WhistFrontend* frontend, int id, int* width,
                                          int* height) {
    VirtualFrontendContext* context = frontend->context;
    *width = context->width;
    *height = context->height;
    return WHIST_SUCCESS;
}

WhistStatus virtual_get_window_virtual_size(WhistFrontend* frontend, int id, int* width,
                                            int* height) {
    VirtualFrontendContext* context = frontend->context;
    *width = context->width;
    *height = context->height;
    return WHIST_SUCCESS;
}

WhistStatus virtual_get_window_display_index(WhistFrontend* frontend, int id, int* index) {
    *index = 0;
    return WHIST_SUCCESS;
}

int virtual_get_window_dpi(WhistFrontend* frontend) {
    VirtualFrontendContext* context = frontend->context;
    return context->dpi;
}

bool virtual_is_any_window_visible(WhistFrontend* frontend) { return true; }

WhistStatus virtual_set_title(WhistFrontend* frontend, int id, const char* title) {
    return WHIST_SUCCESS;
}

void virtual_restore_window(WhistFrontend* frontend, int id) {}

void virtual_set_window_fullscreen(WhistFrontend* frontend, int id, bool fullscreen) {}

void virtual_resize_window(WhistFrontend* frontend, int id, int width, int height) {}

bool virtual_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) {
    if (fifo_queue_dequeue_item(events_queue, event) == 0) {
        update_internal_state(frontend, event);
        return true;
    } else {
        return false;
    }
}

bool virtual_wait_event(WhistFrontend* frontend, WhistFrontendEvent* event, int timeout_ms) {
    if (fifo_queue_dequeue_item_timeout(events_queue, event, timeout_ms) == 0) {
        update_internal_state(frontend, event);
        return true;
    } else {
        return false;
    }
}

void virtual_interrupt(WhistFrontend* frontend) {
    WhistFrontendEvent event;
    event.type = FRONTEND_EVENT_INTERRUPT;
    if (fifo_queue_enqueue_item(events_queue, &event, false) != 0) {
        LOG_ERROR("Virtual frontend interrupt failed");
    }
}

const char* virtual_get_chosen_file(WhistFrontend* frontend) {
    if (on_file_upload != NULL) {
        return on_file_upload(callback_context);
    } else {
        return NULL;
    }
}

void* virtual_file_download_start(WhistFrontend* frontend, const char* file_path,
                                  int64_t file_size) {
    if (on_file_download_start != NULL) {
        return on_file_download_start(file_path, file_size);
    } else {
        return NULL;
    }
}

void virtual_file_download_update(WhistFrontend* frontend, void* opaque, int64_t bytes_so_far,
                                  int64_t bytes_per_sec) {
    if (on_file_download_update != NULL) {
        on_file_download_update(opaque, bytes_so_far, bytes_per_sec);
    }
}

void virtual_file_download_complete(WhistFrontend* frontend, void* opaque) {
    if (on_file_download_complete != NULL) {
        on_file_download_complete(opaque);
    }
}

static const char* css_cursor_from_whist_cursor_type(WhistCursorType type) {
    FATAL_ASSERT(type != WHIST_CURSOR_PNG);

    static const char* const map[] = {
        "none",         "png",        "alias",         "all-scroll",  "default",    "cell",
        "context-menu", "copy",       "crosshair",     "grab",        "grabbing",   "pointer",
        "help",         "text",       "vertical-text", "move",        "no-drop",    "not-allowed",
        "progress",     "col-resize", "e-resize",      "ew-resize",   "n-resize",   "ne-resize",
        "nesw-resize",  "ns-resize",  "nw-resize",     "nwse-resize", "row-resize", "s-resize",
        "se-resize",    "sw-resize",  "w-resize",      "wait",        "zoom-in",    "zoom-out",
    };

    return map[type];
}

void virtual_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {
    // Technically redundant caching, but solves for issues with the ARROW fallback cursor
    // and is really cheap.
    static WhistCursorType last_cursor_type = WHIST_CURSOR_ARROW;
    static WhistMouseMode last_mode = MOUSE_MODE_NORMAL;

    if (cursor->type == WHIST_CURSOR_PNG) {
        // We don't support PNG, so fall back to the arrow cursor.
        cursor->type = WHIST_CURSOR_ARROW;
    }

    if (cursor->type != last_cursor_type || cursor->mode != last_mode) {
        const char* css_name = css_cursor_from_whist_cursor_type(cursor->type);
        if (on_cursor_change != NULL) {
            on_cursor_change(NULL, css_name, cursor->mode == MOUSE_MODE_RELATIVE);
        }
        last_cursor_type = cursor->type;
    }
}

void virtual_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state, int* key_count,
                                int* mod_state) {
    VirtualFrontendContext* context = frontend->context;
    *key_state = context->key_state;
    *key_count = KEYCODE_UPPERBOUND;
    int actual_mod_state = 0;
    FATAL_ASSERT(get_modifier_key_state != NULL);
    int modifiers = get_modifier_key_state();

    // Handle the cases where modifier key is pressed, but our state says otherwise.
    // We don't know if Left or Right key is in pressed state. For now, choose Left key.
    if ((modifiers & SHIFT) && !context->key_state[FK_LSHIFT] && !context->key_state[FK_RSHIFT]) {
        context->key_state[FK_LSHIFT] = 1;
    }
    if ((modifiers & CTRL) && !context->key_state[FK_LCTRL] && !context->key_state[FK_RCTRL]) {
        context->key_state[FK_LCTRL] = 1;
    }
    if ((modifiers & ALT) && !context->key_state[FK_LALT] && !context->key_state[FK_RALT]) {
        context->key_state[FK_LALT] = 1;
    }
    if ((modifiers & GUI) && !context->key_state[FK_LGUI] && !context->key_state[FK_RGUI]) {
        context->key_state[FK_LGUI] = 1;
    }

    // Handle the cases where modifier key is NOT pressed
    if (!(modifiers & SHIFT)) {
        context->key_state[FK_LSHIFT] = context->key_state[FK_RSHIFT] = 0;
    }
    if (!(modifiers & CTRL)) {
        context->key_state[FK_LCTRL] = context->key_state[FK_RCTRL] = 0;
    }
    if (!(modifiers & ALT)) {
        context->key_state[FK_LALT] = context->key_state[FK_RALT] = 0;
    }
    if (!(modifiers & GUI)) {
        context->key_state[FK_LGUI] = context->key_state[FK_RGUI] = 0;
    }

    context->key_state[FK_CAPSLOCK] = (modifiers & CAPS) ? 1 : 0;

    // Setting the mod_state for GUI keys, CAPS LOCK and NUM LOCK alone as the caller is interested
    // in only those flags
    if (context->key_state[FK_LGUI]) {
        actual_mod_state |= MOD_LGUI;
    }
    if (context->key_state[FK_RGUI]) {
        actual_mod_state |= MOD_RGUI;
    }
    if (context->key_state[FK_CAPSLOCK]) {
        actual_mod_state |= MOD_CAPS;
    }
    if (context->key_state[FK_NUMLOCK]) {
        actual_mod_state |= MOD_NUM;
    }
    *mod_state = actual_mod_state;
}

void virtual_paint_png(WhistFrontend* frontend, const uint8_t* data, size_t data_size, int x,
                       int y) {}

void virtual_paint_solid(WhistFrontend* frontend, const WhistRGBColor* color) {}

WhistStatus virtual_update_video(WhistFrontend* frontend, AVFrame* frame, WhistWindow* window_data,
                                 int num_windows) {
    UNUSED(window_data);
    UNUSED(num_windows);
    virtual_interface_send_frame(frame);
    return WHIST_SUCCESS;
}

void virtual_paint_video(WhistFrontend* frontend) {}

void virtual_get_video_device(WhistFrontend* frontend, AVBufferRef** device,
                              enum AVPixelFormat* format) {
    *device = NULL;
    // AV_PIX_FMT_YUV420P is the format that Chromium currently accepts
    *format = AV_PIX_FMT_YUV420P;
}

void virtual_render(WhistFrontend* frontend) {}

void virtual_set_titlebar_color(WhistFrontend* frontend, int id, const WhistRGBColor* color) {}

void virtual_display_notification(WhistFrontend* frontend, const WhistNotification* notif) {
    if (on_notification_callback_ptr != NULL) {
        on_notification_callback_ptr(notif);
    }
}

void virtual_declare_user_activity(WhistFrontend* frontend) {}
