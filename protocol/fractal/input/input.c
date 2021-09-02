/**
 * Copyright Fractal Computers, Inc. 2020
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

#include "input_driver.h"
#include "keyboard_mapping.h"

/*
============================
Defines
============================
*/

#define KeyUp(input_device, fractal_keycode) emit_key_event(input_device, fractal_keycode, 0)
#define KeyDown(input_device, fractal_keycode) emit_key_event(input_device, fractal_keycode, 1)

unsigned int last_input_fmsg_id = 0;
FractalOSType input_os_type = FRACTAL_UNKNOWN_OS;

/*
============================
Public Function Implementations
============================
*/

void reset_input(FractalOSType os_type) {
    /*
        Initialize the input system.
        NOTE: Should be used prior to every new client connection
              that will send inputs
    */

    input_os_type = os_type;
    last_input_fmsg_id = 0;
}

void update_keyboard_state(InputDevice* input_device, FractalClientMessage* fmsg) {
    /*
        Updates the keyboard state on the server to match the client's

        Arguments:
            input_device (InputDevice*): The initialized input device struct defining
                mouse and keyboard states for the user
            fmsg (FractalClientMessage*): The Fractal message packet, defining one
                keyboard event, to update the keyboard state
    */

    if (fmsg->id <= last_input_fmsg_id) {
        // Ignore Old FractalClientMessage
        return;
    }
    last_input_fmsg_id = fmsg->id;

    if (fmsg->type != MESSAGE_KEYBOARD_STATE) {
        LOG_WARNING(
            "updateKeyboardState requires fmsg.type to be "
            "MESSAGE_KEYBOARD_STATE");
        return;
    }

    update_mapped_keyboard_state(input_device, input_os_type, fmsg->keyboard_state);
}

bool replay_user_input(InputDevice* input_device, FractalClientMessage* fmsg) {
    /*
        Replayed a received user action on a server, by sending it to the OS

        Arguments:
            input_device (InputDevice*): The initialized input device struct defining
                mouse and keyboard states for the user
            fmsg (FractalClientMessage*): The Fractal message packet, defining one user
                action event, to replay on the computer

        Returns:
            (bool): True if it replayed the event, False otherwise
    */

    if (fmsg->id <= last_input_fmsg_id) {
        // Ignore Old FractalClientMessage
        return true;
    }
    last_input_fmsg_id = fmsg->id;

    int ret = 0;
    switch (fmsg->type) {
        case MESSAGE_KEYBOARD:
            ret = emit_mapped_key_event(input_device, input_os_type, fmsg->keyboard.code,
                                        fmsg->keyboard.pressed);
            break;
        case MESSAGE_MOUSE_MOTION:
            ret = emit_mouse_motion_event(input_device, fmsg->mouseMotion.x, fmsg->mouseMotion.y,
                                          fmsg->mouseMotion.relative);
            break;
        case MESSAGE_MOUSE_BUTTON:
            ret = emit_mouse_button_event(input_device, fmsg->mouseButton.button,
                                          fmsg->mouseButton.pressed);
            break;
        case MESSAGE_MOUSE_WHEEL:
#if INPUT_DRIVER == UINPUT_INPUT_DRIVER
            ret = emit_high_res_mouse_wheel_event(input_device, fmsg->mouseWheel.precise_x,
                                                  fmsg->mouseWheel.precise_y);
#else
            ret = emit_low_res_mouse_wheel_event(input_device, fmsg->mouseWheel.x,
                                                 fmsg->mouseWheel.y);
#endif  // INPUT_DRIVER
            break;
        case MESSAGE_MULTIGESTURE:
            ret = emit_multigesture_event(
                input_device, fmsg->multigesture.d_theta, fmsg->multigesture.d_dist,
                fmsg->multigesture.gesture_type, fmsg->multigesture.active_gesture);
            break;
        default:
            LOG_ERROR("Unknown message type! %d", fmsg->type);
            break;
    }

    if (ret) {
        LOG_WARNING("Failed to handle message of type %d", fmsg->type);
    }

    return true;
}

size_t input_keycodes(InputDevice* input_device, FractalKeycode* keycodes, size_t count) {
    /*
        Updates the keyboard state on the server to match the client's

        Arguments:
            input_device (InputDevice*): The initialized input device struct defining
                mouse and keyboard states for the user
            keycodes (FractalKeycode*): A pointer to an array of keycodes to sequentially
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
