/*
 * User input processing on Windows.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "windowsinput.h"

#define USE_NUMPAD 0x1000

// @brief Windows keycodes for replaying SDL user inputs on server
// @details index is SDL keycode, value is Windows keycode
const int windows_keycodes[NUM_KEYCODES] = {
    0,                       // SDL keycodes start at index 4
    0,                       // SDL keycodes start at index 4
    0,                       // SDL keycodes start at index 4
    0,                       // SDL keycodes start at index 4
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
    0,                       // 50 -> no SDL keycode at index 50
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
    VK_NEXT,                 // 78 -> Pagedown
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
    0,                       // 100 -> no SDL keycode at index 100
    VK_APPS,                 // 101 -> Application
    0,                       // 102 -> no SDL keycode at index 102
    0,                       // 103 -> no SDL keycode at index 103
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
    0,                       // 120 -> no SDL keycode at index 120
    0,                       // 121 -> no SDL keycode at index 121
    0,                       // 122 -> no SDL keycode at index 122
    0,                       // 123 -> no SDL keycode at index 123
    0,                       // 124 -> no SDL keycode at index 124
    0,                       // 125 -> no SDL keycode at index 125
    0,                       // 126 -> no SDL keycode at index 126
    VK_VOLUME_MUTE,          // 127 -> Mute
    VK_VOLUME_UP,            // 128 -> Volume Up
    VK_VOLUME_DOWN,          // 129 -> Volume Down
    0,                       // 130 -> no SDL keycode at index 130
    0,                       // 131 -> no SDL keycode at index 131
    0,                       // 132 -> no SDL keycode at index 132
    0,                       // 133 -> no SDL keycode at index 133
    0,                       // 134 -> no SDL keycode at index 134
    0,                       // 135 -> no SDL keycode at index 135
    0,                       // 136 -> no SDL keycode at index 136
    0,                       // 137 -> no SDL keycode at index 137
    0,                       // 138 -> no SDL keycode at index 138
    0,                       // 139 -> no SDL keycode at index 139
    0,                       // 140 -> no SDL keycode at index 140
    0,                       // 141 -> no SDL keycode at index 141
    0,                       // 142 -> no SDL keycode at index 142
    0,                       // 143 -> no SDL keycode at index 143
    0,                       // 144 -> no SDL keycode at index 144
    0,                       // 145 -> no SDL keycode at index 145
    0,                       // 146 -> no SDL keycode at index 146
    0,                       // 147 -> no SDL keycode at index 147
    0,                       // 148 -> no SDL keycode at index 148
    0,                       // 149 -> no SDL keycode at index 149
    0,                       // 150 -> no SDL keycode at index 150
    0,                       // 151 -> no SDL keycode at index 151
    0,                       // 152 -> no SDL keycode at index 152
    0,                       // 153 -> no SDL keycode at index 153
    0,                       // 154 -> no SDL keycode at index 154
    0,                       // 155 -> no SDL keycode at index 155
    0,                       // 156 -> no SDL keycode at index 156
    0,                       // 157 -> no SDL keycode at index 157
    0,                       // 158 -> no SDL keycode at index 158
    0,                       // 159 -> no SDL keycode at index 159
    0,                       // 160 -> no SDL keycode at index 160
    0,                       // 161 -> no SDL keycode at index 161
    0,                       // 162 -> no SDL keycode at index 162
    0,                       // 163 -> no SDL keycode at index 163
    0,                       // 164 -> no SDL keycode at index 164
    0,                       // 165 -> no SDL keycode at index 165
    0,                       // 166 -> no SDL keycode at index 166
    0,                       // 167 -> no SDL keycode at index 167
    0,                       // 168 -> no SDL keycode at index 168
    0,                       // 169 -> no SDL keycode at index 169
    0,                       // 170 -> no SDL keycode at index 170
    0,                       // 171 -> no SDL keycode at index 171
    0,                       // 172 -> no SDL keycode at index 172
    0,                       // 173 -> no SDL keycode at index 173
    0,                       // 174 -> no SDL keycode at index 174
    0,                       // 175 -> no SDL keycode at index 175
    0,                       // 176 -> no SDL keycode at index 176
    0,                       // 177 -> no SDL keycode at index 177
    0,                       // 178 -> no SDL keycode at index 178
    0,                       // 179 -> no SDL keycode at index 179
    0,                       // 180 -> no SDL keycode at index 180
    0,                       // 181 -> no SDL keycode at index 181
    0,                       // 182 -> no SDL keycode at index 182
    0,                       // 183 -> no SDL keycode at index 183
    0,                       // 184 -> no SDL keycode at index 184
    0,                       // 185 -> no SDL keycode at index 185
    0,                       // 186 -> no SDL keycode at index 186
    0,                       // 187 -> no SDL keycode at index 187
    0,                       // 188 -> no SDL keycode at index 188
    0,                       // 189 -> no SDL keycode at index 189
    0,                       // 190 -> no SDL keycode at index 190
    0,                       // 191 -> no SDL keycode at index 191
    0,                       // 192 -> no SDL keycode at index 192
    0,                       // 193 -> no SDL keycode at index 193
    0,                       // 194 -> no SDL keycode at index 194
    0,                       // 195 -> no SDL keycode at index 195
    0,                       // 196 -> no SDL keycode at index 196
    0,                       // 197 -> no SDL keycode at index 197
    0,                       // 198 -> no SDL keycode at index 198
    0,                       // 199 -> no SDL keycode at index 199
    0,                       // 200 -> no SDL keycode at index 200
    0,                       // 201 -> no SDL keycode at index 201
    0,                       // 202 -> no SDL keycode at index 202
    0,                       // 203 -> no SDL keycode at index 203
    0,                       // 204 -> no SDL keycode at index 204
    0,                       // 205 -> no SDL keycode at index 205
    0,                       // 206 -> no SDL keycode at index 206
    0,                       // 207 -> no SDL keycode at index 207
    0,                       // 208 -> no SDL keycode at index 208
    0,                       // 209 -> no SDL keycode at index 209
    0,                       // 210 -> no SDL keycode at index 210
    0,                       // 211 -> no SDL keycode at index 212
    0,                       // 213 -> no SDL keycode at index 213
    0,                       // 214 -> no SDL keycode at index 214
    0,                       // 215 -> no SDL keycode at index 215
    0,                       // 216 -> no SDL keycode at index 216
    0,                       // 217 -> no SDL keycode at index 217
    0,                       // 218 -> no SDL keycode at index 218
    0,                       // 219 -> no SDL keycode at index 219
    0,                       // 220 -> no SDL keycode at index 220
    0,                       // 221 -> no SDL keycode at index 221
    0,                       // 222 -> no SDL keycode at index 222
    0,                       // 223 -> no SDL keycode at index 223
    0,
    VK_LCONTROL,            // 224 -> Left Ctrl
    VK_LSHIFT,              // 225 -> Left Shift
    VK_LMENU,               // 226 -> Left Alt
    VK_LWIN,                // 227 -> Left GUI (Windows Key)
    VK_RCONTROL,            // 228 -> Right Ctrl
    VK_RSHIFT,              // 229 -> Right Shift
    VK_RMENU,               // 230 -> Right Alt
    VK_RWIN,                // 231 -> Right GUI (Windows Key)
    0,                      // 232 -> no SDL keycode at index 232
    0,                      // 233 -> no SDL keycode at index 233
    0,                      // 234 -> no SDL keycode at index 234
    0,                      // 235 -> no SDL keycode at index 235
    0,                      // 236 -> no SDL keycode at index 236
    0,                      // 237 -> no SDL keycode at index 237
    0,                      // 238 -> no SDL keycode at index 238
    0,                      // 239 -> no SDL keycode at index 239
    0,                      // 240 -> no SDL keycode at index 240
    0,                      // 241 -> no SDL keycode at index 241
    0,                      // 242 -> no SDL keycode at index 242
    0,                      // 243 -> no SDL keycode at index 243
    0,                      // 244 -> no SDL keycode at index 244
    0,                      // 245 -> no SDL keycode at index 245
    0,                      // 246 -> no SDL keycode at index 246
    0,                      // 247 -> no SDL keycode at index 247
    0,                      // 248 -> no SDL keycode at index 248
    0,                      // 249 -> no SDL keycode at index 249
    0,                      // 250 -> no SDL keycode at index 250
    0,                      // 251 -> no SDL keycode at index 251
    0,                      // 252 -> no SDL keycode at index 252
    0,                      // 253 -> no SDL keycode at index 253
    0,                      // 254 -> no SDL keycode at index 254
    0,                      // 255 -> no SDL keycode at index 255
    0,                      // 256 -> no SDL keycode at index 256
    VK_MODECHANGE,          // 257 -> ModeSwitch
    VK_MEDIA_NEXT_TRACK,    // 258 -> Audio/Media Next
    VK_MEDIA_PREV_TRACK,    // 259 -> Audio/Media Prev
    VK_MEDIA_STOP,          // 260 -> Audio/Media Stop
    VK_MEDIA_PLAY_PAUSE,    // 261 -> Audio/Media Play
    VK_VOLUME_MUTE,         // 262 -> Audio/Media Mute
    VK_LAUNCH_MEDIA_SELECT  // 263 -> Media Select
};

int GetWindowsKeyCode(int sdl_keycode) { return windows_keycodes[sdl_keycode]; }

input_device_t* CreateInputDevice(input_device_t* input_device) {
    input_device = malloc(sizeof(char));
    return input_device;
}

void DestroyInputDevice(input_device_t* input_device) {
    free(input_device);
    return;
}

void SendKeyInput(int windows_keycode, int extraFlags) {
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    ip.ki.wScan =
        (WORD)MapVirtualKeyA(windows_keycode & ~USE_NUMPAD, MAPVK_VK_TO_VSC_EX);
    ip.ki.dwFlags = KEYEVENTF_SCANCODE | extraFlags;
    if (ip.ki.wScan >> 8 == 0xE0 || (windows_keycode & USE_NUMPAD)) {
        ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        ip.ki.wScan &= 0xFF;
    }

    SendInput(1, &ip, sizeof(INPUT));
}

void KeyDown(int windows_keycode) { SendKeyInput(windows_keycode, 0); }

void KeyUp(int windows_keycode) {
    SendKeyInput(windows_keycode, KEYEVENTF_KEYUP);
}

void UpdateKeyboardState(input_device_t* input_device,
                         struct FractalClientMessage* fmsg) {
    input_device;
    if (fmsg->type != MESSAGE_KEYBOARD_STATE) {
        LOG_WARNING(
            "updateKeyboardState requires fmsg.type to be "
            "MESSAGE_KEYBOARD_STATE\n");
        return;
    }

    // Setup base input data
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wVk = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    bool server_caps_lock = GetKeyState(VK_CAPITAL) & 1;
    bool server_num_lock = GetKeyState(VK_NUMLOCK) & 1;

    bool caps_lock_holding = false;
    bool num_lock_holding = false;

    int keypress_mask = 1 << 15;

    // Depress all keys that are currently pressed but should not be pressed
    for (int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes; sdl_keycode++) {
        int windows_keycode = GetWindowsKeyCode(sdl_keycode);

        if (!windows_keycode) continue;

        // If I should key up, then key up
        if (!fmsg->keyboard_state[sdl_keycode] &&
            (GetAsyncKeyState(windows_keycode) & keypress_mask)) {
            KeyUp(windows_keycode);
        }
    }

    // Press all keys that are not currently pressed but should be pressed
    for (int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes; sdl_keycode++) {
        int windows_keycode = GetWindowsKeyCode(sdl_keycode);

        if (!windows_keycode) continue;

        // Keep track of keyboard state for caps lock and num lock

        if (windows_keycode == VK_CAPITAL) {
            caps_lock_holding = fmsg->keyboard_state[sdl_keycode];
        }

        if (windows_keycode == VK_NUMLOCK) {
            num_lock_holding = fmsg->keyboard_state[sdl_keycode];
        }

        // If I should key down, then key down
        if (fmsg->keyboard_state[sdl_keycode] &&
            !(GetAsyncKeyState(windows_keycode) & keypress_mask)) {
            KeyDown(windows_keycode);

            // In the process of swapping a toggle key
            if (windows_keycode == VK_CAPITAL) {
                server_caps_lock = !server_caps_lock;
            }
            if (windows_keycode == VK_NUMLOCK) {
                server_num_lock = !server_num_lock;
            }
        }
    }

    // If caps lock doesn't match, then send a correction
    if (!!server_caps_lock != !!fmsg->caps_lock) {
        LOG_INFO("Caps lock out of sync, updating! From %s to %s\n",
                server_caps_lock ? "caps" : "no caps",
                fmsg->caps_lock ? "caps" : "no caps");
        // If I'm supposed to be holding it down, then just release and then
        // repress
        if (caps_lock_holding) {
            KeyUp(VK_CAPITAL);
            KeyDown(VK_CAPITAL);
        } else {
            // Otherwise, just press and let go like a normal key press
            KeyDown(VK_CAPITAL);
            KeyUp(VK_CAPITAL);
        }
    }

    // If num lock doesn't match, then send a correction
    if (!!server_num_lock != !!fmsg->num_lock) {
        LOG_INFO("Num lock out of sync, updating! From %s to %s\n",
                server_num_lock ? "num lock" : "no num lock",
                fmsg->num_lock ? "num lock" : "no num lock");
        // If I'm supposed to be holding it down, then just release and then
        // repress
        if (num_lock_holding) {
            KeyUp(VK_NUMLOCK);
            KeyDown(VK_NUMLOCK);
        } else {
            // Otherwise, just press and let go like a normal key press
            KeyDown(VK_NUMLOCK);
            KeyUp(VK_NUMLOCK);
        }
    }
}

bool ReplayUserInput(input_device_t* input_device,
                     struct FractalClientMessage* fmsg) {
    input_device;
    // get screen width and height for mouse cursor
    // int sWidth = GetSystemMetrics( SM_CXSCREEN ) - 1; is this still needed?
    // int sHeight = GetSystemMetrics( SM_CYSCREEN ) - 1; ^^
    INPUT Event = {0};

    // switch to fill in the Windows event depending on the FractalClientMessage
    // type
    switch (fmsg->type) {
        case MESSAGE_KEYBOARD:
            // Windows event for keyboard action

            Event.type = INPUT_KEYBOARD;
            Event.ki.time = 0;  // system supplies timestamp

            HKL keyboard_layout = GetKeyboardLayout(0);

            Event.ki.dwFlags = KEYEVENTF_SCANCODE;
            Event.ki.wVk = 0;
            Event.ki.wScan =
                (WORD)MapVirtualKeyExA(windows_keycodes[fmsg->keyboard.code],
                                       MAPVK_VK_TO_VSC_EX, keyboard_layout);

            switch (windows_keycodes[fmsg->keyboard.code]) {
                // List found here:
                // https://docs.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input
                case VK_RMENU:
                case VK_RCONTROL:
                case VK_INSERT:
                case VK_DELETE:
                case VK_HOME:
                case VK_END:
                case VK_PRIOR:
                case VK_NEXT:  // page up and page down
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:  // arrow keys
                case VK_NUMLOCK:
                case VK_PRINT:
                case VK_DIVIDE:  // numpad slash
                case VK_RETURN + USE_NUMPAD:
                    Event.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
            }

            /* Print entire keyboard
            for( int i = 0; i < 256; i++ )
            {
                char keyName[50];
                if( GetKeyNameTextA( i << 16, keyName, sizeof( keyName ) ) != 0
            )
                {
                    mprintf( "Code: %d\n", i );
                    mprintf( "KEY: %s\n", keyName );
                } else
                {
                    mprintf( "Code: %d\n", i );
                    mprintf( "NoKey:\n" );
                }
            }
            for( int i = 0; i < 256; i++ )
            {
                char keyName[50];
                if( GetKeyNameTextA( (i << 16) + (1 << 24), keyName, sizeof(
            keyName ) ) != 0 )
                {
                    mprintf( "Code: %d\n", i + (1 << 24) );
                    mprintf( "KEY: %s\n", keyName );
                } else
                {
                    mprintf( "Code: %d\n", i + (1 << 24) );
                    mprintf( "NoKey:\n" );
                }
            }*/

            if (Event.ki.wScan >> 8 == 0xE0) {
                Event.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                Event.ki.wScan &= 0xFF;
            }

            if (Event.ki.wScan >> 8 == 0xE1) {
                LOG_INFO("Weird Extended\n");
            }

            if (!fmsg->keyboard.pressed) {
                Event.ki.dwFlags |= KEYEVENTF_KEYUP;
            } else {
                Event.ki.dwFlags |= 0;
            }
            break;
        case MESSAGE_MOUSE_MOTION:
            // mouse motion event
            // mprintf("MOUSE: x %d y %d r %d\n", fmsg->mouseMotion.x,
            //        fmsg->mouseMotion.y, fmsg->mouseMotion.relative);

            Event.type = INPUT_MOUSE;
            if (fmsg->mouseMotion.relative) {
                Event.mi.dx = (LONG)(fmsg->mouseMotion.x * 0.9);
                Event.mi.dy = (LONG)(fmsg->mouseMotion.y * 0.9);
                Event.mi.dwFlags = MOUSEEVENTF_MOVE;
            } else {
                Event.mi.dx = (LONG)(fmsg->mouseMotion.x * (double)65536 /
                                     MOUSE_SCALING_FACTOR);
                Event.mi.dy = (LONG)(fmsg->mouseMotion.y * (double)65536 /
                                     MOUSE_SCALING_FACTOR);
                Event.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            }
            break;
        case MESSAGE_MOUSE_BUTTON:
            // mouse button event
            Event.type = INPUT_MOUSE;
            Event.mi.dx = 0;
            Event.mi.dy = 0;

            // Emulating button click
            // switch to parse button type
            switch (fmsg->mouseButton.button) {
                case SDL_BUTTON_LEFT:
                    // left click
                    if (fmsg->mouseButton.pressed) {
                        Event.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    } else {
                        Event.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    }
                    break;
                case SDL_BUTTON_MIDDLE:
                    // middle click
                    if (fmsg->mouseButton.pressed) {
                        Event.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                    } else {
                        Event.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
                    }
                    break;
                case SDL_BUTTON_RIGHT:
                    // right click
                    if (fmsg->mouseButton.pressed) {
                        Event.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                    } else {
                        Event.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                    }
                    break;
            }
            // End emulating button click

            break;  // outer switch
        case MESSAGE_MOUSE_WHEEL:
            // mouse wheel event
            Event.type = INPUT_MOUSE;
            Event.mi.dwFlags = MOUSEEVENTF_WHEEL;
            Event.mi.dx = 0;
            Event.mi.dy = 0;
            Event.mi.mouseData = fmsg->mouseWheel.y * 100;
            break;
    }

    // send FMSG mapped to Windows event to Windows and return
    int num_events_sent =
        SendInput(1, &Event, sizeof(INPUT));  // 1 structure to send

    if (1 != num_events_sent) {
        LOG_WARNING("SendInput did not send all events! %d\n", num_events_sent);
        return false;
    }

    return true;
}

void EnterWinString(enum FractalKeycode* keycodes, int len) {
    // get screen width and height for mouse cursor
    int i, index = 0;
    enum FractalKeycode keycode;
    INPUT Event[200];

    for (i = 0; i < len; i++) {
        keycode = keycodes[i];
        Event[index].ki.wVk = (WORD)windows_keycodes[keycode];
        Event[index].type = INPUT_KEYBOARD;
        Event[index].ki.wScan = 0;
        Event[index].ki.time = 0;  // system supplies timestamp
        Event[index].ki.dwFlags = 0;

        index++;

        Event[index].ki.wVk = (WORD)windows_keycodes[keycode];
        Event[index].type = INPUT_KEYBOARD;
        Event[index].ki.wScan = 0;
        Event[index].ki.time = 0;  // system supplies timestamp
        Event[index].ki.dwFlags = KEYEVENTF_KEYUP;

        index++;
    }

    // send FMSG mapped to Windows event to Windows and return
    SendInput(index, Event, sizeof(INPUT));
}
