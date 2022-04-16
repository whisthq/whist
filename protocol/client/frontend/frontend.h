#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

#include <whist/core/whist.h>
#include <whist/core/error_codes.h>

typedef struct WhistFrontend WhistFrontend;

typedef enum FrontendEventType {
    FRONTEND_EVENT_UNHANDLED = 0,
    FRONTEND_EVENT_RESIZE,
    FRONTEND_EVENT_VISIBILITY,
    FRONTEND_EVENT_AUDIO_UPDATE,
    FRONTEND_EVENT_KEYPRESS,
    FRONTEND_EVENT_MOUSE_MOTION,
    FRONTEND_EVENT_MOUSE_BUTTON,
    FRONTEND_EVENT_MOUSE_WHEEL,
    FRONTEND_EVENT_MOUSE_LEAVE,
    FRONTEND_EVENT_GESTURE,
    FRONTEND_EVENT_FILE_DROP,
    FRONTEND_EVENT_QUIT,
} FrontendEventType;

typedef struct FrontendKeypressEvent {
    WhistKeycode code;
    WhistKeymod mod;
    bool pressed;
} FrontendKeypressEvent;

typedef struct FrontendResizeEvent {
    int width;
    int height;
} FrontendResizeEvent;

typedef struct FrontendVisibilityEvent {
    bool visible;
} FrontendVisibilityEvent;

typedef struct FrontendMouseMotionEvent {
    struct {
        int x;
        int y;
    } absolute;
    struct {
        int x;
        int y;
    } relative;
    bool relative_mode;
} FrontendMouseMotionEvent;

typedef struct FrontendMouseButtonEvent {
    WhistMouseButton button;
    bool pressed;
} FrontendMouseButtonEvent;

typedef struct FrontendMouseWheelEvent {
    WhistMouseWheelMomentumType momentum_phase;
    struct {
        int x;
        int y;
    } delta;
    struct {
        float x;
        float y;
    } precise_delta;
} FrontendMouseWheelEvent;

typedef struct FrontendGestureEvent {
    struct {
        float theta;
        float dist;
    } delta;
    struct {
        float x;
        float y;
    } center;
    unsigned int num_fingers;
    WhistMultigestureType type;
} FrontendGestureEvent;

typedef struct FrontendFileDropEvent {
    struct {
        int x;
        int y;
    } position;
    char* filename;  // must be freed by handler
} FrontendFileDropEvent;

typedef struct FrontendQuitEvent {
    bool quit_application;
} FrontendQuitEvent;

typedef struct WhistFrontendEvent {
    WhistFrontend* frontend;
    FrontendEventType type;
    union {
        FrontendKeypressEvent keypress;
        FrontendMouseMotionEvent mouse_motion;
        FrontendMouseButtonEvent mouse_button;
        FrontendMouseWheelEvent mouse_wheel;
        FrontendGestureEvent gesture;
        FrontendFileDropEvent file_drop;
        FrontendQuitEvent quit;
        FrontendResizeEvent resize;
        FrontendVisibilityEvent visibility;
    };
} WhistFrontendEvent;

// TODO: Opaquify this
typedef struct WhistFrontendFunctionTable {
    // Lifecycle
    WhistStatus (*init)(WhistFrontend* frontend, int width, int height, const char* title);
    void (*destroy)(WhistFrontend* frontend);

    // Audio
    void (*open_audio)(WhistFrontend* frontend, unsigned int frequency, unsigned int channels);
    bool (*audio_is_open)(WhistFrontend* frontend);
    void (*close_audio)(WhistFrontend* frontend);
    WhistStatus (*queue_audio)(WhistFrontend* frontend, const uint8_t* data, size_t size);
    size_t (*get_audio_buffer_size)(WhistFrontend* frontend);

    // Display
    void (*temp_set_window)(WhistFrontend* frontend, void* window);
    void (*get_window_pixel_size)(WhistFrontend* frontend, int* width, int* height);
    void (*get_window_virtual_size)(WhistFrontend* frontend, int* width, int* height);
    void (*get_window_position)(WhistFrontend* frontend, int* x, int* y);
    WhistStatus (*get_window_display_index)(WhistFrontend* frontend, int* index);
    int (*get_window_dpi)(WhistFrontend* frontend);
    bool (*is_window_visible)(WhistFrontend* frontend);
    WhistStatus (*set_title)(WhistFrontend* frontend, const char* title);
    void (*restore_window)(WhistFrontend* frontend);
    void (*set_window_fullscreen)(WhistFrontend* frontend, bool fullscreen);
    void (*resize_window)(WhistFrontend* frontend, int width, int height);

    // Events
    bool (*poll_event)(WhistFrontend* frontend, WhistFrontendEvent* event);

    // Mouse
    void (*get_global_mouse_position)(WhistFrontend* frontend, int* x, int* y);
    void (*set_cursor)(WhistFrontend* frontend, WhistCursorInfo* cursor);

    // Keyboard
    void (*get_keyboard_state)(WhistFrontend* frontend, const uint8_t** key_state, int* key_count,
                               int* mod_state);

    // Video
    void (*paint_png)(WhistFrontend* frontend, const char* filename, int x, int y);
    void (*paint_solid)(WhistFrontend* frontend, WhistRGBColor* color);
    void (*paint_avframe)(WhistFrontend* frontend, AVFrame* frame);
    void (*render)(WhistFrontend* frontend);
    void (*set_titlebar_color)(WhistFrontend* frontend, WhistRGBColor* color);

} WhistFrontendFunctionTable;

struct WhistFrontend {
    void* context;
    unsigned int id;
    const WhistFrontendFunctionTable* call;
};

const WhistFrontendFunctionTable* sdl_get_function_table(void);

// Lifecycle
WhistFrontend* whist_frontend_create_sdl(int width, int height, const char* title);
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
void temp_frontend_set_window(WhistFrontend* frontend, void* window);
void whist_frontend_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height);
void whist_frontend_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height);
void whist_frontend_get_window_position(WhistFrontend* frontend, int* x, int* y);
WhistStatus whist_frontend_get_window_display_index(WhistFrontend* frontend, int* index);
// note: whist_frontend_get_window_dpi() should only be called inside main thread, at least on MacOS
int whist_frontend_get_window_dpi(WhistFrontend* frontend);
bool whist_frontend_is_window_visible(WhistFrontend* frontend);
bool whist_frontend_is_window_occluded(WhistFrontend* frontend);
int whist_frontend_set_screensaver_enabled(WhistFrontend* frontend, bool enabled);
void whist_frontend_resize_window(WhistFrontend* frontend, int width, int height);
void whist_frontend_restore_window(WhistFrontend* frontend);
void whist_frontend_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen);

// Title
WhistStatus whist_frontend_set_title(WhistFrontend* frontend, const char* title);

// Events
bool whist_frontend_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event);

// Keyboard
int whist_frontend_send_key_event(WhistFrontend* frontend, WhistKeycode keycode, bool pressed);
void whist_frontend_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state,
                                       int* key_count, int* mod_state);

// Mouse
void whist_frontend_get_global_mouse_position(WhistFrontend* frontend, int* x, int* y);
void whist_frontend_set_cursor(WhistFrontend* frontend, WhistCursorInfo* cursor);

// Video
void whist_frontend_paint_solid(WhistFrontend* frontend, WhistRGBColor* color);
void whist_frontend_paint_avframe(WhistFrontend* frontend, AVFrame* frame);
void whist_frontend_paint_png(WhistFrontend* frontend, const char* filename, int x, int y);
void whist_frontend_render(WhistFrontend* frontend);
void whist_frontend_set_titlebar_color(WhistFrontend* frontend, WhistRGBColor* color);

// Alerts
int whist_frontend_show_insufficient_bandwidth_alert(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_H
