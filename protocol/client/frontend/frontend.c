#include "frontend.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>

static atomic_int next_frontend_id = ATOMIC_VAR_INIT(0);

static WhistFrontend* whist_frontend_create(const WhistFrontendFunctionTable* function_table,
                                            int width, int height, const char* title) {
    WhistFrontend* frontend = safe_malloc(sizeof(WhistFrontend));
    frontend->context = NULL;
    frontend->id = atomic_fetch_add(&next_frontend_id, 1);
    frontend->call = function_table;
    if (frontend->call->init(frontend, width, height, title) != WHIST_SUCCESS) {
        free(frontend);
        return NULL;
    }
    return frontend;
}

WhistFrontend* whist_frontend_create_sdl(int width, int height, const char* title) {
    return whist_frontend_create(sdl_get_function_table(), width, height, title);
}

void whist_frontend_destroy(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->destroy(frontend);
    free(frontend);
}
unsigned int whist_frontend_get_id(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->id;
}

void whist_frontend_open_audio(WhistFrontend* frontend, unsigned int frequency,
                               unsigned int channels) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->open_audio(frontend, frequency, channels);
}

bool whist_frontend_audio_is_open(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->audio_is_open(frontend);
}

void whist_frontend_close_audio(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->close_audio(frontend);
}

WhistStatus whist_frontend_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->queue_audio(frontend, data, size);
}

size_t whist_frontend_get_audio_buffer_size(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->get_audio_buffer_size(frontend);
}

void temp_frontend_set_window(WhistFrontend* frontend, void* window) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->temp_set_window(frontend, window);
}

void whist_frontend_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->get_window_pixel_size(frontend, width, height);
}

void whist_frontend_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->get_window_virtual_size(frontend, width, height);
}

void whist_frontend_get_window_position(WhistFrontend* frontend, int* x, int* y) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->get_window_position(frontend, x, y);
}

WhistStatus whist_frontend_get_window_display_index(WhistFrontend* frontend, int* index) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->get_window_display_index(frontend, index);
}

int whist_frontend_get_window_dpi(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->get_window_dpi(frontend);
}

bool whist_frontend_is_window_visible(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->is_window_visible(frontend);
}

WhistStatus whist_frontend_set_title(WhistFrontend* frontend, const char* title) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->set_title(frontend, title);
}

bool whist_frontend_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) {
    FATAL_ASSERT(frontend != NULL);
    bool ret = frontend->call->poll_event(frontend, event);
    if (ret && event != NULL) {
        event->frontend = frontend;
    }
    return ret;
}

void whist_frontend_get_global_mouse_position(WhistFrontend* frontend, int* x, int* y) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->get_global_mouse_position(frontend, x, y);
}

void whist_frontend_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->set_cursor(frontend, cursor);
}

void whist_frontend_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state,
                                       int* key_count, int* mod_state) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->get_keyboard_state(frontend, key_state, key_count, mod_state);
}

void whist_frontend_restore_window(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->restore_window(frontend);
}

void whist_frontend_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->set_window_fullscreen(frontend, fullscreen);
}

void whist_frontend_paint_png(WhistFrontend* frontend, const char* filename, int output_width,
                              int output_height, int x, int y) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->paint_png(frontend, filename, output_width, output_height, x, y);
}

void whist_frontend_paint_solid(WhistFrontend* frontend, const WhistRGBColor* color) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->paint_solid(frontend, color);
}

void whist_frontend_paint_avframe(WhistFrontend* frontend, AVFrame* frame, int output_width,
                                  int output_height) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->paint_avframe(frontend, frame, output_width, output_height);
}

void whist_frontend_render(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->render(frontend);
}

void whist_frontend_resize_window(WhistFrontend* frontend, int width, int height) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->resize_window(frontend, width, height);
}

void whist_frontend_set_titlebar_color(WhistFrontend* frontend, const WhistRGBColor* color) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->set_titlebar_color(frontend, color);
}
