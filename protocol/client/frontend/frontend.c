#include "frontend.h"
#include "sdl/sdl.h"
#include "virtual/virtual.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>

#define LOG_FRONTEND false
#if LOG_FRONTEND
#define LOG_FRONTEND_DEBUG(...) LOG_DEBUG(__VA_ARGS__)
#else
#define LOG_FRONTEND_DEBUG(...)
#endif  // LOG_FRONTEND

static atomic_int next_frontend_id = ATOMIC_VAR_INIT(0);

WhistFrontend* whist_frontend_create(const char* type) {
    const WhistFrontendFunctionTable* table = NULL;
    if (strcmp(type, "sdl") == 0) {
        table = sdl_get_function_table();
    } else if (strcmp(type, "virtual") == 0) {
        table = virtual_get_function_table();
    } else {
        LOG_ERROR("Invalid frontend type: %s", type);
        return NULL;
    }

    WhistFrontend* frontend = safe_malloc(sizeof(WhistFrontend));
    frontend->context = NULL;
    frontend->id = atomic_fetch_add(&next_frontend_id, 1);
    frontend->type = type;
    frontend->call = table;
    return frontend;
}

unsigned int whist_frontend_get_id(WhistFrontend* frontend) { return frontend->id; }

#define FRONTEND_ENTRY()                                                            \
    do {                                                                            \
        FATAL_ASSERT(frontend != NULL);                                             \
        LOG_FRONTEND_DEBUG("[%s %u] %s()", frontend->type, frontend->id, __func__); \
    } while (0)

// TODO: Can we use api.h to do this cleanly? The issue is that it's hard to separate out types from
// parameter names.

WhistStatus whist_frontend_init(WhistFrontend* frontend, int width, int height, const char* title,
                                const WhistRGBColor* color) {
    FRONTEND_ENTRY();
    return frontend->call->init(frontend, width, height, title, color);
}

void whist_frontend_destroy(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->destroy(frontend);
    free(frontend);
}

void whist_frontend_open_audio(WhistFrontend* frontend, unsigned int frequency,
                               unsigned int channels) {
    FRONTEND_ENTRY();
    frontend->call->open_audio(frontend, frequency, channels);
}

bool whist_frontend_audio_is_open(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    return frontend->call->audio_is_open(frontend);
}

void whist_frontend_close_audio(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->close_audio(frontend);
}

WhistStatus whist_frontend_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    FRONTEND_ENTRY();
    return frontend->call->queue_audio(frontend, data, size);
}

size_t whist_frontend_get_audio_buffer_size(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    return frontend->call->get_audio_buffer_size(frontend);
}

WhistStatus whist_frontend_get_window_pixel_size(WhistFrontend* frontend, int id, int* width,
                                                 int* height) {
    FRONTEND_ENTRY();
    return frontend->call->get_window_pixel_size(frontend, id, width, height);
}

WhistStatus whist_frontend_get_window_virtual_size(WhistFrontend* frontend, int id, int* width,
                                                   int* height) {
    FRONTEND_ENTRY();
    return frontend->call->get_window_virtual_size(frontend, id, width, height);
}

WhistStatus whist_frontend_get_window_display_index(WhistFrontend* frontend, int id, int* index) {
    FRONTEND_ENTRY();
    return frontend->call->get_window_display_index(frontend, id, index);
}

int whist_frontend_get_window_dpi(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    return frontend->call->get_window_dpi(frontend);
}

bool whist_frontend_is_any_window_visible(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    return frontend->call->is_any_window_visible(frontend);
}

WhistStatus whist_frontend_set_title(WhistFrontend* frontend, int id, const char* title) {
    FRONTEND_ENTRY();
    return frontend->call->set_title(frontend, id, title);
}

bool whist_frontend_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) {
    FRONTEND_ENTRY();
    return frontend->call->poll_event(frontend, event);
}

bool whist_frontend_wait_event(WhistFrontend* frontend, WhistFrontendEvent* event, int timeout_ms) {
    FRONTEND_ENTRY();
    return frontend->call->wait_event(frontend, event, timeout_ms);
}

void whist_frontend_interrupt(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->interrupt(frontend);
}

const char* whist_frontend_get_chosen_file(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    return frontend->call->get_chosen_file(frontend);
}

void* whist_frontend_file_download_start(WhistFrontend* frontend, const char* file_path,
                                         int64_t file_size) {
    FRONTEND_ENTRY();
    return frontend->call->file_download_start(frontend, file_path, file_size);
}

void whist_frontend_file_download_update(WhistFrontend* frontend, void* opaque,
                                         int64_t bytes_so_far, int64_t bytes_per_sec) {
    FRONTEND_ENTRY();
    frontend->call->file_download_update(frontend, opaque, bytes_so_far, bytes_per_sec);
}

void whist_frontend_file_download_complete(WhistFrontend* frontend, void* opaque) {
    FRONTEND_ENTRY();
    frontend->call->file_download_complete(frontend, opaque);
}

void whist_frontend_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {
    FRONTEND_ENTRY();
    frontend->call->set_cursor(frontend, cursor);
}

void whist_frontend_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state,
                                       int* key_count, int* mod_state) {
    FRONTEND_ENTRY();
    frontend->call->get_keyboard_state(frontend, key_state, key_count, mod_state);
}

void whist_frontend_restore_window(WhistFrontend* frontend, int id) {
    FRONTEND_ENTRY();
    frontend->call->restore_window(frontend, id);
}

void whist_frontend_set_window_fullscreen(WhistFrontend* frontend, int id, bool fullscreen) {
    FRONTEND_ENTRY();
    frontend->call->set_window_fullscreen(frontend, id, fullscreen);
}

void whist_frontend_get_video_device(WhistFrontend* frontend, AVBufferRef** device,
                                     enum AVPixelFormat* format) {
    FRONTEND_ENTRY();
    frontend->call->get_video_device(frontend, device, format);
}

WhistStatus whist_frontend_update_video(WhistFrontend* frontend, AVFrame* frame,
                                        WhistWindow* window_data, int num_windows) {
    FRONTEND_ENTRY();
    return frontend->call->update_video(frontend, frame, window_data, num_windows);
}

void whist_frontend_paint_png(WhistFrontend* frontend, const uint8_t* data, size_t data_size, int x,
                              int y) {
    FRONTEND_ENTRY();
    frontend->call->paint_png(frontend, data, data_size, x, y);
}

void whist_frontend_paint_solid(WhistFrontend* frontend, int id, const WhistRGBColor* color) {
    FRONTEND_ENTRY();
    frontend->call->paint_solid(frontend, id, color);
}

void whist_frontend_paint_video(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->paint_video(frontend);
}

void whist_frontend_render(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->render(frontend);
}

void whist_frontend_resize_window(WhistFrontend* frontend, int id, int width, int height) {
    FRONTEND_ENTRY();
    frontend->call->resize_window(frontend, id, width, height);
}

void whist_frontend_set_titlebar_color(WhistFrontend* frontend, int id,
                                       const WhistRGBColor* color) {
    FRONTEND_ENTRY();
    frontend->call->set_titlebar_color(frontend, id, color);
}

void whist_frontend_display_notification(WhistFrontend* frontend, const WhistNotification* notif) {
    FRONTEND_ENTRY();
    frontend->call->display_notification(frontend, notif);
}

void whist_frontend_declare_user_activity(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->declare_user_activity(frontend);
}

const char* whist_frontend_event_type_string(FrontendEventType type) {
    static const char* const types[] = {
        [FRONTEND_EVENT_UNHANDLED] = "unhandled",
        [FRONTEND_EVENT_RESIZE] = "resize",
        [FRONTEND_EVENT_VISIBILITY] = "visibility",
        [FRONTEND_EVENT_AUDIO_UPDATE] = "audio-update",
        [FRONTEND_EVENT_KEYPRESS] = "keypress",
        [FRONTEND_EVENT_MOUSE_MOTION] = "mouse-motion",
        [FRONTEND_EVENT_MOUSE_BUTTON] = "mouse-button",
        [FRONTEND_EVENT_MOUSE_WHEEL] = "mouse-wheel",
        [FRONTEND_EVENT_MOUSE_LEAVE] = "mouse-leave",
        [FRONTEND_EVENT_GESTURE] = "gesture",
        [FRONTEND_EVENT_FILE_DRAG] = "file-drag",
        [FRONTEND_EVENT_FILE_DROP] = "file-drop",
        [FRONTEND_EVENT_STARTUP_PARAMETER] = "startup-parameter",
        [FRONTEND_EVENT_OPEN_URL] = "open-url",
        [FRONTEND_EVENT_QUIT] = "quit",
        [FRONTEND_EVENT_INTERRUPT] = "interrupt",
    };
    if (type < 0 || (size_t)type >= ARRAY_LENGTH(types)) {
        return "invalid";
    } else {
        return types[type];
    }
}
