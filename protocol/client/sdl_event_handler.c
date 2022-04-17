/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file sdl_event_handler.c
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

handleSDLEvent() must be called on any SDL event that occurs. Any action
trigged an SDL event must be triggered in sdl_event_handler.c
*/

/*
============================
Includes
============================
*/

#include "sdl_event_handler.h"

#include <whist/logging/logging.h>
#include "sdl_utils.h"
#include "audio.h"
#include "client_utils.h"
#include "network.h"
#include "native_window_utils.h"
#include <whist/utils/atomic.h>
#include <whist/debug/debug_console.h>

// Main state variables
extern bool client_exiting;
extern MouseMotionAccumulation mouse_state;

// This variable represents whether there is an active pinch gesture
bool active_pinch = false;

// This variable represents whether or not we are actively sending momentum scrolls
//     On MOMENTUM_BEGIN, we set this to true, and start sending momentum scrolls
//     On MOMENTUM_END, or on other non-scroll input, we set this to false, and ignore future
//     momentum scrolls.
//     This is primarily to make cmd keypresses kill momentum scrolls, otherwise
//     scroll + a later cmd causes an unintentional cmd+scroll zoom event
bool active_momentum_scroll = false;

/*
============================
Private Globals
============================
*/

atomic_int g_pending_audio_device_update = ATOMIC_VAR_INIT(0);

/*
============================
Private Functions Declarations
============================
*/

static int handle_frontend_event(WhistFrontendEvent* event);

/*
============================
Public Function Implementations
============================
*/

bool sdl_handle_events(WhistFrontend* frontend) {
    WhistFrontendEvent event;
    while (whist_frontend_poll_event(frontend, &event)) {
        if (handle_frontend_event(&event) != 0) {
            return false;
        }
    }

    // After handle_sdl_event potentially captures a mouse motion,
    // We throttle it down to only update once every 0.5ms
    static WhistTimer mouse_motion_timer;
    static bool first_mouse_motion = true;
    if (first_mouse_motion || get_timer(&mouse_motion_timer) * MS_IN_SECOND > 0.5) {
        if (update_mouse_motion(frontend) != 0) {
            return false;
        }
        start_timer(&mouse_motion_timer);
        first_mouse_motion = false;
    }

    return true;
}

bool sdl_pending_audio_device_update(void) {
    // Mark as no longer pending "0",
    // and return whether or not it was pending since the last time we called this function
    int pending_audio_device_update = atomic_exchange(&g_pending_audio_device_update, 0);
    return pending_audio_device_update > 0;
}

/*
============================
Private Function Implementations
============================
*/

static void handle_keypress_event(FrontendKeypressEvent* event) {
    WhistClientMessage msg = {0};
    msg.type = MESSAGE_KEYBOARD;
    msg.keyboard.code = event->code;
    msg.keyboard.pressed = event->pressed;
    msg.keyboard.mod = event->mod;
    send_wcmsg(&msg);
}

static void handle_mouse_motion_event(FrontendMouseMotionEvent* event) {
    mouse_state.x_nonrel = event->absolute.x;
    mouse_state.y_nonrel = event->absolute.y;
    mouse_state.x_rel += event->relative.x;
    mouse_state.y_rel += event->relative.y;
    mouse_state.is_relative = event->relative_mode;
    mouse_state.update = true;
}

static void handle_mouse_button_event(FrontendMouseButtonEvent* event) {
    WhistClientMessage msg = {0};
    msg.type = MESSAGE_MOUSE_BUTTON;
    msg.mouseButton.button = event->button;
    msg.mouseButton.pressed = event->pressed;
    send_wcmsg(&msg);
}

static void handle_mouse_wheel_event(FrontendMouseWheelEvent* event) {
    if (active_pinch) {
        // Suppress scroll events during a pinch gesture
        return;
    }

    WhistMouseWheelMomentumType momentum_phase = event->momentum_phase;
    if (momentum_phase == MOUSEWHEEL_MOMENTUM_BEGIN) {
        active_momentum_scroll = true;
    } else if (momentum_phase == MOUSEWHEEL_MOMENTUM_END ||
               momentum_phase == MOUSEWHEEL_MOMENTUM_NONE) {
        active_momentum_scroll = false;
    }

    // We received another event while momentum was active, so cancel the
    // momentum scroll by ignoring the event
    if (!active_momentum_scroll && momentum_phase == MOUSEWHEEL_MOMENTUM_ACTIVE) {
        return;
    }

    WhistClientMessage msg = {0};
    msg.type = MESSAGE_MOUSE_WHEEL;
    msg.mouseWheel.x = event->delta.x;
    msg.mouseWheel.y = event->delta.y;
    msg.mouseWheel.precise_x = event->precise_delta.x;
    msg.mouseWheel.precise_y = event->precise_delta.y;
    send_wcmsg(&msg);
}

static void handle_gesture_event(FrontendGestureEvent* event) {
    WhistClientMessage msg = {0};
    msg.type = MESSAGE_MULTIGESTURE;
    msg.multigesture = (WhistMultigestureMessage){
        .d_theta = event->delta.theta,
        .d_dist = event->delta.dist,
        .x = event->center.x,
        .y = event->center.y,
        .num_fingers = event->num_fingers,
        .active_gesture = active_pinch,
        .gesture_type = event->type,
    };

    if (event->type == MULTIGESTURE_PINCH_OPEN || event->type == MULTIGESTURE_PINCH_CLOSE) {
        active_pinch = true;
    } else if (active_pinch) {
        active_pinch = false;
        msg.multigesture.gesture_type = MULTIGESTURE_CANCEL;
    }
    send_wcmsg(&msg);
}

static void handle_file_drop_event(WhistFrontend* frontend, FrontendFileDropEvent* event) {
    FileEventInfo drop_info;
    int dpi = whist_frontend_get_window_dpi(frontend);

    // Scale the drop coordinates for server-side compatibility
    drop_info.server_drop.x = event->position.x * dpi / 96;
    drop_info.server_drop.y = event->position.y * dpi / 96;
    sdl_end_drag_event();
    file_synchronizer_set_file_reading_basic_metadata(event->filename, FILE_TRANSFER_SERVER_DROP,
                                                      &drop_info);
    free(event->filename);
}

static void handle_quit_event(FrontendQuitEvent* event) {
    if (event->quit_application) {
        const char* quit_client_app_notification = "QUIT_APPLICATION";
        LOG_INFO("%s", quit_client_app_notification);
    }

    LOG_INFO("The user triggered a Quit event! WhistClient is now Quitting...");
    client_exiting = true;
}

static int handle_frontend_event(WhistFrontendEvent* event) {
    if (event->type != FRONTEND_EVENT_MOUSE_WHEEL && active_momentum_scroll) {
        // Cancel momentum scrolls on non-wheel events
        active_momentum_scroll = false;
    }

    switch (event->type) {
        case FRONTEND_EVENT_RESIZE: {
            sdl_renderer_resize_window(event->frontend, event->resize.width, event->resize.height);
            break;
        }
        case FRONTEND_EVENT_VISIBILITY: {
            if (event->visibility.visible) {
                LOG_INFO("Window now visible -- start streaming");
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_START_STREAMING;
                send_wcmsg(&wcmsg);
            } else {
                LOG_INFO("Window now hidden -- stop streaming");
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_STOP_STREAMING;
                send_wcmsg(&wcmsg);
            }
            break;
        }
        case FRONTEND_EVENT_AUDIO_UPDATE: {
            // Mark the audio device as pending an update
            atomic_fetch_or(&g_pending_audio_device_update, 1);
            break;
        }
        case FRONTEND_EVENT_KEYPRESS: {
            handle_keypress_event(&event->keypress);
            break;
        }
        case FRONTEND_EVENT_MOUSE_MOTION: {
            handle_mouse_motion_event(&event->mouse_motion);
            break;
        }
        case FRONTEND_EVENT_MOUSE_BUTTON: {
            handle_mouse_button_event(&event->mouse_button);
            break;
        }
        case FRONTEND_EVENT_MOUSE_WHEEL: {
            handle_mouse_wheel_event(&event->mouse_wheel);
            break;
        }
        case FRONTEND_EVENT_MOUSE_LEAVE: {
            break;
        }
        case FRONTEND_EVENT_GESTURE: {
            handle_gesture_event(&event->gesture);
            break;
        }
        case FRONTEND_EVENT_FILE_DROP: {
            handle_file_drop_event(event->frontend, &event->file_drop);
            break;
        }
        case FRONTEND_EVENT_QUIT: {
            handle_quit_event(&event->quit);
            break;
        }
        case FRONTEND_EVENT_UNHANDLED: {
            break;
        }
    }
    return 0;
}
