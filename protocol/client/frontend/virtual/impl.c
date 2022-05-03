#include "common.h"

WhistStatus virtual_init(WhistFrontend* frontend, int width, int height, const char* title,
                         const WhistRGBColor* color) {
    frontend->context = safe_malloc(sizeof(VirtualFrontendContext));
    VirtualFrontendContext* context = (VirtualFrontendContext*)frontend->context;
    context->width = width ? width : 640;
    context->height = height ? height : 480;
    context->dpi = 192;
    return WHIST_SUCCESS;
}

void virtual_destroy(WhistFrontend* frontend) {
    VirtualFrontendContext* context = frontend->context;

    if (!context) {
        return;
    }

    free(context);
}

void virtual_open_audio(WhistFrontend* frontend, unsigned int frequency, unsigned int channels) {}

bool virtual_audio_is_open(WhistFrontend* frontend) { return true; }

void virtual_close_audio(WhistFrontend* frontend) {}

WhistStatus virtual_queue_audio(WhistFrontend* frontend, const uint8_t* data, size_t size) {
    return WHIST_SUCCESS;
}

size_t virtual_get_audio_buffer_size(WhistFrontend* frontend) { return 1; }

void virtual_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height) {
    VirtualFrontendContext* context = frontend->context;
    *width = context->width;
    *height = context->height;
}

void virtual_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height) {
    VirtualFrontendContext* context = frontend->context;
    *width = context->width;
    *height = context->height;
}

WhistStatus virtual_get_window_display_index(WhistFrontend* frontend, int* index) {
    *index = 0;
    return WHIST_SUCCESS;
}

int virtual_get_window_dpi(WhistFrontend* frontend) {
    VirtualFrontendContext* context = frontend->context;
    return context->dpi;
}

bool virtual_is_window_visible(WhistFrontend* frontend) { return true; }

WhistStatus virtual_set_title(WhistFrontend* frontend, const char* title) { return WHIST_SUCCESS; }

void virtual_restore_window(WhistFrontend* frontend) {}

void virtual_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen) {}

void virtual_resize_window(WhistFrontend* frontend, int width, int height) {
    VirtualFrontendContext* context = frontend->context;
    context->width = width;
    context->height = height;
}

bool virtual_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) { return false; }

void virtual_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor) {}

void virtual_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state, int* key_count,
                                int* mod_state) {
    *key_state = NULL;
    *key_count = 0;
    *mod_state = 0;
}

void virtual_paint_png(WhistFrontend* frontend, const char* filename, int output_width,
                       int output_height, int x, int y) {}

void virtual_paint_solid(WhistFrontend* frontend, const WhistRGBColor* color) {}

WhistStatus virtual_update_video(WhistFrontend* frontend, AVFrame* frame) { return WHIST_SUCCESS; }

void virtual_paint_video(WhistFrontend* frontend, int output_width, int output_height) {}

void virtual_get_video_device(WhistFrontend* frontend, AVBufferRef** device,
                              enum AVPixelFormat* format) {
    *device = NULL;
#if defined(__APPLE__)
    // for now i'm going to assume this always works
    *format = AV_PIX_FMT_VIDEOTOOLBOX;
#else
    *format = AV_PIX_FMT_NONE;
    // todo: figure out d3d11 here potentially
#endif  // __APPLE__
}

void virtual_render(WhistFrontend* frontend) {}

void virtual_set_titlebar_color(WhistFrontend* frontend, const WhistRGBColor* color) {}

void virtual_declare_user_activity(WhistFrontend* frontend) {}
