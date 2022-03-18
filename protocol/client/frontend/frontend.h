#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

#include <whist/core/whist.h>
#include <whist/core/error_codes.h>

typedef struct WhistFrontend WhistFrontend;
typedef struct FrontendWindowInfo FrontendWindowInfo;
typedef struct WhistFrontendFunctionTable {
    // Lifecycle
    WhistStatus (*init)(WhistFrontend* frontend);
    void (*destroy)(WhistFrontend* frontend);

    // Audio
    void (*open_audio)(WhistFrontend* frontend, unsigned int frequency, unsigned int channels);
    bool (*audio_is_open)(WhistFrontend* frontend);
    void (*close_audio)(WhistFrontend* frontend);
    WhistStatus (*queue_audio)(WhistFrontend* frontend, const uint8_t* data, size_t size);
    size_t (*get_audio_buffer_size)(WhistFrontend* frontend);

    // Display
    void (*temp_set_window)(WhistFrontend* frontend, void* window);
    WhistStatus (*get_window_info)(WhistFrontend* frontend, FrontendWindowInfo* info);
} WhistFrontendFunctionTable;

typedef void WhistFrontendEvent;
typedef void WhistAudioFormat;

struct WhistFrontend {
    void* context;
    unsigned int id;
    const WhistFrontendFunctionTable* call;
};

const WhistFrontendFunctionTable* sdl_get_function_table(void);

// Lifecycle
WhistFrontend* whist_frontend_create_sdl(void);
WhistFrontend* whist_frontend_create_external(void);
unsigned int whist_frontend_get_id(WhistFrontend* frontend);
void whist_frontend_destroy(WhistFrontend* frontend);

// Audio
void whist_frontend_open_audio(WhistFrontend* frontend, unsigned int frequency,
                               unsigned int channels);
WhistStatus whist_frontend_queue_audio(WhistFrontend* frontend, const uint8_t* audio_data,
                                       size_t audio_data_size);
size_t whist_frontend_get_audio_buffer_size(WhistFrontend* frontend);
bool whist_frontend_audio_is_open(WhistFrontend* frontend);
void whist_frontend_close_audio(WhistFrontend* frontend);

// Display
struct FrontendWindowInfo {
    struct {
        int width;
        int height;
    } pixel_size;
    struct {
        int width;
        int height;
    } virtual_size;
    struct {
        int x;
        int y;
    } position;
    bool fullscreen;
    bool minimized;
    bool visible;
    int display_index;
};

void temp_frontend_set_window(WhistFrontend* frontend, void* window);
WhistStatus whist_frontend_get_window_info(WhistFrontend* frontend, FrontendWindowInfo* info);
// int whist_get_display_info(int display_id, FrontendDisplayInfo* display_info);
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
int whist_frontend_render_nv12(WhistFrontend* frontend, uint8_t* y_plane, uint8_t* uv_plane,
                               int y_stride, int uv_stride, int x, int y, int width, int height);
int whist_frontend_render_png(WhistFrontend* frontend, const char* filename);

// Alerts
int whist_frontend_show_insufficient_bandwidth_alert(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_H
