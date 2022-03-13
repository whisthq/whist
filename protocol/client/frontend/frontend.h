#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

#include <whist/core/whist.h>

typedef struct WhistFrontend WhistFrontend;

typedef struct WhistFrontendFunctionTable {
    int (*init)(WhistFrontend* frontend);
    void (*destroy)(WhistFrontend* frontend);
} WhistFrontendFunctionTable;

typedef void WhistFrontendEvent;
typedef void WhistAudioFormat;

struct WhistFrontend {
    void* context;
    const WhistFrontendFunctionTable* call;
};

const WhistFrontendFunctionTable* sdl_get_function_table(void);

// Lifecycle
WhistFrontend* whist_frontend_create_sdl(void);
WhistFrontend* whist_Frontend_create_external(void);
void whist_frontend_destroy(WhistFrontend* frontend);

// Audio
int whist_frontend_open_audio(WhistFrontend* frontend, WhistAudioFormat* format);
int whist_frontend_queue_audio(WhistFrontend* frontend, uint8_t* audio_data, size_t audio_data_size);
int whist_frontend_toggle_audio(WhistFrontend* frontend, bool enabled);
size_t whist_frontend_get_audio_buffer_size(WhistFrontend* frontend);
int whist_frontend_close_audio(WhistFrontend* frontend);

// Display
typedef struct {
    struct {
        unsigned int width;
        unsigned int height;
    } virtual_size;
    struct {
        unsigned int width;
        unsigned int height;
    } pixel_size;
    struct {
        int x;
        int y;
    } position;
    bool fullscreen;
    bool minimized;
    bool visible;
    int display_id;
} FrontendWindowInfo;

typedef struct FrontendDisplayInfo {
    struct {
        unsigned int width;
        unsigned int height;
    } display_size;
    unsigned int display_dpi;
    int display_id;
} FrontendDisplayInfo;

int whist_frontend_get_window_info(WhistFrontend* frontend, FrontendWindowInfo* info);
int whist_get_display_info(int display_id, FrontendDisplayInfo* display_info);
bool whist_frontend_window_changed_display(WhistFrontend* frontend);
int whist_frontend_set_screensaver_enabled(WhistFrontend* frontend, bool enabled);
int whist_frontend_resize_window(WhistFrontend* frontend, int width, int height);
int whist_frontend_set_window_minimized(WhistFrontend* frontend, bool minimized);
int whist_frontend_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen);
int whist_frontend_set_window_accent_color(WhistFrontend* frontend, WhistRGBColor color);

// Title
int whist_frontend_set_title(WhistFrontend* frontend, const char* title);

// Events
bool whist_frontend_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event);
bool whist_frontend_submit_event(WhistFrontend* frontend, WhistFrontendEvent* event);

// Keyboard
int whist_frontend_send_key_event(WhistFrontend* frontend, WhistKeycode keycode, bool pressed);
int whist_frontend_get_keyboard_state(WhistFrontend* frontend, WhistKeyboardState* state);

// Mouse
bool whist_frontend_get_relative_mouse_mode(WhistFrontend* frontend);
bool whist_frontend_capture_mouse(WhistFrontend* frontend, bool capture);
bool whist_frontend_get_mouse_position(WhistFrontend* frontend, int* x, int* y);
int whist_frontend_update_mouse_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor);
int whist_frontend_get_global_mouse_position(WhistFrontend* frontend, int* x, int* y);

// Video
int whist_frontend_render_solid(WhistFrontend* frontend, WhistRGBColor color);
int whist_frontend_render_nv12(WhistFrontend* frontend, uint8_t *y_plane, uint8_t *uv_plane,
                               int y_stride, int uv_stride, int x, int y, int width, int height);
int whist_frontend_render_png(WhistFrontend* frontend, const char* filename);

// Alerts
int whist_frontend_show_insufficient_bandwidth_alert(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_H
