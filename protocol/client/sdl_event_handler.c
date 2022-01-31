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
#include "sdlscreeninfo.h"
#include "sdl_utils.h"
#include "audio.h"
#include "client_utils.h"
#include "network.h"
#include "native_window_utils.h"
#include <whist/utils/atomic.h>

// Keyboard state variables
bool alt_pressed = false;
bool ctrl_pressed = false;
bool lgui_pressed = false;
bool rgui_pressed = false;

// Main state variables
extern bool client_exiting;

extern volatile SDL_Window *window;

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

atomic_int g_num_pending_audio_device_updates = ATOMIC_VAR_INIT(0);

/*
============================
Private Functions Declarations
============================
*/

static int handle_sdl_event(SDL_Event *event);
static int handle_key_up_down(SDL_Event *event);
static int handle_mouse_motion(SDL_Event *event);
static int handle_mouse_wheel(SDL_Event *event);
static int handle_mouse_button_up_down(SDL_Event *event);
static int handle_multi_gesture(SDL_Event *event);
static int handle_pinch(SDL_Event *event);

/*
============================
Public Function Implementations
============================
*/

bool sdl_handle_events(void) {
    // We cannot use SDL_WaitEventTimeout here, because
    // Linux seems to treat a 1ms timeout as an infinite timeout
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        if (handle_sdl_event(&sdl_event) != 0) {
            return false;
        }
    }

    // After handle_sdl_event potentially captures a mouse motion,
    // We throttle it down to only update once every 0.5ms
    static WhistTimer mouse_motion_timer;
    static bool first_mouse_motion = true;
    if (first_mouse_motion || get_timer(&mouse_motion_timer) * MS_IN_SECOND > 0.5) {
        if (update_mouse_motion() != 0) {
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
    int num_pending_audio_device_updates = atomic_exchange(&g_num_pending_audio_device_updates, 0);
    return num_pending_audio_device_updates > 0;
}

/*
============================
Private Function Implementations
============================
*/

static int handle_key_up_down(SDL_Event *event) {
    /*
        Handle the SDL key press or release event

        Arguments:
            event (SDL_event*): SDL event for key press/release

        Return:
            (int): 0 on success
    */

    WhistKeycode keycode = (WhistKeycode)event->key.keysym.scancode;
    bool is_pressed = event->key.type == SDL_KEYDOWN;

    // LOG_INFO("Scancode: %d", event->key.keysym.scancode);
    // LOG_INFO("Keycode: %d %d", keycode, is_pressed);
    // LOG_INFO("%s %s", (is_pressed ? "Pressed" : "Released"),
    // SDL_GetKeyName(event->key.keysym.sym));

    // Keep memory of alt/ctrl/lgui/rgui status
    if (keycode == FK_LALT) {
        alt_pressed = is_pressed;
    }
    if (keycode == FK_LCTRL) {
        ctrl_pressed = is_pressed;
    }
    if (keycode == FK_RCTRL) {
        ctrl_pressed = is_pressed;
    }
    if (keycode == FK_LGUI) {
        lgui_pressed = is_pressed;
    }
    if (keycode == FK_RGUI) {
        rgui_pressed = is_pressed;
    }

    if (ctrl_pressed && alt_pressed && keycode == FK_F4) {
        LOG_INFO("Quitting...");
        client_exiting = true;
    }

    WhistClientMessage wcmsg = {0};
    wcmsg.type = MESSAGE_KEYBOARD;
    wcmsg.keyboard.code = keycode;
    wcmsg.keyboard.pressed = is_pressed;
    wcmsg.keyboard.mod = event->key.keysym.mod;
    send_wcmsg(&wcmsg);

    return 0;
}

static int handle_mouse_motion(SDL_Event *event) {
    /*
        Handle the SDL mouse motion event

        Arguments:
            event (SDL_Event*): SDL event for mouse motion

        Result:
            (int): 0 on success, -1 on failure
    */

    // Relative motion is the delta x and delta y from last
    // mouse position Absolute mouse position is where it is
    // on the screen We multiply by scaling factor so that
    // integer division doesn't destroy accuracy
    bool is_relative = SDL_GetRelativeMouseMode() == SDL_TRUE;

    if (is_relative && !mouse_state.is_relative) {
        // old datum was absolute, new is relative
        // Hence, we flush out the old datum
        if (update_mouse_motion() != 0) {
            return -1;
        }
    }

    mouse_state.x_nonrel = event->motion.x;
    mouse_state.y_nonrel = event->motion.y;
    mouse_state.is_relative = is_relative;

    if (is_relative) {
        mouse_state.x_rel += event->motion.xrel;
        mouse_state.y_rel += event->motion.yrel;
    }

    mouse_state.update = true;

    return 0;
}

static int handle_mouse_button_up_down(SDL_Event *event) {
    /*
        Handle the SDL mouse button press/release event

        Arguments:
            event (SDL_Event*): SDL event for mouse button press/release event

        Result:
            (int): 0 on success
    */

    WhistClientMessage wcmsg = {0};
    wcmsg.type = MESSAGE_MOUSE_BUTTON;
    // Record if left / right / middle button
    wcmsg.mouseButton.button = event->button.button;
    wcmsg.mouseButton.pressed = event->button.type == SDL_MOUSEBUTTONDOWN;
    if (wcmsg.mouseButton.button == MOUSE_L) {
        SDL_CaptureMouse(wcmsg.mouseButton.pressed);
    }
    send_wcmsg(&wcmsg);

    return 0;
}

static int handle_mouse_wheel(SDL_Event *event) {
    /*
        Handle the SDL mouse wheel event

        Arguments:
            event (SDL_Event*): SDL event for mouse wheel event

        Result:
            (int): 0 on success
    */

    // If a pinch is active, don't send a scroll event
    if (active_pinch) {
        return 0;
    }

    WhistMouseWheelMomentumType momentum_phase =
        (WhistMouseWheelMomentumType)event->wheel.momentum_phase;

    if (momentum_phase == MOUSEWHEEL_MOMENTUM_BEGIN) {
        active_momentum_scroll = true;
    } else if (momentum_phase == MOUSEWHEEL_MOMENTUM_END ||
               momentum_phase == MOUSEWHEEL_MOMENTUM_NONE) {
        active_momentum_scroll = false;
    }

    // The active momentum scroll has been interrupted and aborted
    if (momentum_phase == MOUSEWHEEL_MOMENTUM_ACTIVE && !active_momentum_scroll) {
        return 0;
    }

    WhistClientMessage wcmsg = {0};
    wcmsg.type = MESSAGE_MOUSE_WHEEL;
    wcmsg.mouseWheel.x = event->wheel.x;
    wcmsg.mouseWheel.y = event->wheel.y;
    wcmsg.mouseWheel.precise_x = event->wheel.preciseX;
    wcmsg.mouseWheel.precise_y = event->wheel.preciseY;
    send_wcmsg(&wcmsg);

    return 0;
}

static int handle_pinch(SDL_Event *event) {
    /*
        Handle the SDL pinch event

        Arguments:
            event (SDL_Event*): SDL event for touchpad pinch event

        Result:
            (int): 0 on success
    */

    WhistClientMessage wcmsg = {0};
    wcmsg.type = MESSAGE_MULTIGESTURE;
    wcmsg.multigesture = (WhistMultigestureMessage){.d_theta = 0,
                                                    .d_dist = event->pinch.scroll_amount,
                                                    .x = 0,
                                                    .y = 0,
                                                    .num_fingers = 2,
                                                    .active_gesture = active_pinch};

    wcmsg.multigesture.gesture_type = MULTIGESTURE_NONE;
    if (event->pinch.magnification < 0) {
        wcmsg.multigesture.gesture_type = MULTIGESTURE_PINCH_CLOSE;
        active_pinch = true;
    } else if (event->pinch.magnification > 0) {
        wcmsg.multigesture.gesture_type = MULTIGESTURE_PINCH_OPEN;
        active_pinch = true;
    } else if (active_pinch) {
        // 0 magnification means that the pinch gesture is complete
        wcmsg.multigesture.gesture_type = MULTIGESTURE_CANCEL;
        active_pinch = false;
    }

    send_wcmsg(&wcmsg);

    return 0;
}

static int handle_multi_gesture(SDL_Event *event) {
    WhistClientMessage wcmsg = {0};
    wcmsg.type = MESSAGE_MULTIGESTURE;
    wcmsg.multigesture = (WhistMultigestureMessage){.d_theta = event->mgesture.dTheta,
                                                    .d_dist = event->mgesture.dDist,
                                                    .x = event->mgesture.x,
                                                    .y = event->mgesture.y,
                                                    .num_fingers = event->mgesture.numFingers,
                                                    .gesture_type = MULTIGESTURE_NONE};
    send_wcmsg(&wcmsg);

    return 0;
}

static int handle_file_drop(SDL_Event *event) {
    int mouse_x, mouse_y;
    SDL_CaptureMouse(true);
    SDL_GetMouseState(&mouse_x, &mouse_y);
    SDL_CaptureMouse(false);
    FileEventInfo drop_event_info;
    // Scale the mouse position for server-side compatibility
    drop_event_info.server_drop.x = mouse_x * get_native_window_dpi((SDL_Window *)window) / 96;
    drop_event_info.server_drop.y = mouse_y * get_native_window_dpi((SDL_Window *)window) / 96;
    file_synchronizer_set_file_reading_basic_metadata(event->drop.file, FILE_TRANSFER_SERVER_DROP,
                                                      &drop_event_info);

    return 0;
}

static int handle_sdl_event(SDL_Event *event) {
    /*
        Handle SDL event based on type

        Return:
            (int): 0 on success, -1 on failure
    */

    // Allow non-scroll events to interrupt momentum scrolls
    if (event->type != SDL_MOUSEWHEEL && active_momentum_scroll) {
        active_momentum_scroll = false;
    }

    switch (event->type) {
        case SDL_WINDOWEVENT: {
            if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                sdl_renderer_resize_window(event->window.data1, event->window.data2);
            }
#ifdef __APPLE__
            else if (event->window.event == SDL_WINDOWEVENT_OCCLUDED) {
                LOG_INFO("SDL_WINDOWEVENT_OCCLUDED - Stop Streaming");
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_STOP_STREAMING;
                whist_sleep(100);
                send_wcmsg(&wcmsg);
            } else if (event->window.event == SDL_WINDOWEVENT_UNOCCLUDED) {
                LOG_INFO("SDL_WINDOWEVENT_UNOCCLUDED - Start Streaming");
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_START_STREAMING;
                send_wcmsg(&wcmsg);
            }
#else
            else if (event->window.event == SDL_WINDOWEVENT_MINIMIZED) {
                LOG_INFO("SDL_WINDOWEVENT_MINIMIZED - Stop Streaming");
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_STOP_STREAMING;
                send_wcmsg(&wcmsg);
            } else if (event->window.event == SDL_WINDOWEVENT_RESTORED) {
                LOG_INFO("SDL_WINDOWEVENT_RESTORED - Start Streaming");
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_START_STREAMING;
                send_wcmsg(&wcmsg);
            }
#endif
            break;
        }
        case SDL_AUDIODEVICEADDED:
        case SDL_AUDIODEVICEREMOVED: {
            // Mark the audio device as pending an update "1"
            atomic_fetch_add(&g_num_pending_audio_device_updates, 1);
            break;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            if (handle_key_up_down(event) != 0) {
                return -1;
            }
            break;
        }
        case SDL_MOUSEMOTION: {
            if (handle_mouse_motion(event) != 0) {
                return -1;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            if (handle_mouse_button_up_down(event) != 0) {
                return -1;
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            if (handle_mouse_wheel(event) != 0) {
                return -1;
            }
            break;
        }
        case SDL_MULTIGESTURE: {
            if (handle_multi_gesture(event) != 0) {
                return -1;
            }
            break;
        }
        case SDL_PINCH: {
            if (handle_pinch(event) != 0) {
                return -1;
            }
            break;
        }
        case SDL_DROPFILE: {
            if (handle_file_drop(event) != 0) {
                return -1;
            }
            break;
        }
        case SDL_QUIT: {
            LOG_INFO("The user triggered a Quit event! WhistClient is now Quitting...");
            client_exiting = true;
            break;
        }
    }
    return 0;
}
