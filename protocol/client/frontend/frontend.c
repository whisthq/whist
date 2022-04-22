#include "frontend.h"
#include "sdl/sdl.h"
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
    } else if (strcmp(type, "external") == 0) {
        LOG_WARNING("External frontend not implemented yet");
        return NULL;
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

void whist_frontend_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height) {
    FRONTEND_ENTRY();
    frontend->call->get_window_pixel_size(frontend, width, height);
}

void whist_frontend_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height) {
    FRONTEND_ENTRY();
    frontend->call->get_window_virtual_size(frontend, width, height);
}

WhistStatus whist_frontend_get_window_display_index(WhistFrontend* frontend, int* index) {
    FRONTEND_ENTRY();
    return frontend->call->get_window_display_index(frontend, index);
}

int whist_frontend_get_window_dpi(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    return frontend->call->get_window_dpi(frontend);
}

bool whist_frontend_is_window_visible(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    return frontend->call->is_window_visible(frontend);
}

WhistStatus whist_frontend_set_title(WhistFrontend* frontend, const char* title) {
    FRONTEND_ENTRY();
    return frontend->call->set_title(frontend, title);
}

bool whist_frontend_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) {
    FRONTEND_ENTRY();
    return frontend->call->poll_event(frontend, event);
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

void whist_frontend_restore_window(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->restore_window(frontend);
}

void whist_frontend_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen) {
    FRONTEND_ENTRY();
    frontend->call->set_window_fullscreen(frontend, fullscreen);
}

void whist_frontend_paint_png(WhistFrontend* frontend, const char* filename, int output_width,
                              int output_height, int x, int y) {
    FRONTEND_ENTRY();
    frontend->call->paint_png(frontend, filename, output_width, output_height, x, y);
}

void whist_frontend_paint_solid(WhistFrontend* frontend, const WhistRGBColor* color) {
    FRONTEND_ENTRY();
    frontend->call->paint_solid(frontend, color);
}

void whist_frontend_paint_avframe(WhistFrontend* frontend, AVFrame* frame, int output_width,
                                  int output_height) {
    FRONTEND_ENTRY();
    frontend->call->paint_avframe(frontend, frame, output_width, output_height);
}

void whist_frontend_render(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->render(frontend);
}

void whist_frontend_resize_window(WhistFrontend* frontend, int width, int height) {
    FRONTEND_ENTRY();
    frontend->call->resize_window(frontend, width, height);
}

void whist_frontend_set_titlebar_color(WhistFrontend* frontend, const WhistRGBColor* color) {
    FRONTEND_ENTRY();
    frontend->call->set_titlebar_color(frontend, color);
}

void whist_frontend_declare_user_activity(WhistFrontend* frontend) {
    FRONTEND_ENTRY();
    frontend->call->declare_user_activity(frontend);
}
