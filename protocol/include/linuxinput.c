#include "linuxinput.h"  // header file for this file

// @brief Linux keycodes for replaying SDL user inputs on server
// @details index is SDL keycode, value is Linux keycode.
// To debug specific keycodes, use 'sudo showkey --keycodes'.
const int linux_keycodes[NUM_KEYCODES] = {
    0,               // SDL keycodes start at index 4
    0,               // SDL keycodes start at index 4
    0,               // SDL keycodes start at index 4
    0,               // SDL keycodes start at index 4
    KEY_A,           // 4 -> A
    KEY_B,           // 5 -> B
    KEY_C,           // 6 -> C
    KEY_D,           // 7 -> D
    KEY_E,           // 8 -> E
    KEY_F,           // 9 -> F
    KEY_G,           // 10 -> G
    KEY_H,           // 11 -> H
    KEY_I,           // 12 -> I
    KEY_J,           // 13 -> J
    KEY_K,           // 14 -> K
    KEY_L,           // 15 -> L
    KEY_M,           // 16 -> M
    KEY_N,           // 17 -> N
    KEY_O,           // 18 -> O
    KEY_P,           // 19 -> P
    KEY_Q,           // 20 -> Q
    KEY_R,           // 21 -> R
    KEY_S,           // 22 -> S
    KEY_T,           // 23 -> T
    KEY_U,           // 24 -> U
    KEY_V,           // 25 -> V
    KEY_W,           // 26 -> W
    KEY_X,           // 27 -> X
    KEY_Y,           // 28 -> Y
    KEY_Z,           // 29 -> Z
    KEY_1,           // 30 -> 1
    KEY_2,           // 31 -> 2
    KEY_3,           // 32 -> 3
    KEY_4,           // 33 -> 4
    KEY_5,           // 34 -> 5
    KEY_6,           // 35 -> 6
    KEY_7,           // 36 -> 7
    KEY_8,           // 37 -> 8
    KEY_9,           // 38 -> 9
    KEY_0,           // 39 -> 0
    KEY_ENTER,       // 40 -> Enter
    KEY_ESC,         // 41 -> Escape
    KEY_BACKSPACE,   // 42 -> Backspace
    KEY_TAB,         // 43 -> Tab
    KEY_SPACE,       // 44 -> Space
    KEY_MINUS,       // 45 -> Minus
    KEY_EQUAL,       // 46 -> Equal
    KEY_LEFTBRACE,   // 47 -> Left Bracket
    KEY_RIGHTBRACE,  // 48 -> Right Bracket
    KEY_BACKSLASH,   // 49 -> Backslash
    0,               // 50 -> no SDL keycode at index 50
    KEY_SEMICOLON,   // 51 -> Semicolon
    KEY_APOSTROPHE,  // 52 -> Apostrophe
    KEY_GRAVE,       // 53 -> Backtick
    KEY_COMMA,       // 54 -> Comma
    KEY_DOT,         // 55 -> Period
    KEY_SLASH,       // 56 -> Forward Slash
    KEY_CAPSLOCK,    // 57 -> Capslock
    KEY_F1,          // 58 -> F1
    KEY_F2,          // 59 -> F2
    KEY_F3,          // 60 -> F3
    KEY_F4,          // 61 -> F4
    KEY_F5,          // 62 -> F5
    KEY_F6,          // 63 -> F6
    KEY_F7,          // 64 -> F7
    KEY_F8,          // 65 -> F8
    KEY_F9,          // 66 -> F9
    KEY_F10,         // 67 -> F10
    KEY_F11,         // 68 -> F11
    KEY_F12,         // 69 -> F12
    KEY_SYSRQ,       // 70 -> Print Screen
    KEY_SCROLLLOCK,  // 71 -> Scroll Lock
    KEY_PAUSE,       // 72 -> Pause
    KEY_INSERT,      // 73 -> Insert
    KEY_HOME,        // 74 -> Home
    KEY_PAGEUP,      // 75 -> Pageup
    KEY_DELETE,      // 76 -> Delete
    KEY_END,         // 77 -> End
    KEY_PAGEDOWN,    // 78 -> Pagedown
    KEY_RIGHT,       // 79 -> Right
    KEY_LEFT,        // 80 -> Left
    KEY_DOWN,        // 81 -> Down
    KEY_UP,          // 82 -> Up
    KEY_NUMLOCK,     // 83 -> Numlock
    KEY_KPSLASH,     // 84 -> Numeric Keypad Divide
    KEY_KPASTERISK,  // 85 -> Numeric Keypad Multiply
    KEY_KPMINUS,     // 86 -> Numeric Keypad Minus
    KEY_KPPLUS,      // 87 -> Numeric Keypad Plus
    KEY_KPENTER,     // 88 -> Numeric Keypad Enter
    KEY_KP1,         // 89 -> Numeric Keypad 1
    KEY_KP2,         // 90 -> Numeric Keypad 2
    KEY_KP3,         // 91 -> Numeric Keypad 3
    KEY_KP4,         // 92 -> Numeric Keypad 4
    KEY_KP5,         // 93 -> Numeric Keypad 5
    KEY_KP6,         // 94 -> Numeric Keypad 6
    KEY_KP7,         // 95 -> Numeric Keypad 7
    KEY_KP8,         // 96 -> Numeric Keypad 8
    KEY_KP9,         // 97 -> Numeric Keypad 9
    KEY_KP0,         // 98 -> Numeric Keypad 0
    KEY_KPDOT,       // 99 -> Numeric Keypad Period
    0,               // 100 -> no SDL keycode at index 100
    KEY_COMPOSE,     // 101 -> Application
    0,               // 102 -> no SDL keycode at index 102
    0,               // 103 -> no SDL keycode at index 103
    KEY_F13,         // 104 -> F13
    KEY_F14,         // 105 -> F14
    KEY_F15,         // 106 -> F15
    KEY_F16,         // 107 -> F16
    KEY_F17,         // 108 -> F17
    KEY_F18,         // 109 -> F18
    KEY_F19,         // 110 -> F19
    KEY_F20,         // 111 -> F20
    KEY_F21,         // 112 -> F21
    KEY_F22,         // 113 -> F22
    KEY_F23,         // 114 -> F23
    KEY_F24,         // 115 -> F24
    0,               // 116 -> Execute (can't find what this is supposed to be)
    KEY_HELP,        // 117 -> Help
    KEY_MENU,        // 118 -> Menu
    KEY_SELECT,      // 119 -> Select
    0,               // 120 -> no SDL keycode at index 120
    0,               // 121 -> no SDL keycode at index 121
    0,               // 122 -> no SDL keycode at index 122
    0,               // 123 -> no SDL keycode at index 123
    0,               // 124 -> no SDL keycode at index 124
    0,               // 125 -> no SDL keycode at index 125
    0,               // 126 -> no SDL keycode at index 126
    KEY_MUTE,        // 127 -> Mute
    KEY_VOLUMEUP,    // 128 -> Volume Up
    KEY_VOLUMEDOWN,  // 129 -> Volume Down
    0,               // 130 -> no SDL keycode at index 130
    0,               // 131 -> no SDL keycode at index 131
    0,               // 132 -> no SDL keycode at index 132
    0,               // 133 -> no SDL keycode at index 133
    0,               // 134 -> no SDL keycode at index 134
    0,               // 135 -> no SDL keycode at index 135
    0,               // 136 -> no SDL keycode at index 136
    0,               // 137 -> no SDL keycode at index 137
    0,               // 138 -> no SDL keycode at index 138
    0,               // 139 -> no SDL keycode at index 139
    0,               // 140 -> no SDL keycode at index 140
    0,               // 141 -> no SDL keycode at index 141
    0,               // 142 -> no SDL keycode at index 142
    0,               // 143 -> no SDL keycode at index 143
    0,               // 144 -> no SDL keycode at index 144
    0,               // 145 -> no SDL keycode at index 145
    0,               // 146 -> no SDL keycode at index 146
    0,               // 147 -> no SDL keycode at index 147
    0,               // 148 -> no SDL keycode at index 148
    0,               // 149 -> no SDL keycode at index 149
    0,               // 150 -> no SDL keycode at index 150
    0,               // 151 -> no SDL keycode at index 151
    0,               // 152 -> no SDL keycode at index 152
    0,               // 153 -> no SDL keycode at index 153
    0,               // 154 -> no SDL keycode at index 154
    0,               // 155 -> no SDL keycode at index 155
    0,               // 156 -> no SDL keycode at index 156
    0,               // 157 -> no SDL keycode at index 157
    0,               // 158 -> no SDL keycode at index 158
    0,               // 159 -> no SDL keycode at index 159
    0,               // 160 -> no SDL keycode at index 160
    0,               // 161 -> no SDL keycode at index 161
    0,               // 162 -> no SDL keycode at index 162
    0,               // 163 -> no SDL keycode at index 163
    0,               // 164 -> no SDL keycode at index 164
    0,               // 165 -> no SDL keycode at index 165
    0,               // 166 -> no SDL keycode at index 166
    0,               // 167 -> no SDL keycode at index 167
    0,               // 168 -> no SDL keycode at index 168
    0,               // 169 -> no SDL keycode at index 169
    0,               // 170 -> no SDL keycode at index 170
    0,               // 171 -> no SDL keycode at index 171
    0,               // 172 -> no SDL keycode at index 172
    0,               // 173 -> no SDL keycode at index 173
    0,               // 174 -> no SDL keycode at index 174
    0,               // 175 -> no SDL keycode at index 175
    0,               // 176 -> no SDL keycode at index 176
    0,               // 177 -> no SDL keycode at index 177
    0,               // 178 -> no SDL keycode at index 178
    0,               // 179 -> no SDL keycode at index 179
    0,               // 180 -> no SDL keycode at index 180
    0,               // 181 -> no SDL keycode at index 181
    0,               // 182 -> no SDL keycode at index 182
    0,               // 183 -> no SDL keycode at index 183
    0,               // 184 -> no SDL keycode at index 184
    0,               // 185 -> no SDL keycode at index 185
    0,               // 186 -> no SDL keycode at index 186
    0,               // 187 -> no SDL keycode at index 187
    0,               // 188 -> no SDL keycode at index 188
    0,               // 189 -> no SDL keycode at index 189
    0,               // 190 -> no SDL keycode at index 190
    0,               // 191 -> no SDL keycode at index 191
    0,               // 192 -> no SDL keycode at index 192
    0,               // 193 -> no SDL keycode at index 193
    0,               // 194 -> no SDL keycode at index 194
    0,               // 195 -> no SDL keycode at index 195
    0,               // 196 -> no SDL keycode at index 196
    0,               // 197 -> no SDL keycode at index 197
    0,               // 198 -> no SDL keycode at index 198
    0,               // 199 -> no SDL keycode at index 199
    0,               // 200 -> no SDL keycode at index 200
    0,               // 201 -> no SDL keycode at index 201
    0,               // 202 -> no SDL keycode at index 202
    0,               // 203 -> no SDL keycode at index 203
    0,               // 204 -> no SDL keycode at index 204
    0,               // 205 -> no SDL keycode at index 205
    0,               // 206 -> no SDL keycode at index 206
    0,               // 207 -> no SDL keycode at index 207
    0,               // 208 -> no SDL keycode at index 208
    0,               // 209 -> no SDL keycode at index 209
    0,               // 210 -> no SDL keycode at index 210
    0,               // 211 -> no SDL keycode at index 212
    0,               // 213 -> no SDL keycode at index 213
    0,               // 214 -> no SDL keycode at index 214
    0,               // 215 -> no SDL keycode at index 215
    0,               // 216 -> no SDL keycode at index 216
    0,               // 217 -> no SDL keycode at index 217
    0,               // 218 -> no SDL keycode at index 218
    0,               // 219 -> no SDL keycode at index 219
    0,               // 220 -> no SDL keycode at index 220
    0,               // 221 -> no SDL keycode at index 221
    0,               // 222 -> no SDL keycode at index 222
    0,               // 223 -> no SDL keycode at index 223
    0,
    KEY_LEFTCTRL,      // 224 -> Left Ctrl
    KEY_LEFTSHIFT,     // 225 -> Left Shift
    KEY_LEFTALT,       // 226 -> Left Alt
    KEY_LEFTMETA,      // 227 -> Left GUI (Windows Key)
    KEY_RIGHTCTRL,     // 228 -> Right Ctrl
    KEY_RIGHTSHIFT,    // 229 -> Right Shift
    KEY_RIGHTALT,      // 230 -> Right Alt
    KEY_RIGHTMETA,     // 231 -> Right GUI (Windows Key)
    0,                 // 232 -> no SDL keycode at index 232
    0,                 // 233 -> no SDL keycode at index 233
    0,                 // 234 -> no SDL keycode at index 234
    0,                 // 235 -> no SDL keycode at index 235
    0,                 // 236 -> no SDL keycode at index 236
    0,                 // 237 -> no SDL keycode at index 237
    0,                 // 238 -> no SDL keycode at index 238
    0,                 // 239 -> no SDL keycode at index 239
    0,                 // 240 -> no SDL keycode at index 240
    0,                 // 241 -> no SDL keycode at index 241
    0,                 // 242 -> no SDL keycode at index 242
    0,                 // 243 -> no SDL keycode at index 243
    0,                 // 244 -> no SDL keycode at index 244
    0,                 // 245 -> no SDL keycode at index 245
    0,                 // 246 -> no SDL keycode at index 246
    0,                 // 247 -> no SDL keycode at index 247
    0,                 // 248 -> no SDL keycode at index 248
    0,                 // 249 -> no SDL keycode at index 249
    0,                 // 250 -> no SDL keycode at index 250
    0,                 // 251 -> no SDL keycode at index 251
    0,                 // 252 -> no SDL keycode at index 252
    0,                 // 253 -> no SDL keycode at index 253
    0,                 // 254 -> no SDL keycode at index 254
    0,                 // 255 -> no SDL keycode at index 255
    0,                 // 256 -> no SDL keycode at index 256
    KEY_MODE,          // 257 -> ModeSwitch
    KEY_NEXTSONG,      // 258 -> Audio/Media Next
    KEY_PREVIOUSSONG,  // 259 -> Audio/Media Prev
    KEY_STOPCD,        // 260 -> Audio/Media Stop
    KEY_PLAYPAUSE,     // 261 -> Audio/Media Play
    KEY_MUTE,          // 262 -> Audio/Media Mute
    KEY_SELECT         // 263 -> Media Select
};

const int linux_mouse_buttons[6] = {
    0,           // 0 -> no FractalMouseButton
    BTN_LEFT,    // 1 -> Left Button
    BTN_MIDDLE,  // 2 -> Middle Button
    BTN_RIGHT,   // 3 -> Right Button
    BTN_3,       // 4 -> Extra Mouse Button 1
    BTN_4        // 5 -> Extra Mouse Button 2
};

// typedef struct FractalCursorTypes {
//     HCURSOR CursorAppStarting;
//     HCURSOR CursorArrow;
//     HCURSOR CursorCross;
//     HCURSOR CursorHand;
//     HCURSOR CursorHelp;
//     HCURSOR CursorIBeam;
//     HCURSOR CursorIcon;
//     HCURSOR CursorNo;
//     HCURSOR CursorSize;
//     HCURSOR CursorSizeAll;
//     HCURSOR CursorSizeNESW;
//     HCURSOR CursorSizeNS;
//     HCURSOR CursorSizeNWSE;
//     HCURSOR CursorSizeWE;
//     HCURSOR CursorUpArrow;
//     HCURSOR CursorWait;
// } FractalCursorTypes;

// struct FractalCursorTypes l_types = {0};
// struct FractalCursorTypes* types = &l_types;

// FractalCursorImage GetCursorImage(PCURSORINFO pci);

void LoadCursors();

// void initCursors() { LoadCursors(); }

int GetLinuxKeyCode(int sdl_keycode) { return linux_keycodes[sdl_keycode]; }

int GetLinuxMouseButton(FractalMouseButton fractal_code) {
    return linux_mouse_buttons[fractal_code];
}

// void SendKeyInput(int x11_keysym, int pressed) {
//     Window focusWindow;
//     int revert;
//     XGetInputFocus(disp, &focusWindow, &revert);
//     KeyCode kcode = XKeysymToKeycode(disp, x11_keysym);
//     int res = XTestFakeKeyEvent(disp, kcode, pressed, 0);
// }

// void KeyDown(int x11_keysym) { SendKeyInput(x11_keysym, 1); }

// void KeyUp(int x11_keysym) { SendKeyInput(x11_keysym, 0); }

// void updateKeyboardState(struct FractalClientMessage* fmsg) {
//     if (fmsg->type != MESSAGE_KEYBOARD_STATE) {
//         mprintf(
//             "updateKeyboardState requires fmsg.type to be "
//             "MESSAGE_KEYBOARD_STATE\n");
//         return;
//     }

//     // Setup base input data
//     INPUT ip;
//     ip.type = INPUT_KEYBOARD;
//     ip.ki.wVk = 0;
//     ip.ki.time = 0;
//     ip.ki.dwExtraInfo = 0;

//     bool server_caps_lock = GetKeyState(VK_CAPITAL) & 1;
//     bool server_num_lock = GetKeyState(VK_NUMLOCK) & 1;

//     bool caps_lock_holding = false;
//     bool num_lock_holding = false;

//     int keypress_mask = 1 << 15;

//     // Depress all keys that are currently pressed but should not be pressed
//     for (int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes;
//     sdl_keycode++) {
//         int windows_keycode = GetWindowsKeyCode(sdl_keycode);

//         if (!windows_keycode) continue;

//         // If I should key up, then key up
//         if (!fmsg->keyboard_state[sdl_keycode] &&
//             (GetAsyncKeyState(windows_keycode) & keypress_mask)) {
//             KeyUp(windows_keycode);
//         }
//     }

//     // Press all keys that are not currently pressed but should be pressed
//     for (int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes;
//     sdl_keycode++) {
//         int windows_keycode = GetWindowsKeyCode(sdl_keycode);

//         if (!windows_keycode) continue;

//         // Keep track of keyboard state for caps lock and num lock

//         if (windows_keycode == VK_CAPITAL) {
//             caps_lock_holding = fmsg->keyboard_state[sdl_keycode];
//         }

//         if (windows_keycode == VK_NUMLOCK) {
//             num_lock_holding = fmsg->keyboard_state[sdl_keycode];
//         }

//         // If I should key down, then key down
//         if (fmsg->keyboard_state[sdl_keycode] &&
//             !(GetAsyncKeyState(windows_keycode) & keypress_mask)) {
//             KeyDown(windows_keycode);

//             // In the process of swapping a toggle key
//             if (windows_keycode == VK_CAPITAL) {
//                 server_caps_lock = !server_caps_lock;
//             }
//             if (windows_keycode == VK_NUMLOCK) {
//                 server_num_lock = !server_num_lock;
//             }
//         }
//     }

//     // If caps lock doesn't match, then send a correction
//     if (!!server_caps_lock != !!fmsg->caps_lock) {
//         mprintf("Caps lock out of sync, updating! From %s to %s\n",
//                 server_caps_lock ? "caps" : "no caps",
//                 fmsg->caps_lock ? "caps" : "no caps");
//         // If I'm supposed to be holding it down, then just release and then
//         // repress
//         if (caps_lock_holding) {
//             KeyUp(VK_CAPITAL);
//             KeyDown(VK_CAPITAL);
//         } else {
//             // Otherwise, just press and let go like a normal key press
//             KeyDown(VK_CAPITAL);
//             KeyUp(VK_CAPITAL);
//         }
//     }

//     // If num lock doesn't match, then send a correction
//     if (!!server_num_lock != !!fmsg->num_lock) {
//         mprintf("Num lock out of sync, updating! From %s to %s\n",
//                 server_num_lock ? "num lock" : "no num lock",
//                 fmsg->num_lock ? "num lock" : "no num lock");
//         // If I'm supposed to be holding it down, then just release and then
//         // repress
//         if (num_lock_holding) {
//             KeyUp(VK_NUMLOCK);
//             KeyDown(VK_NUMLOCK);
//         } else {
//             // Otherwise, just press and let go like a normal key press
//             KeyDown(VK_NUMLOCK);
//             KeyUp(VK_NUMLOCK);
//         }
//     }
// }

input_device* CreateInputDevice(input_device* device) {
    device->fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (device->fd < 0) {
        mprintf(
            "CreateInputDevice: Error opening '/dev/uinput' for writing: %s\n",
            strerror(errno));
        return NULL;
    }

    // register keycode and mouse button events
    ioctl(device->fd, UI_SET_EVBIT, EV_KEY);
    int kcode;
    for (int i = 0; i < NUM_KEYCODES; ++i) {
        if (kcode = GetLinuxKeyCode(i)) {
            ioctl(device->fd, UI_SET_KEYBIT, kcode);
        }
    }

    // register mouse button events
    ioctl(device->fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(device->fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(device->fd, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(device->fd, UI_SET_KEYBIT, BTN_3);
    ioctl(device->fd, UI_SET_KEYBIT, BTN_4);

    // register relative mouse motion events
    ioctl(device->fd, UI_SET_EVBIT, EV_REL);
    ioctl(device->fd, UI_SET_RELBIT, REL_X);
    ioctl(device->fd, UI_SET_RELBIT, REL_Y);
    ioctl(device->fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(device->fd, UI_SET_RELBIT, REL_HWHEEL);

    // register absolute mouse motion events
    ioctl(device->fd, UI_SET_EVBIT, EV_ABS);
    ioctl(device->fd, UI_SET_ABSBIT, ABS_X);
    ioctl(device->fd, UI_SET_ABSBIT, ABS_Y);

    // register sync event
    ioctl(device->fd, UI_SET_EVBIT, EV_SYN);

    device->usetup = (struct uinput_setup*)malloc(sizeof(struct uinput_setup));
    memset(device->usetup, 0, sizeof(struct uinput_setup));
    device->usetup->id.bustype = BUS_USB;
    device->usetup->id.vendor = 0xf4c1;
    device->usetup->id.product = 0x1124;
    strcpy(device->usetup->name, "Fractal Input Controller");

    ioctl(device->fd, UI_DEV_SETUP, device->usetup);
    ioctl(device->fd, UI_DEV_CREATE);

    return device;
}

void DestroyInputDevice(input_device* device) {
    if (!device) {
        mprintf("DestroyInputDevice: Nothing to do, device is null!\n");
        return;
    }
    ioctl(device->fd, UI_DEV_DESTROY);
    close(device->fd);
    free(device->usetup);
    free(device);
    device = NULL;
}

void EmitInputEvent(input_device* device, int type, int code, int val) {
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;

    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    int res = write(device->fd, &ie, sizeof(ie));
    mprintf("wrote %d of %d bytes to '/dev/uinput', type %d code %d val %d\n",
            res, sizeof(ie), type, code, val);
}

/// @brief replays a user action taken on the client and sent to the server
/// @details parses the FractalClientMessage struct and send input to Windows OS
void ReplayUserInput(input_device* device, struct FractalClientMessage* fmsg) {
    char name[20];
    ioctl(device->fd, UI_GET_SYSNAME(20), name);
    mprintf("Received input request for device %s!\n", name);

    // switch to fill in the event depending on the FractalClientMessage
    // type
    switch (fmsg->type) {
        case MESSAGE_KEYBOARD:
            // event for keyboard action
            mprintf("KEYBOARD: code %d\n",
                    GetLinuxKeyCode(fmsg->keyboard.code));
            EmitInputEvent(device, EV_KEY, GetLinuxKeyCode(fmsg->keyboard.code),
                           fmsg->keyboard.pressed);
            break;
        case MESSAGE_MOUSE_MOTION:
            // mouse motion event
            ("MOUSE: x %d y %d r %d\n", fmsg->mouseMotion.x,
             fmsg->mouseMotion.y, fmsg->mouseMotion.relative);
            if (fmsg->mouseMotion.relative) {
                mprintf("RELATIVE MOUSE\n");
                EmitInputEvent(device, EV_REL, REL_X, fmsg->mouseMotion.x);
                EmitInputEvent(device, EV_REL, REL_Y, fmsg->mouseMotion.y);
            } else {
                mprintf("ABSOLUTE MOUSE\n");
                EmitInputEvent(device, EV_ABS, ABS_X,
                               (int)(fmsg->mouseMotion.x *
                                     (int32_t)get_native_screen_width() /
                                     (int32_t)1000000));
                EmitInputEvent(device, EV_ABS, ABS_Y,
                               (int)(fmsg->mouseMotion.y *
                                     (int32_t)get_native_screen_height() /
                                     (int32_t)1000000));
            }
            break;
        case MESSAGE_MOUSE_BUTTON:
            // mouse button event
            mprintf("MOUSE BUTTON: %x\n",
                    GetLinuxMouseButton(fmsg->mouseButton.button));
            EmitInputEvent(device, EV_KEY,
                           GetLinuxMouseButton(fmsg->mouseButton.button),
                           fmsg->mouseButton.pressed);
            break;  // outer switch
        case MESSAGE_MOUSE_WHEEL:
            // mouse wheel event
            EmitInputEvent(device, EV_REL, REL_HWHEEL, fmsg->mouseWheel.x);
            EmitInputEvent(device, EV_REL, REL_WHEEL, fmsg->mouseWheel.y);
            break;
            // TODO: add clipboard
    }

    // sync the input event
    EmitInputEvent(device, EV_SYN, SYN_REPORT, 0);
}

// void EnterWinString(enum FractalKeycode* keycodes, int len) {
//     // get screen width and height for mouse cursor
//     int i, index = 0;
//     enum FractalKeycode keycode;
//     INPUT Event[200];

//     for (i = 0; i < len; i++) {
//         keycode = keycodes[i];
//         Event[index].ki.wVk = (WORD)windows_keycodes[keycode];
//         Event[index].type = INPUT_KEYBOARD;
//         Event[index].ki.wScan = 0;
//         Event[index].ki.time = 0;  // system supplies timestamp
//         Event[index].ki.dwFlags = 0;

//         index++;

//         Event[index].ki.wVk = (WORD)windows_keycodes[keycode];
//         Event[index].type = INPUT_KEYBOARD;
//         Event[index].ki.wScan = 0;
//         Event[index].ki.time = 0;  // system supplies timestamp
//         Event[index].ki.dwFlags = KEYEVENTF_KEYUP;

//         index++;
//     }

//     // send FMSG mapped to Windows event to Windows and return
//     SendInput(index, Event, sizeof(INPUT));
// }

// void LoadCursors() {
//     types->CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
//     types->CursorArrow = LoadCursor(NULL, IDC_ARROW);
//     types->CursorCross = LoadCursor(NULL, IDC_CROSS);
//     types->CursorHand = LoadCursor(NULL, IDC_HAND);
//     types->CursorHelp = LoadCursor(NULL, IDC_HELP);
//     types->CursorIBeam = LoadCursor(NULL, IDC_IBEAM);
//     types->CursorIcon = LoadCursor(NULL, IDC_ICON);
//     types->CursorNo = LoadCursor(NULL, IDC_NO);
//     types->CursorSize = LoadCursor(NULL, IDC_SIZE);
//     types->CursorSizeAll = LoadCursor(NULL, IDC_SIZEALL);
//     types->CursorSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
//     types->CursorSizeNS = LoadCursor(NULL, IDC_SIZENS);
//     types->CursorSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);
//     types->CursorSizeWE = LoadCursor(NULL, IDC_SIZEWE);
//     types->CursorUpArrow = LoadCursor(NULL, IDC_UPARROW);
//     types->CursorWait = LoadCursor(NULL, IDC_WAIT);
// }

// FractalCursorImage GetCursorImage(PCURSORINFO pci) {
//     HCURSOR cursor = pci->hCursor;
//     FractalCursorImage image = {0};

//     if (cursor == types->CursorArrow) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
//     } else if (cursor == types->CursorCross) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_CROSSHAIR;
//     } else if (cursor == types->CursorHand) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_HAND;
//     } else if (cursor == types->CursorIBeam) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_IBEAM;
//     } else if (cursor == types->CursorNo) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_NO;
//     } else if (cursor == types->CursorSizeAll) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZEALL;
//     } else if (cursor == types->CursorSizeNESW) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZENESW;
//     } else if (cursor == types->CursorSizeNS) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZENS;
//     } else if (cursor == types->CursorSizeNWSE) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZENWSE;
//     } else if (cursor == types->CursorSizeWE) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZEWE;
//     } else if (cursor == types->CursorWait) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_WAITARROW;
//     } else {
//         image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
//     }

//     return image;
// }

// FractalCursorImage GetCurrentCursor() {
//     CURSORINFO pci;
//     pci.cbSize = sizeof(CURSORINFO);
//     GetCursorInfo(&pci);

//     FractalCursorImage image = {0};
//     image = GetCursorImage(&pci);

//     image.cursor_state = pci.flags;

//     return image;
// }