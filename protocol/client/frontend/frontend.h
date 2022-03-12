#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

typedef enum WhistFrontendType {
    WHIST_FRONTEND_SDL,
    WHIST_FRONTEND_EXTERNAL,
} WhistFrontendType;

typedef struct WhistFrontend {
    void* context;
    int (*open_audio)(WhistFrontend* frontend, WhistAudioFormat* format);
    int (*queue_audio)(WhistFrontend* frontend, uint8_t* audio_data, size_t audio_data_size);
} WhistFrontend;

// Lifecycle
WhistFrontend* whist_frontend_create(WhistFrontendType type);
int whist_frontend_destroy(WhistFrontend* frontend);

// Audio
int whist_frontend_open_audio(WhistFrontend* frontend, WhistAudioFormat* format);
int whist_frontend_queue_audio(WhistFrontend* frontend, uint8_t* audio_data, size_t audio_data_size);
int whist_frontend_toggle_audio(WhistFrontend* frontend, bool enabled);
size_t whist_frontend_get_audio_buffer_size(WhistFrontend* frontend);
int whist_frontend_close_audio(WhistFrontend* frontend);

// Display
int whist_frontend_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height);
int whist_frontend_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height);
bool whist_frontend_window_changed_display(WhistFrontend* frontend);
int whist_frontend_get_display_dpi(WhistFrontend* frontend, int* dpi);
int whist_frontend_get_display_size(WhistFrontend* frontend, int* width, int* height);
int whist_frontend_get_window_position(WhistFrontend* frontend, int* x, int* y);
int whist_frontend_toggle_screensaver(WhistFrontend* frontend, bool enabled);
int whist_frontend_resize_window(WhistFrontend* frontend, int width, int height);
bool whist_frontend_is_window_visible(WhistFrontend* frontend);
bool whist_frontend_is_window_minimized(WhistFrontend* frontend);
bool whist_frontend_toggle_window_minimized(WhistFrontend* frontend);
int whist_frontend_toggle_window_fullscreen(WhistFrontend* frontend, bool enabled);
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
