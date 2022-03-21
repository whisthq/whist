#include "frontend.h"
#include <whist/core/whist.h>
#include <whist/utils/atomic.h>

static atomic_int next_frontend_id = ATOMIC_VAR_INIT(0);

static WhistFrontend* whist_frontend_create(const WhistFrontendFunctionTable* function_table) {
    WhistFrontend* frontend = safe_malloc(sizeof(WhistFrontend));
    frontend->context = NULL;
    frontend->id = atomic_fetch_add(&next_frontend_id, 1);
    frontend->call = function_table;
    if (frontend->call->init(frontend) != WHIST_SUCCESS) {
        free(frontend);
        return NULL;
    }
    return frontend;
}

WhistFrontend* whist_frontend_create_sdl(void) {
    return whist_frontend_create(sdl_get_function_table());
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

WhistStatus whist_frontend_get_window_info(WhistFrontend* frontend, FrontendWindowInfo* info) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->get_window_info(frontend, info);
}

WhistStatus whist_frontend_set_title(WhistFrontend* frontend, const char* title) {
    FATAL_ASSERT(frontend != NULL);
    return frontend->call->set_title(frontend, title);
}
