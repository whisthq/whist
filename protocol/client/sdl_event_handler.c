/**
 * Copyright 2021 Whist Technologies, Inc.
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
Private Functions
============================
*/

int handle_key_up_down(SDL_Event *event);
int handle_mouse_motion(SDL_Event *event);
int handle_mouse_wheel(SDL_Event *event);
int handle_mouse_button_up_down(SDL_Event *event);
int handle_multi_gesture(SDL_Event *event);
int handle_pinch(SDL_Event *event);

/*
============================
Private Function Implementations
============================
*/

int handle_key_up_down(SDL_Event *event) {
    /*
        Handle the SDL key press or release event

        Arguments:
            event (SDL_event*): SDL event for key press/release

        Return:
            (int): 0 on success
    */

    WhistKeycode keycode =
        (WhistKeycode)SDL_GetScancodeFromName(SDL_GetKeyName(event->key.keysym.sym));
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

int handle_mouse_motion(SDL_Event *event) {
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

int handle_mouse_button_up_down(SDL_Event *event) {
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

int handle_mouse_wheel(SDL_Event *event) {
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

int handle_pinch(SDL_Event *event) {
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

int handle_multi_gesture(SDL_Event *event) {
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

/*
============================
Public Function Implementations
============================
*/

int try_handle_sdl_event(void) {
    /*
        Handle an SDL event if polled

        Return:
            (int): 0 on success, -1 on failure
    */

    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        if (handle_sdl_event(&event) != 0) {
            return -1;
        }
    }
    return 0;
}

int handle_sdl_event(SDL_Event *event) {
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
                sdl_resize_window(event->window.data1, event->window.data2);
            }
#ifdef __APPLE__
            else if (event->window.event == SDL_WINDOWEVENT_OCCLUDED) {
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_STOP_STREAMING;
                whist_sleep(100);
                send_wcmsg(&wcmsg);
            } else if (event->window.event == SDL_WINDOWEVENT_UNOCCLUDED) {
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_START_STREAMING;
                send_wcmsg(&wcmsg);
            }
#else
            else if (event->window.event == SDL_WINDOWEVENT_MINIMIZED) {
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_STOP_STREAMING;
                send_wcmsg(&wcmsg);
            } else if (event->window.event == SDL_WINDOWEVENT_RESTORED) {
                WhistClientMessage wcmsg = {0};
                wcmsg.type = MESSAGE_START_STREAMING;
                send_wcmsg(&wcmsg);
            }
#endif
            break;
        }
        case SDL_AUDIODEVICEADDED:
        case SDL_AUDIODEVICEREMOVED: {
            // Refresh the audio device
            enable_audio_refresh();
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
        case SDL_QUIT: {
            LOG_INFO("The user triggered a Quit event! WhistClient is now Quitting...");
            client_exiting = true;
            break;
        }
    }
    return 0;
}
