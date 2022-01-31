/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file input.c
 * @brief This file defines general input-processing functions and toggles
 *        between Windows and Linux servers.
============================
Usage
============================

Toggles dynamically between input receiving on Windows or Linux Ubuntu
computers. You can create an input device to receive input (keystrokes, mouse
clicks, etc.) via CreateInputDevice. You can then send input to the Windows OS
via ReplayUserInput, and use UpdateKeyboardState to sync keyboard state between
local and remote computers (say, sync them to both have CapsLock activated).
Lastly, you can input an array of keycodes using `InputKeycodes`.
*/

/*
============================
Includes
============================
*/

#include <whist/utils/os_utils.h>
#include "input.h"
#include "input_internal.h"
#include "keyboard_mapping.h"

/*
============================
Defines
============================
*/

#define KeyUp(input_device, whist_keycode) emit_key_event(input_device, whist_keycode, 0)
#define KeyDown(input_device, whist_keycode) emit_key_event(input_device, whist_keycode, 1)

static unsigned int last_input_fcmsg_id = 0;
static WhistOSType input_os_type = WHIST_UNKNOWN_OS;

/*
============================
Public Function Implementations
============================
*/

InputDevice* create_input_device(InputDeviceType kind) {
    switch (kind) {
#if defined(__linux__)
        case WHIST_INPUT_DEVICE_XTEST:
            return xtest_create_input_device();

        case WHIST_INPUT_DEVICE_UINPUT:
            return uinput_create_input_device();
#endif

#ifdef _WIN32
        case WHIST_INPUT_DEVICE_WIN32:
            return win_create_input_device();
#endif

        case WHIST_INPUT_DEVICE_WESTON:
        default:
            return NULL;
    }
}

void destroy_input_device(InputDevice* dev) {
    FATAL_ASSERT(dev != NULL);
    dev->destroy();
}

int get_keyboard_modifier_state(InputDevice* dev, WhistKeycode keycode) {
    return dev->get_keyboard_modifier_state(dev, keycode);
}

int get_keyboard_key_state(InputDevice* dev, WhistKeycode keycode) {
    return dev->get_keyboard_key_state(dev, keycode);
}

int ignore_key_state(InputDevice* dev, WhistKeycode keycode, bool active_pinch) {
    return dev->ignore_key_state(dev, keycode, active_pinch);
}
int emit_key_event(InputDevice* dev, WhistKeycode keycode, int pressed) {
    return dev->emit_key_event(dev, keycode, pressed);
}

int emit_mouse_motion_event(InputDevice* dev, int32_t x, int32_t y, int relative) {
    return dev->emit_mouse_motion_event(dev, x, y, relative);
}

int emit_mouse_button_event(InputDevice* dev, WhistMouseButton button, int pressed) {
    return dev->emit_mouse_button_event(dev, button, pressed);
}

int emit_low_res_mouse_wheel_event(InputDevice* dev, int32_t x, int32_t y) {
    return dev->emit_low_res_mouse_wheel_event(dev, x, y);
}

int emit_high_res_mouse_wheel_event(InputDevice* dev, float x, float y) {
    return dev->emit_high_res_mouse_wheel_event(dev, x, y);
}

int emit_multigesture_event(InputDevice* dev, float d_theta, float d_dist,
                            WhistMultigestureType gesture_type, bool active_gesture) {
    return dev->emit_multigesture_event(dev, d_theta, d_dist, gesture_type, active_gesture);
}

void reset_input(WhistOSType os_type) {
    /*
        Initialize the input system.
        NOTE: Should be used prior to every new client connection
              that will send inputs
    */

    input_os_type = os_type;
    last_input_fcmsg_id = 0;
}

void update_keyboard_state(InputDevice* input_device, WhistClientMessage* fcmsg) {
    /*
        Updates the keyboard state on the server to match the client's

        Arguments:
            input_device (InputDevice*): The initialized input device struct defining
                mouse and keyboard states for the user
            fcmsg (WhistClientMessage*): The Whist message packet, defining one
                keyboard event, to update the keyboard state
    */

    if (fcmsg->id <= last_input_fcmsg_id) {
        // Ignore Old WhistClientMessage
        return;
    }
    last_input_fcmsg_id = fcmsg->id;

    if (fcmsg->type != MESSAGE_KEYBOARD_STATE) {
        LOG_WARNING(
            "updateKeyboardState requires fcmsg.type to be "
            "MESSAGE_KEYBOARD_STATE");
        return;
    }

    set_keyboard_layout(fcmsg->keyboard_state.layout);

    update_mapped_keyboard_state(input_device, input_os_type, fcmsg->keyboard_state);
}

bool replay_user_input(InputDevice* input_device, WhistClientMessage* fcmsg) {
    /*
        Replayed a received user action on a server, by sending it to the OS

        Arguments:
            input_device (InputDevice*): The initialized input device struct defining
                mouse and keyboard states for the user
            fcmsg (WhistClientMessage*): The Whist message packet, defining one user
                action event, to replay on the computer

        Returns:
            (bool): True if it replayed the event, False otherwise
    */

    if (fcmsg->id <= last_input_fcmsg_id) {
        // Ignore Old WhistClientMessage
        return true;
    }
    last_input_fcmsg_id = fcmsg->id;

    int ret = 0;
    switch (fcmsg->type) {
        case MESSAGE_KEYBOARD:
            ret = emit_mapped_key_event(input_device, input_os_type, fcmsg->keyboard.code,
                                        fcmsg->keyboard.pressed);
            break;
        case MESSAGE_MOUSE_MOTION:
            ret = emit_mouse_motion_event(input_device, fcmsg->mouseMotion.x, fcmsg->mouseMotion.y,
                                          fcmsg->mouseMotion.relative);
            break;
        case MESSAGE_MOUSE_BUTTON:
            ret = emit_mouse_button_event(input_device, fcmsg->mouseButton.button,
                                          fcmsg->mouseButton.pressed);
            break;
        case MESSAGE_MOUSE_WHEEL:
            if (input_device->emit_high_res_mouse_wheel_event) {
                // Prefer the high res event, if it exists
                ret = emit_high_res_mouse_wheel_event(input_device, fcmsg->mouseWheel.precise_x,
                                                      fcmsg->mouseWheel.precise_y);
            } else {
                // Otherwise, pass in the low res mouse event
                ret = emit_low_res_mouse_wheel_event(input_device, fcmsg->mouseWheel.x,
                                                     fcmsg->mouseWheel.y);
            }
            break;
        case MESSAGE_MULTIGESTURE:
            ret = emit_multigesture_event(
                input_device, fcmsg->multigesture.d_theta, fcmsg->multigesture.d_dist,
                fcmsg->multigesture.gesture_type, fcmsg->multigesture.active_gesture);
            break;
        default:
            LOG_ERROR("Unknown message type! %d", fcmsg->type);
            break;
    }

    if (ret) {
        LOG_WARNING("Failed to handle message of type %d", fcmsg->type);
    }

    return true;
}

size_t input_keycodes(InputDevice* input_device, WhistKeycode* keycodes, size_t count) {
    /*
        Updates the keyboard state on the server to match the client's

        Arguments:
            input_device (InputDevice*): The initialized input device struct defining
                mouse and keyboard states for the user
            keycodes (WhistKeycode*): A pointer to an array of keycodes to sequentially
                input
            count (size_t): The number of keycodes to read from `keycodes`

        Returns:
            (size_t): The number of keycodes successfully inputted from the array
    */

    for (unsigned int i = 0; i < count; ++i) {
        if (KeyDown(input_device, keycodes[i])) {
            LOG_WARNING("Error pressing keycode %d!", keycodes[i]);
            return i;
        }
        if (KeyUp(input_device, keycodes[i])) {
            LOG_WARNING("Error unpressing keycode %d!", keycodes[i]);
            return i + 1;
        }
    }
    return count;
}
