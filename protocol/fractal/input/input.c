void UpdateKeyboardState(input_device_t* input_device, FractalClientMessage* fmsg) {
    if (fmsg->type != MESSAGE_KEYBOARD_STATE) {
        LOG_WARNING(
            "updateKeyboardState requires fmsg.type to be "
            "MESSAGE_KEYBOARD_STATE");
        return;
    }

    bool server_caps_lock = GetServerModifierState(input_device, FK_CAPSLOCK);
    bool server_num_lock = GetServerModifierState(input_device, FK_NUMLOCK);

    bool client_caps_lock_holding = fmsg->keyboard_state[FK_CAPSLOCK];
    bool client_num_lock_holding = fmsg->keyboard_state[FK_NUMLOCK];

    for (int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes; ++sdl_keycode) {
        int server_keycode = GetServerKeyCode(sdl_keycode);

        if (!server_keycode) continue;

        if (!fmsg->keyboard_state[sdl_keycode] && GetServerKeyState(input_device, sdl_keycode)) {
            KeyUp(input_device, server_keycode);
        } else if (fmsg->keyboard_state[sdl_keycode] &&
                   !GetServerKeyState(input_device, server_keycode)) {
            KeyDown(input_device, server_keycode);

            if (server_keycode == FK_CAPSLOCK) {
                server_caps_lock = !server_caps_lock;
            }
            if (server_keycode == FK_NUMLOCK) {
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
            KeyUp(input_device, KEY_NUMLOCK);
            KeyDown(input_device, KEY_NUMLOCK);
        } else {
            KeyDown(input_device, KEY_NUMLOCK);
            KeyUp(input_device, KEY_NUMLOCK);
        }
    }

    SyncInput(input_device);
}