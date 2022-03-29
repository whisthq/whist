#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

#include <whist/core/whist.h>
#include <whist/core/error_codes.h>

typedef struct WhistFrontend WhistFrontend;

typedef enum FrontendEventType {
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
    FRONTEND_EVENT_UNHANDLED,
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
    void (*get_window_pixel_size)(WhistFrontend* frontend, int* width, int* height);
    void (*get_window_virtual_size)(WhistFrontend* frontend, int* width, int* height);
    void (*get_window_position)(WhistFrontend* frontend, int* x, int* y);
    WhistStatus (*get_window_display_index)(WhistFrontend* frontend, int* index);
    int (*get_window_dpi)(WhistFrontend* frontend);
    bool (*is_window_visible)(WhistFrontend* frontend);
    WhistStatus (*set_title)(WhistFrontend* frontend, const char* title);

    // Events
    bool (*poll_event)(WhistFrontend* frontend, WhistFrontendEvent* event);

    // Mouse
    void (*get_global_mouse_position)(WhistFrontend* frontend, int* x, int* y);
} WhistFrontendFunctionTable;

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
void temp_frontend_set_window(WhistFrontend* frontend, void* window);
void whist_frontend_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height);
void whist_frontend_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height);
void whist_frontend_get_window_position(WhistFrontend* frontend, int* x, int* y);
WhistStatus whist_frontend_get_window_display_index(WhistFrontend* frontend, int* index);
int whist_frontend_get_window_dpi(WhistFrontend* frontend);
bool whist_frontend_is_window_visible(WhistFrontend* frontend);
bool whist_frontend_is_window_occluded(WhistFrontend* frontend);
int whist_frontend_set_screensaver_enabled(WhistFrontend* frontend, bool enabled);
int whist_frontend_resize_window(WhistFrontend* frontend, int width, int height);
int whist_frontend_set_window_minimized(WhistFrontend* frontend, bool minimized);
int whist_frontend_set_window_fullscreen(WhistFrontend* frontend, bool fullscreen);
int whist_frontend_set_window_accent_color(WhistFrontend* frontend, WhistRGBColor color);

// Title
WhistStatus whist_frontend_set_title(WhistFrontend* frontend, const char* title);

// Events
bool whist_frontend_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event);

// Keyboard
int whist_frontend_send_key_event(WhistFrontend* frontend, WhistKeycode keycode, bool pressed);
int whist_frontend_get_keyboard_state(WhistFrontend* frontend, WhistKeyboardState* state);

// Mouse
void whist_frontend_get_global_mouse_position(WhistFrontend* frontend, int* x, int* y);

// Video
int whist_frontend_render_solid(WhistFrontend* frontend, WhistRGBColor color);
int whist_frontend_render_nv12(WhistFrontend* frontend, uint8_t* y_plane, uint8_t* uv_plane,
                               int y_stride, int uv_stride, int x, int y, int width, int height);
int whist_frontend_render_png(WhistFrontend* frontend, const char* filename);

// Alerts
int whist_frontend_show_insufficient_bandwidth_alert(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_H
