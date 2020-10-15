#include "input_driver.h"

#define KeyUp(input_device, sdl_keycode) EmitKeyEvent(input_device, sdl_keycode, 0)
#define KeyDown(input_device, sdl_keycode) EmitKeyEvent(input_device, sdl_keycode, 1)

unsigned int last_input_fmsg_id = 0;

void ResetInput() { last_input_fmsg_id = 0; }

void UpdateKeyboardState(input_device_t* input_device, FractalClientMessage* fmsg) {
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

    bool server_caps_lock = GetKeyboardModifierState(input_device, FK_CAPSLOCK);
    bool server_num_lock = GetKeyboardModifierState(input_device, FK_NUMLOCK);

    bool client_caps_lock_holding = fmsg->keyboard_state[FK_CAPSLOCK];
    bool client_num_lock_holding = fmsg->keyboard_state[FK_NUMLOCK];

    for (int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes; ++sdl_keycode) {
        if (!fmsg->keyboard_state[sdl_keycode] && GetKeyboardKeyState(input_device, sdl_keycode)) {
            KeyUp(input_device, sdl_keycode);
        } else if (fmsg->keyboard_state[sdl_keycode] &&
                   !GetKeyboardKeyState(input_device, sdl_keycode)) {
            KeyDown(input_device, sdl_keycode);

            if (sdl_keycode == FK_CAPSLOCK) {
                server_caps_lock = !server_caps_lock;
            }
            if (sdl_keycode == FK_NUMLOCK) {
                server_num_lock = !server_num_lock;
            }
        }
    }

    if (!!server_caps_lock != !!fmsg->caps_lock) {
        LOG_INFO("Caps lock out of sync, updating! From %s to %s\n",
                 server_caps_lock ? "caps" : "no caps", fmsg->caps_lock ? "caps" : "no caps");
        if (client_caps_lock_holding) {
            KeyUp(input_device, FK_CAPSLOCK);
            KeyDown(input_device, FK_CAPSLOCK);
        } else {
            KeyDown(input_device, FK_CAPSLOCK);
            KeyUp(input_device, FK_CAPSLOCK);
        }
    }

    if (!!server_num_lock != !!fmsg->num_lock) {
        LOG_INFO("Num lock out of sync, updating! From %s to %s\n",
                 server_num_lock ? "num lock" : "no num lock",
                 fmsg->num_lock ? "num lock" : "no num lock");
        if (client_num_lock_holding) {
            KeyUp(input_device, FK_NUMLOCK);
            KeyDown(input_device, FK_NUMLOCK);
        } else {
            KeyDown(input_device, FK_NUMLOCK);
            KeyUp(input_device, FK_NUMLOCK);
        }
    }
}

bool ReplayUserInput(input_device_t* input_device, FractalClientMessage* fmsg) {
    if (fmsg->id <= last_input_fmsg_id) {
        // Ignore Old FractalClientMessage
        return true;
    }
    last_input_fmsg_id = fmsg->id;

    int ret = 0;
    switch (fmsg->type) {
        case MESSAGE_KEYBOARD:
            ret = EmitKeyEvent(input_device, fmsg->keyboard.code, fmsg->keyboard.pressed);
            break;
        case MESSAGE_MOUSE_MOTION:
            ret = EmitMouseMotionEvent(input_device, fmsg->mouseMotion.x, fmsg->mouseMotion.y,
                                       fmsg->mouseMotion.relative);
            break;
        case MESSAGE_MOUSE_BUTTON:
            ret = EmitMouseButtonEvent(input_device, fmsg->mouseButton.button,
                                       fmsg->mouseButton.pressed);
            break;
        case MESSAGE_MOUSE_WHEEL:
            ret = EmitMouseWheelEvent(input_device, fmsg->mouseWheel.x, fmsg->mouseWheel.y);
            break;
        case MESSAGE_MULTIGESTURE:
            LOG_WARNING("Multi-touch support not added yet!");
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

size_t InputKeycodes(input_device_t* input_device, FractalKeycode* keycodes, size_t count) {
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
