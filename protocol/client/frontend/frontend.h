#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

#include <whist/core/whist.h>
#include <whist/core/error_codes.h>
#include "api.h"

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
    FRONTEND_EVENT_FILE_DRAG,
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

typedef struct FrontendFileDragEvent {
    struct {
        int x;
        int y;
    } position;
    bool end_drag;
} FrontendFileDragEvent;

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
};

#define FRONTEND_HEADER_DECLARATION(return_type, name, ...) \
    return_type whist_frontend_##name(__VA_ARGS__);
FRONTEND_API(FRONTEND_HEADER_DECLARATION)
#undef FRONTEND_HEADER_DECLARATION

WhistFrontend* whist_frontend_create_sdl(void);
WhistFrontend* whist_frontend_create_external(void);
unsigned int whist_frontend_get_id(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_H
