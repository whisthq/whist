#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

#include <whist/core/whist.h>
#include <whist/core/error_codes.h>
#include "api.h"

typedef struct WhistFrontend WhistFrontend;

typedef enum FrontendEventType {
    FRONTEND_EVENT_UNHANDLED = 0,
    /**
     * Resize event.
     *
     * The output window has been resized, so anything which draws to
     * the window should adjust to use the new size.
     */
    FRONTEND_EVENT_RESIZE,
    /**
     * Visibility change event.
     *
     * When not visible, nothing needs to be drawn to the window.
     */
    FRONTEND_EVENT_VISIBILITY,
    /**
     * Audio device update event.
     *
     * An audio device has been added or removed, so if audio is playing
     * then the device state will need to be refreshed.
     */
    FRONTEND_EVENT_AUDIO_UPDATE,
    /**
     * Key press event.
     *
     * A key have been either pressed or released.
     */
    FRONTEND_EVENT_KEYPRESS,
    /**
     * Mouse motion event.
     */
    FRONTEND_EVENT_MOUSE_MOTION,
    /**
     * Mouse button event.
     *
     * A mouse button was either pressed or released.
     */
    FRONTEND_EVENT_MOUSE_BUTTON,
    /**
     * Mouse wheel event.
     */
    FRONTEND_EVENT_MOUSE_WHEEL,
    /**
     * Mouse leave event.
     *
     * The mouse has left the output window.
     */
    FRONTEND_EVENT_MOUSE_LEAVE,
    /**
     * Gesture event.
     */
    FRONTEND_EVENT_GESTURE,
    /**
     * File drag event.
     *
     *  A file is being dragged over the window.
     */
    FRONTEND_EVENT_FILE_DRAG,
    /**
     * File drop event.
     */
    FRONTEND_EVENT_FILE_DROP,
    /**
     * Quit event.
     *
     * The user or OS indicated that the application should quit.
     */
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
    char* filename;  // must be freed by handler, if not NULL
    bool end_drop;   // true when ending a series of a drop events part of the same multi-file drop
} FrontendFileDropEvent;

typedef struct FrontendFileDragEvent {
    struct {
        int x;
        int y;
    } position;
    int group_id;
    bool end_drag;
    char* filename;  // File being dragged (multiple files should
                     //     be sent in multiple messages)
} FrontendFileDragEvent;

typedef struct FrontendQuitEvent {
    bool quit_application;
} FrontendQuitEvent;

typedef struct WhistFrontendEvent {
    FrontendEventType type;
    union {
        FrontendKeypressEvent keypress;
        FrontendMouseMotionEvent mouse_motion;
        FrontendMouseButtonEvent mouse_button;
        FrontendMouseWheelEvent mouse_wheel;
        FrontendGestureEvent gesture;
        FrontendFileDropEvent file_drop;
        FrontendFileDragEvent file_drag;
        FrontendQuitEvent quit;
        FrontendResizeEvent resize;
        FrontendVisibilityEvent visibility;
    };
} WhistFrontendEvent;

// TODO: Opaquify this
#define FRONTEND_FUNCTION_TABLE_DECLARATION(return_type, name, ...) \
    return_type (*name)(__VA_ARGS__);
typedef struct WhistFrontendFunctionTable {
    FRONTEND_API(FRONTEND_FUNCTION_TABLE_DECLARATION)
} WhistFrontendFunctionTable;
#undef FRONTEND_FUNCTION_TABLE_DECLARATION

struct WhistFrontend {
    void* context;
    unsigned int id;
    const WhistFrontendFunctionTable* call;
    const char* type;
};

#define WHIST_FRONTEND_SDL "sdl"
#define WHIST_FRONTEND_EXTERNAL "external"

#define FRONTEND_HEADER_DECLARATION(return_type, name, ...) \
    return_type whist_frontend_##name(__VA_ARGS__);
FRONTEND_API(FRONTEND_HEADER_DECLARATION)
#undef FRONTEND_HEADER_DECLARATION

WhistFrontend* whist_frontend_create(const char* type);
unsigned int whist_frontend_get_id(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_H
