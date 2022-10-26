#ifndef WHIST_CLIENT_FRONTEND_STRUCTS_H
#define WHIST_CLIENT_FRONTEND_STRUCTS_H

#include "../../whist/core/events.h"

typedef struct WhistFrontend WhistFrontend;

typedef enum WhistError {
    WHIST_DISCONNECT_ERROR,
    WHIST_DIMENSION_ERROR,
    WHIST_AUDIO_ERROR,
} WhistError;

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
     * Focus event.
     *
     * The window has been focused,
     * and the user is now viewing the whist stream
     */
    FRONTEND_EVENT_FOCUS,
    /**
     * Defocus event
     *
     * The window has been defocused,
     * and the user is no longer viewing the whist stream
     */
    FRONTEND_EVENT_DEFOCUS,
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
     * URL open event.
     */
    FRONTEND_EVENT_OPEN_URL,
    /**
     * Startup parameter event.
     */
    FRONTEND_EVENT_STARTUP_PARAMETER,
    /**
     * Quit event.
     *
     * The user or OS indicated that the application should quit.
     */
    FRONTEND_EVENT_QUIT,
    /**
     * Interrupt event.
     *
     * Used to interrupt the main thread while it is waiting for events
     * when there is some external work for it to do.  This is triggered
     * by calling the whist_frontend_interrupt() function from another
     * thread.
     */
    FRONTEND_EVENT_INTERRUPT,
} FrontendEventType;

typedef struct FrontendKeypressEvent {
    WhistKeycode code;
    WhistKeymod mod;
    bool pressed;
} FrontendKeypressEvent;

typedef struct FrontendResizeEvent {
    int width;
    int height;
    int dpi;  // Right now this value is used only by the virtual interface(chromium)
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

typedef struct FrontendOpenURLEvent {
    char *url;  // must be freed by the handler if not NULL.
} FrontendOpenURLEvent;

typedef struct FrontendStartupParameterEvent {
    char *key;    // must be freed by the handler if not NULL.
    char *value;  // must be freed by the handler if not NULL.
    bool error;
} FrontendStartupParameterEvent;

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
    char *filename;  // must be freed by handler if not NULL
    bool end_drop;   // true when ending a series of a drop events part of the same multi-file drop
} FrontendFileDropEvent;

typedef struct FrontendFileDragEvent {
    struct {
        int x;
        int y;
    } position;
    int group_id;
    bool end_drag;
    char *filename;  // File being dragged (multiple files should
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
        FrontendOpenURLEvent open_url;
        FrontendStartupParameterEvent startup_parameter;
        FrontendQuitEvent quit;
        FrontendResizeEvent resize;
        FrontendVisibilityEvent visibility;
    };
} WhistFrontendEvent;

#endif  // WHIST_CLIENT_FRONTEND_STRUCTS_H
