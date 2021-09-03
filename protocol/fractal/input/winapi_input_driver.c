/**
 * Copyright Fractal Computers, Inc. 2020
 * @file winapi_input_driver.c
 * @brief This file defines the Windows input device type
============================
Usage
============================

You can create an input device to receive
input (keystrokes, mouse clicks, etc.) via CreateInputDevice.
*/

#include "input_driver.h"

#define USE_NUMPAD 0x1000

// This defines the total range of a mouse coordinate, from 0 to
// MOUSE_COORDINATE_RANGE as left to right or top to bottom
#define WINAPI_MOUSE_COORDINATE_RANGE 0xFFFF

/**
 * @brief Windows keycodes for replaying Fractal user inputs on server
 * @details index is Fractal keycode, value is Windows keycode
 */
const int windows_keycodes[KEYCODE_UPPERBOUND] = {
    0,                       // Fractal keycodes start at index 4
    0,                       // Fractal keycodes start at index 4
    0,                       // Fractal keycodes start at index 4
    0,                       // Fractal keycodes start at index 4
    0x41,                    // 4 -> A
    0x42,                    // 5 -> B
    0x43,                    // 6 -> C
    0x44,                    // 7 -> D
    0x45,                    // 8 -> E
    0x46,                    // 9 -> F
    0x47,                    // 10 -> G
    0x48,                    // 11 -> H
    0x49,                    // 12 -> I
    0x4A,                    // 13 -> J
    0x4B,                    // 14 -> K
    0x4C,                    // 15 -> L
    0x4D,                    // 16 -> M
    0x4E,                    // 17 -> N
    0x4F,                    // 18 -> O
    0x50,                    // 19 -> P
    0x51,                    // 20 -> Q
    0x52,                    // 21 -> R
    0x53,                    // 22 -> S
    0x54,                    // 23 -> T
    0x55,                    // 24 -> U
    0x56,                    // 25 -> V
    0x57,                    // 26 -> W
    0x58,                    // 27 -> X
    0x59,                    // 28 -> Y
    0x5A,                    // 29 -> Z
    0x31,                    // 30 -> 1
    0x32,                    // 31 -> 2
    0x33,                    // 32 -> 3
    0x34,                    // 33 -> 4
    0x35,                    // 34 -> 5
    0x36,                    // 35 -> 6
    0x37,                    // 36 -> 7
    0x38,                    // 37 -> 8
    0x39,                    // 38 -> 9
    0x30,                    // 39 -> 0
    VK_RETURN,               // 40 -> Enter
    0x1B,                    // 41 -> Escape
    VK_BACK,                 // 42 -> Backspace
    0x09,                    // 43 -> Tab
    0x20,                    // 44 -> Space
    0xBD,                    // 45 -> Minus
    0xBB,                    // 46 -> Equal
    0xDB,                    // 47 -> Left Bracket
    0xDD,                    // 48 -> Right Bracket
    0xE2,                    // 49 -> Backslash
    0,                       // 50 -> no Fractal keycode at index 50
    VK_OEM_1,                // 51 -> Semicolon
    VK_OEM_7,                // 52 -> Apostrophe
    VK_OEM_3,                // 53 -> Backtick
    VK_OEM_COMMA,            // 54 -> Comma
    VK_OEM_PERIOD,           // 55 -> Period
    VK_OEM_2,                // 56 -> Forward Slash
    VK_CAPITAL,              // 57 -> Capslock
    VK_F1,                   // 58 -> F1
    VK_F2,                   // 59 -> F2
    VK_F3,                   // 60 -> F3
    VK_F4,                   // 61 -> F4
    VK_F5,                   // 62 -> F5
    VK_F6,                   // 63 -> F6
    VK_F7,                   // 64 -> F7
    VK_F8,                   // 65 -> F8
    VK_F9,                   // 66 -> F9
    VK_F10,                  // 67 -> F10
    VK_F11,                  // 68 -> F11
    VK_F12,                  // 69 -> F12
    VK_SNAPSHOT,             // 70 -> Print Screen
    VK_SCROLL,               // 71 -> Scroll Lock
    VK_PAUSE,                // 72 -> Pause
    VK_INSERT,               // 73 -> Insert
    VK_HOME,                 // 74 -> Home
    VK_PRIOR,                // 75 -> Pageup
    VK_DELETE,               // 76 -> Delete
    VK_END,                  // 77 -> End
    VK_LEFT,                 // 78 -> Pagedown
    VK_RIGHT,                // 79 -> Right
    VK_LEFT,                 // 80 -> Left
    VK_DOWN,                 // 81 -> Down
    VK_UP,                   // 82 -> Up
    VK_NUMLOCK,              // 83 -> Numlock
    VK_DIVIDE,               // 84 -> Numeric Keypad Divide
    VK_MULTIPLY,             // 85 -> Numeric Keypad Multiply
    VK_SUBTRACT,             // 86 -> Numeric Keypad Minus
    VK_ADD,                  // 87 -> Numeric Keypad Plus
    VK_RETURN + USE_NUMPAD,  // 88 -> Numeric Keypad Enter
    0x61,                    // 89 -> Numeric Keypad 1
    0x62,                    // 90 -> Numeric Keypad 2
    0x63,                    // 91 -> Numeric Keypad 3
    0x64,                    // 92 -> Numeric Keypad 4
    0x65,                    // 93 -> Numeric Keypad 5
    0x66,                    // 94 -> Numeric Keypad 6
    0x67,                    // 95 -> Numeric Keypad 7
    0x68,                    // 96 -> Numeric Keypad 8
    0x69,                    // 97 -> Numeric Keypad 9
    0x60,                    // 98 -> Numeric Keypad 0
    VK_DECIMAL,              // 99 -> Numeric Keypad Period
    0,                       // 100 -> no Fractal keycode at index 100
    VK_APPS,                 // 101 -> Application
    0,                       // 102 -> no Fractal keycode at index 102
    0,                       // 103 -> no Fractal keycode at index 103
    VK_F13,                  // 104 -> F13
    VK_F14,                  // 105 -> F14
    VK_F15,                  // 106 -> F15
    VK_F16,                  // 107 -> F16
    VK_F17,                  // 108 -> F17
    VK_F18,                  // 109 -> F18
    VK_F19,                  // 110 -> F19
    VK_F20,                  // 111 -> F20
    VK_F21,                  // 112 -> F21
    VK_F22,                  // 113 -> F22
    VK_F23,                  // 114 -> F23
    VK_F24,                  // 115 -> F24
    VK_EXECUTE,              // 116 -> Execute
    VK_HELP,                 // 117 -> Help
    0,                       // 118 -> Menu
    VK_SELECT,               // 119 -> Select
    0,                       // 120 -> no Fractal keycode at index 120
    0,                       // 121 -> no Fractal keycode at index 121
    0,                       // 122 -> no Fractal keycode at index 122
    0,                       // 123 -> no Fractal keycode at index 123
    0,                       // 124 -> no Fractal keycode at index 124
    0,                       // 125 -> no Fractal keycode at index 125
    0,                       // 126 -> no Fractal keycode at index 126
    VK_VOLUME_MUTE,          // 127 -> Mute
    VK_VOLUME_UP,            // 128 -> Volume Up
    VK_VOLUME_DOWN,          // 129 -> Volume Down
    0,                       // 130 -> no Fractal keycode at index 130
    0,                       // 131 -> no Fractal keycode at index 131
    0,                       // 132 -> no Fractal keycode at index 132
    0,                       // 133 -> no Fractal keycode at index 133
    0,                       // 134 -> no Fractal keycode at index 134
    0,                       // 135 -> no Fractal keycode at index 135
    0,                       // 136 -> no Fractal keycode at index 136
    0,                       // 137 -> no Fractal keycode at index 137
    0,                       // 138 -> no Fractal keycode at index 138
    0,                       // 139 -> no Fractal keycode at index 139
    0,                       // 140 -> no Fractal keycode at index 140
    0,                       // 141 -> no Fractal keycode at index 141
    0,                       // 142 -> no Fractal keycode at index 142
    0,                       // 143 -> no Fractal keycode at index 143
    0,                       // 144 -> no Fractal keycode at index 144
    0,                       // 145 -> no Fractal keycode at index 145
    0,                       // 146 -> no Fractal keycode at index 146
    0,                       // 147 -> no Fractal keycode at index 147
    0,                       // 148 -> no Fractal keycode at index 148
    0,                       // 149 -> no Fractal keycode at index 149
    0,                       // 150 -> no Fractal keycode at index 150
    0,                       // 151 -> no Fractal keycode at index 151
    0,                       // 152 -> no Fractal keycode at index 152
    0,                       // 153 -> no Fractal keycode at index 153
    0,                       // 154 -> no Fractal keycode at index 154
    0,                       // 155 -> no Fractal keycode at index 155
    0,                       // 156 -> no Fractal keycode at index 156
    0,                       // 157 -> no Fractal keycode at index 157
    0,                       // 158 -> no Fractal keycode at index 158
    0,                       // 159 -> no Fractal keycode at index 159
    0,                       // 160 -> no Fractal keycode at index 160
    0,                       // 161 -> no Fractal keycode at index 161
    0,                       // 162 -> no Fractal keycode at index 162
    0,                       // 163 -> no Fractal keycode at index 163
    0,                       // 164 -> no Fractal keycode at index 164
    0,                       // 165 -> no Fractal keycode at index 165
    0,                       // 166 -> no Fractal keycode at index 166
    0,                       // 167 -> no Fractal keycode at index 167
    0,                       // 168 -> no Fractal keycode at index 168
    0,                       // 169 -> no Fractal keycode at index 169
    0,                       // 170 -> no Fractal keycode at index 170
    0,                       // 171 -> no Fractal keycode at index 171
    0,                       // 172 -> no Fractal keycode at index 172
    0,                       // 173 -> no Fractal keycode at index 173
    0,                       // 174 -> no Fractal keycode at index 174
    0,                       // 175 -> no Fractal keycode at index 175
    0,                       // 176 -> no Fractal keycode at index 176
    0,                       // 177 -> no Fractal keycode at index 177
    0,                       // 178 -> no Fractal keycode at index 178
    0,                       // 179 -> no Fractal keycode at index 179
    0,                       // 180 -> no Fractal keycode at index 180
    0,                       // 181 -> no Fractal keycode at index 181
    0,                       // 182 -> no Fractal keycode at index 182
    0,                       // 183 -> no Fractal keycode at index 183
    0,                       // 184 -> no Fractal keycode at index 184
    0,                       // 185 -> no Fractal keycode at index 185
    0,                       // 186 -> no Fractal keycode at index 186
    0,                       // 187 -> no Fractal keycode at index 187
    0,                       // 188 -> no Fractal keycode at index 188
    0,                       // 189 -> no Fractal keycode at index 189
    0,                       // 190 -> no Fractal keycode at index 190
    0,                       // 191 -> no Fractal keycode at index 191
    0,                       // 192 -> no Fractal keycode at index 192
    0,                       // 193 -> no Fractal keycode at index 193
    0,                       // 194 -> no Fractal keycode at index 194
    0,                       // 195 -> no Fractal keycode at index 195
    0,                       // 196 -> no Fractal keycode at index 196
    0,                       // 197 -> no Fractal keycode at index 197
    0,                       // 198 -> no Fractal keycode at index 198
    0,                       // 199 -> no Fractal keycode at index 199
    0,                       // 200 -> no Fractal keycode at index 200
    0,                       // 201 -> no Fractal keycode at index 201
    0,                       // 202 -> no Fractal keycode at index 202
    0,                       // 203 -> no Fractal keycode at index 203
    0,                       // 204 -> no Fractal keycode at index 204
    0,                       // 205 -> no Fractal keycode at index 205
    0,                       // 206 -> no Fractal keycode at index 206
    0,                       // 207 -> no Fractal keycode at index 207
    0,                       // 208 -> no Fractal keycode at index 208
    0,                       // 209 -> no Fractal keycode at index 209
    0,                       // 210 -> no Fractal keycode at index 210
    0,                       // 211 -> no Fractal keycode at index 212
    0,                       // 213 -> no Fractal keycode at index 213
    0,                       // 214 -> no Fractal keycode at index 214
    0,                       // 215 -> no Fractal keycode at index 215
    0,                       // 216 -> no Fractal keycode at index 216
    0,                       // 217 -> no Fractal keycode at index 217
    0,                       // 218 -> no Fractal keycode at index 218
    0,                       // 219 -> no Fractal keycode at index 219
    0,                       // 220 -> no Fractal keycode at index 220
    0,                       // 221 -> no Fractal keycode at index 221
    0,                       // 222 -> no Fractal keycode at index 222
    0,                       // 223 -> no Fractal keycode at index 223
    0,
    VK_LCONTROL,            // 224 -> Left Ctrl
    VK_LSHIFT,              // 225 -> Left Shift
    VK_LMENU,               // 226 -> Left Alt
    VK_LWIN,                // 227 -> Left GUI (Windows Key)
    VK_RCONTROL,            // 228 -> Right Ctrl
    VK_RSHIFT,              // 229 -> Right Shift
    VK_RMENU,               // 230 -> Right Alt
    VK_RWIN,                // 231 -> Right GUI (Windows Key)
    0,                      // 232 -> no Fractal keycode at index 232
    0,                      // 233 -> no Fractal keycode at index 233
    0,                      // 234 -> no Fractal keycode at index 234
    0,                      // 235 -> no Fractal keycode at index 235
    0,                      // 236 -> no Fractal keycode at index 236
    0,                      // 237 -> no Fractal keycode at index 237
    0,                      // 238 -> no Fractal keycode at index 238
    0,                      // 239 -> no Fractal keycode at index 239
    0,                      // 240 -> no Fractal keycode at index 240
    0,                      // 241 -> no Fractal keycode at index 241
    0,                      // 242 -> no Fractal keycode at index 242
    0,                      // 243 -> no Fractal keycode at index 243
    0,                      // 244 -> no Fractal keycode at index 244
    0,                      // 245 -> no Fractal keycode at index 245
    0,                      // 246 -> no Fractal keycode at index 246
    0,                      // 247 -> no Fractal keycode at index 247
    0,                      // 248 -> no Fractal keycode at index 248
    0,                      // 249 -> no Fractal keycode at index 249
    0,                      // 250 -> no Fractal keycode at index 250
    0,                      // 251 -> no Fractal keycode at index 251
    0,                      // 252 -> no Fractal keycode at index 252
    0,                      // 253 -> no Fractal keycode at index 253
    0,                      // 254 -> no Fractal keycode at index 254
    0,                      // 255 -> no Fractal keycode at index 255
    0,                      // 256 -> no Fractal keycode at index 256
    VK_MODECHANGE,          // 257 -> ModeSwitch
    VK_MEDIA_NEXT_TRACK,    // 258 -> Audio/Media Next
    VK_MEDIA_PREV_TRACK,    // 259 -> Audio/Media Prev
    VK_MEDIA_STOP,          // 260 -> Audio/Media Stop
    VK_MEDIA_PLAY_PAUSE,    // 261 -> Audio/Media Play
    VK_VOLUME_MUTE,         // 262 -> Audio/Media Mute
    VK_LAUNCH_MEDIA_SELECT  // 263 -> Media Select
};

InputDevice* create_input_device() {
    InputDevice* input_device = safe_malloc(sizeof(InputDevice));
    memset(input_device, 0, sizeof(InputDevice));
    return input_device;
}

void destroy_input_device(InputDevice* input_device) {
    free(input_device);
    return;
}

#define GetWindowsKeyCode(fractal_keycode) windows_keycodes[fractal_keycode]
#define KEYPRESS_MASK 0x8000

int get_keyboard_modifier_state(InputDevice* input_device, FractalKeycode fractal_keycode) {
    UNUSED(input_device);
    return 1 & GetKeyState(GetWindowsKeyCode(fractal_keycode));
}

int get_keyboard_key_state(InputDevice* input_device, FractalKeycode fractal_keycode) {
    UNUSED(input_device);
    return (KEYPRESS_MASK & GetAsyncKeyState(GetWindowsKeyCode(fractal_keycode))) >> 15;
}

int ignore_key_state(InputDevice* input_device, FractalKeycode fractal_keycode, bool active_pinch) {
    return 0;
}

int emit_key_event(InputDevice* input_device, FractalKeycode fractal_keycode, int pressed) {
    UNUSED(input_device);

    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    HKL keyboard_layout = GetKeyboardLayout(0);

    int windows_keycode = GetWindowsKeyCode(fractal_keycode);

    ip.ki.wScan =
        (WORD)MapVirtualKeyExA(windows_keycode & ~USE_NUMPAD, MAPVK_VK_TO_VSC_EX, keyboard_layout);
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (!pressed) {
        ip.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    if (ip.ki.wScan >> 8 == 0xE0 || (windows_keycode & USE_NUMPAD)) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        ip.ki.wScan &= 0xFF;
    }

    int ret = SendInput(1, &ip, sizeof(INPUT));

    if (ret != 1) {
        LOG_WARNING("Failed to send input event for keycode %d, pressed %d!", fractal_keycode,
                    pressed);
        return -1;
    }

    return 0;
}

int emit_mouse_motion_event(InputDevice* input_device, int32_t x, int32_t y, int relative) {
    UNUSED(input_device);

    INPUT ip;
    ip.type = INPUT_MOUSE;
    if (relative) {
        ip.mi.dx = (LONG)(x * 0.9);
        ip.mi.dy = (LONG)(y * 0.9);
        ip.mi.dwFlags = MOUSEEVENTF_MOVE;
    } else {
        ip.mi.dx = (LONG)(x * (double)WINAPI_MOUSE_COORDINATE_RANGE / MOUSE_SCALING_FACTOR);
        ip.mi.dy = (LONG)(y * (double)WINAPI_MOUSE_COORDINATE_RANGE / MOUSE_SCALING_FACTOR);
        ip.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    }

    int ret = SendInput(1, &ip, sizeof(INPUT));

    if (ret != 1) {
        LOG_WARNING("Failed to send mouse motion event for x %d, y %d, relative %d!", x, y,
                    relative);
        return -1;
    }

    return 0;
}

int emit_mouse_button_event(InputDevice* input_device, FractalMouseButton button, int pressed) {
    UNUSED(input_device);

    INPUT ip;
    ip.type = INPUT_MOUSE;
    ip.mi.dx = 0;
    ip.mi.dy = 0;

    switch (button) {
        case MOUSE_L: {
            // left click
            if (pressed) {
                ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            } else {
                ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            }
            break;
        }
        case MOUSE_MIDDLE: {
            // middle click
            if (pressed) {
                ip.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            } else {
                ip.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            }
            break;
        }
        case MOUSE_R: {
            // right click
            if (pressed) {
                ip.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            } else {
                ip.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            }
            break;
        }
        case MOUSE_X1: {
            // extra button 1 click
            if (pressed) {
                ip.mi.dwFlags = MOUSEEVENTF_XDOWN;
                ip.mi.mouseData = XBUTTON1;
            } else {
                ip.mi.dwFlags = MOUSEEVENTF_XUP;
                ip.mi.mouseData = XBUTTON1;
            }
            break;
        }
        case MOUSE_X2: {
            // extra button 2 click
            if (pressed) {
                ip.mi.dwFlags = MOUSEEVENTF_XDOWN;
                ip.mi.mouseData = XBUTTON2;
            } else {
                ip.mi.dwFlags = MOUSEEVENTF_XUP;
                ip.mi.mouseData = XBUTTON2;
            }
            break;
        }
        default:
            break;
    }

    int ret = SendInput(1, &ip, sizeof(INPUT));

    if (ret != 1) {
        LOG_WARNING("Failed to send mouse button event for button %d, pressed %d!", button,
                    pressed);
        return -1;
    }

    return 0;
}

int emit_low_res_mouse_wheel_event(InputDevice* input_device, int32_t x, int32_t y) {
    UNUSED(input_device);

    INPUT ip[2];  // vertical and horizontal are separate
    ip[0].type = INPUT_MOUSE;
    ip[0].mi.dx = 0;
    ip[0].mi.dy = 0;
    ip[0].mi.mouseData = y * 100;
    ip[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
    ip[1].type = INPUT_MOUSE;
    ip[1].mi.dx = 0;
    ip[1].mi.dy = 0;
    ip[1].mi.mouseData = x * 100;
    ip[1].mi.dwFlags = MOUSEEVENTF_HWHEEL;

    unsigned int ret = SendInput(2, ip, sizeof(INPUT));

    if (ret == 1) {
        LOG_INFO("Didn't send horizontal mouse motion for x %d!", x);
        return 0;
    } else if (ret != 2) {
        LOG_WARNING("Failed to send any mouse wheel event for x %d, y %d!", x, y);
        return -1;
    }

    return 0;
}

int emit_high_res_mouse_wheel_event(InputDevice* input_device, float x, float y) {
    UNUSED(input_device);
    UNUSED(x);
    UNUSED(y);

    LOG_WARNING(
        "High resolution scroll not implemented for Windows servers! "
        "Falling back to low-resolution scroll.");
    return -1;
}

int emit_multigesture_event(InputDevice* input_device, float d_theta, float d_dist,
                            FractalMultigestureType gesture_type, bool active_gesture) {
    UNUSED(input_device);
    UNUSED(d_theta);
    UNUSED(d_dist);
    UNUSED(gesture_type);
    UNUSED(active_gesture);

    LOG_WARNING("Multigesture events not implemented for Windows servers! ");
    return -1;
}

void enter_win_string(enum FractalKeycode* keycodes, int len) {
    // get screen width and height for mouse cursor
    int i, index = 0;
    enum FractalKeycode keycode;
    INPUT event[200];

    for (i = 0; i < len; i++) {
        keycode = keycodes[i];
        event[index].ki.wVk = (WORD)windows_keycodes[keycode];
        event[index].type = INPUT_KEYBOARD;
        event[index].ki.wScan = 0;
        event[index].ki.time = 0;  // system supplies timestamp
        event[index].ki.dwFlags = 0;

        index++;

        event[index].ki.wVk = (WORD)windows_keycodes[keycode];
        event[index].type = INPUT_KEYBOARD;
        event[index].ki.wScan = 0;
        event[index].ki.time = 0;  // system supplies timestamp
        event[index].ki.dwFlags = KEYEVENTF_KEYUP;

        index++;
    }

    // send FMSG mapped to Windows event to Windows and return
    SendInput(index, event, sizeof(INPUT));
}
