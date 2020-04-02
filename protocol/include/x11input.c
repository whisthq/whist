#include "input.h"  // header file for this file
#define XK_LATIN1
#define XK_MISCELLANY
#define XK_3270
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysymdef.h>

// @brief Linux keycodes for replaying SDL user inputs on server
// @details index is SDL keycode, value is Linux keycode
const int x11_keysyms[NUM_KEYCODES] = {
    0,                        // SDL keycodes start at index 4
    0,                        // SDL keycodes start at index 4
    0,                        // SDL keycodes start at index 4
    0,                        // SDL keycodes start at index 4
    XK_A,                     // 4 -> A
    XK_B,                     // 5 -> B
    XK_C,                     // 6 -> C
    XK_D,                     // 7 -> D
    XK_E,                     // 8 -> E
    XK_F,                     // 9 -> F
    XK_G,                     // 10 -> G
    XK_H,                     // 11 -> H
    XK_I,                     // 12 -> I
    XK_J,                     // 13 -> J
    XK_K,                     // 14 -> K
    XK_L,                     // 15 -> L
    XK_M,                     // 16 -> M
    XK_N,                     // 17 -> N
    XK_O,                     // 18 -> O
    XK_P,                     // 19 -> P
    XK_Q,                     // 20 -> Q
    XK_R,                     // 21 -> R
    XK_S,                     // 22 -> S
    XK_T,                     // 23 -> T
    XK_U,                     // 24 -> U
    XK_V,                     // 25 -> V
    XK_W,                     // 26 -> W
    XK_X,                     // 27 -> X
    XK_Y,                     // 28 -> Y
    XK_Z,                     // 29 -> Z
    XK_1,                     // 30 -> 1
    XK_2,                     // 31 -> 2
    XK_3,                     // 32 -> 3
    XK_4,                     // 33 -> 4
    XK_5,                     // 34 -> 5
    XK_6,                     // 35 -> 6
    XK_7,                     // 36 -> 7
    XK_8,                     // 37 -> 8
    XK_9,                     // 38 -> 9
    XK_0,                     // 39 -> 0
    XK_Return,                // 40 -> Enter
    XK_Escape,                // 41 -> Escape
    XK_BackSpace,             // 42 -> Backspace
    XK_Tab,                   // 43 -> Tab
    XK_space,                 // 44 -> Space
    XK_minus,                 // 45 -> Minus
    XK_equal,                 // 46 -> Equal
    XK_bracketleft,           // 47 -> Left Bracket
    XK_bracketright,          // 48 -> Right Bracket
    XK_backslash,             // 49 -> Backslash
    0,                        // 50 -> no SDL keycode at index 50
    XK_semicolon,             // 51 -> Semicolon
    XK_apostrophe,            // 52 -> Apostrophe
    XK_grave,                 // 53 -> Backtick
    XK_comma,                 // 54 -> Comma
    XK_period,                // 55 -> Period
    XK_slash,                 // 56 -> Forward Slash
    XK_Caps_Lock,             // 57 -> Capslock
    XK_F1,                    // 58 -> F1
    XK_F2,                    // 59 -> F2
    XK_F3,                    // 60 -> F3
    XK_F4,                    // 61 -> F4
    XK_F5,                    // 62 -> F5
    XK_F6,                    // 63 -> F6
    XK_F7,                    // 64 -> F7
    XK_F8,                    // 65 -> F8
    XK_F9,                    // 66 -> F9
    XK_F10,                   // 67 -> F10
    XK_F11,                   // 68 -> F11
    XK_F12,                   // 69 -> F12
    XK_3270_PrintScreen,      // 70 -> Print Screen
    XK_Scroll_Lock,           // 71 -> Scroll Lock
    XK_Pause,                 // 72 -> Pause
    XK_Insert,                // 73 -> Insert
    XK_Home,                  // 74 -> Home
    XK_Page_Up,               // 75 -> Pageup
    XK_Delete,                // 76 -> Delete
    XK_End,                   // 77 -> End
    XK_Page_Down,             // 78 -> Pagedown
    XK_Right,                 // 79 -> Right
    XK_Left,                  // 80 -> Left
    XK_Down,                  // 81 -> Down
    XK_Up,                    // 82 -> Up
    XK_Num_Lock,              // 83 -> Numlock
    XK_KP_Divide,             // 84 -> Numeric Keypad Divide
    XK_KP_Multiply,           // 85 -> Numeric Keypad Multiply
    XK_KP_Subtract,           // 86 -> Numeric Keypad Minus
    XK_KP_Add,                // 87 -> Numeric Keypad Plus
    XK_KP_Enter,              // 88 -> Numeric Keypad Enter
    XK_KP_1,                  // 89 -> Numeric Keypad 1
    XK_KP_2,                  // 90 -> Numeric Keypad 2
    XK_KP_3,                  // 91 -> Numeric Keypad 3
    XK_KP_4,                  // 92 -> Numeric Keypad 4
    XK_KP_5,                  // 93 -> Numeric Keypad 5
    XK_KP_6,                  // 94 -> Numeric Keypad 6
    XK_KP_7,                  // 95 -> Numeric Keypad 7
    XK_KP_8,                  // 96 -> Numeric Keypad 8
    XK_KP_9,                  // 97 -> Numeric Keypad 9
    XK_KP_0,                  // 98 -> Numeric Keypad 0
    XK_KP_Decimal,            // 99 -> Numeric Keypad Period
    0,                        // 100 -> no SDL keycode at index 100
    XK_Menu,                  // 101 -> Application
    0,                        // 102 -> no SDL keycode at index 102
    0,                        // 103 -> no SDL keycode at index 103
    XK_F13,                   // 104 -> F13
    XK_F14,                   // 105 -> F14
    XK_F15,                   // 106 -> F15
    XK_F16,                   // 107 -> F16
    XK_F17,                   // 108 -> F17
    XK_F18,                   // 109 -> F18
    XK_F19,                   // 110 -> F19
    XK_F20,                   // 111 -> F20
    XK_F21,                   // 112 -> F21
    XK_F22,                   // 113 -> F22
    XK_F23,                   // 114 -> F23
    XK_F24,                   // 115 -> F24
    XK_Execute,               // 116 -> Execute
    XK_Help,                  // 117 -> Help
    XK_Menu,                  // 118 -> Menu
    XK_Select,                // 119 -> Select
    0,                        // 120 -> no SDL keycode at index 120
    0,                        // 121 -> no SDL keycode at index 121
    0,                        // 122 -> no SDL keycode at index 122
    0,                        // 123 -> no SDL keycode at index 123
    0,                        // 124 -> no SDL keycode at index 124
    0,                        // 125 -> no SDL keycode at index 125
    0,                        // 126 -> no SDL keycode at index 126
    XF86XK_AudioMute,         // 127 -> Mute
    XF86XK_AudioRaiseVolume,  // 128 -> Volume Up
    XF86XK_AudioLowerVolume,  // 129 -> Volume Down
    0,                        // 130 -> no SDL keycode at index 130
    0,                        // 131 -> no SDL keycode at index 131
    0,                        // 132 -> no SDL keycode at index 132
    0,                        // 133 -> no SDL keycode at index 133
    0,                        // 134 -> no SDL keycode at index 134
    0,                        // 135 -> no SDL keycode at index 135
    0,                        // 136 -> no SDL keycode at index 136
    0,                        // 137 -> no SDL keycode at index 137
    0,                        // 138 -> no SDL keycode at index 138
    0,                        // 139 -> no SDL keycode at index 139
    0,                        // 140 -> no SDL keycode at index 140
    0,                        // 141 -> no SDL keycode at index 141
    0,                        // 142 -> no SDL keycode at index 142
    0,                        // 143 -> no SDL keycode at index 143
    0,                        // 144 -> no SDL keycode at index 144
    0,                        // 145 -> no SDL keycode at index 145
    0,                        // 146 -> no SDL keycode at index 146
    0,                        // 147 -> no SDL keycode at index 147
    0,                        // 148 -> no SDL keycode at index 148
    0,                        // 149 -> no SDL keycode at index 149
    0,                        // 150 -> no SDL keycode at index 150
    0,                        // 151 -> no SDL keycode at index 151
    0,                        // 152 -> no SDL keycode at index 152
    0,                        // 153 -> no SDL keycode at index 153
    0,                        // 154 -> no SDL keycode at index 154
    0,                        // 155 -> no SDL keycode at index 155
    0,                        // 156 -> no SDL keycode at index 156
    0,                        // 157 -> no SDL keycode at index 157
    0,                        // 158 -> no SDL keycode at index 158
    0,                        // 159 -> no SDL keycode at index 159
    0,                        // 160 -> no SDL keycode at index 160
    0,                        // 161 -> no SDL keycode at index 161
    0,                        // 162 -> no SDL keycode at index 162
    0,                        // 163 -> no SDL keycode at index 163
    0,                        // 164 -> no SDL keycode at index 164
    0,                        // 165 -> no SDL keycode at index 165
    0,                        // 166 -> no SDL keycode at index 166
    0,                        // 167 -> no SDL keycode at index 167
    0,                        // 168 -> no SDL keycode at index 168
    0,                        // 169 -> no SDL keycode at index 169
    0,                        // 170 -> no SDL keycode at index 170
    0,                        // 171 -> no SDL keycode at index 171
    0,                        // 172 -> no SDL keycode at index 172
    0,                        // 173 -> no SDL keycode at index 173
    0,                        // 174 -> no SDL keycode at index 174
    0,                        // 175 -> no SDL keycode at index 175
    0,                        // 176 -> no SDL keycode at index 176
    0,                        // 177 -> no SDL keycode at index 177
    0,                        // 178 -> no SDL keycode at index 178
    0,                        // 179 -> no SDL keycode at index 179
    0,                        // 180 -> no SDL keycode at index 180
    0,                        // 181 -> no SDL keycode at index 181
    0,                        // 182 -> no SDL keycode at index 182
    0,                        // 183 -> no SDL keycode at index 183
    0,                        // 184 -> no SDL keycode at index 184
    0,                        // 185 -> no SDL keycode at index 185
    0,                        // 186 -> no SDL keycode at index 186
    0,                        // 187 -> no SDL keycode at index 187
    0,                        // 188 -> no SDL keycode at index 188
    0,                        // 189 -> no SDL keycode at index 189
    0,                        // 190 -> no SDL keycode at index 190
    0,                        // 191 -> no SDL keycode at index 191
    0,                        // 192 -> no SDL keycode at index 192
    0,                        // 193 -> no SDL keycode at index 193
    0,                        // 194 -> no SDL keycode at index 194
    0,                        // 195 -> no SDL keycode at index 195
    0,                        // 196 -> no SDL keycode at index 196
    0,                        // 197 -> no SDL keycode at index 197
    0,                        // 198 -> no SDL keycode at index 198
    0,                        // 199 -> no SDL keycode at index 199
    0,                        // 200 -> no SDL keycode at index 200
    0,                        // 201 -> no SDL keycode at index 201
    0,                        // 202 -> no SDL keycode at index 202
    0,                        // 203 -> no SDL keycode at index 203
    0,                        // 204 -> no SDL keycode at index 204
    0,                        // 205 -> no SDL keycode at index 205
    0,                        // 206 -> no SDL keycode at index 206
    0,                        // 207 -> no SDL keycode at index 207
    0,                        // 208 -> no SDL keycode at index 208
    0,                        // 209 -> no SDL keycode at index 209
    0,                        // 210 -> no SDL keycode at index 210
    0,                        // 211 -> no SDL keycode at index 212
    0,                        // 213 -> no SDL keycode at index 213
    0,                        // 214 -> no SDL keycode at index 214
    0,                        // 215 -> no SDL keycode at index 215
    0,                        // 216 -> no SDL keycode at index 216
    0,                        // 217 -> no SDL keycode at index 217
    0,                        // 218 -> no SDL keycode at index 218
    0,                        // 219 -> no SDL keycode at index 219
    0,                        // 220 -> no SDL keycode at index 220
    0,                        // 221 -> no SDL keycode at index 221
    0,                        // 222 -> no SDL keycode at index 222
    0,                        // 223 -> no SDL keycode at index 223
    0,
    XK_Control_L,       // 224 -> Left Ctrl
    XK_Shift_L,         // 225 -> Left Shift
    XK_Alt_L,           // 226 -> Left Alt
    XK_Super_L,         // 227 -> Left GUI (Windows Key)
    XK_Control_R,       // 228 -> Right Ctrl
    XK_Shift_R,         // 229 -> Right Shift
    XK_Alt_R,           // 230 -> Right Alt
    XK_Super_R,         // 231 -> Right GUI (Windows Key)
    0,                  // 232 -> no SDL keycode at index 232
    0,                  // 233 -> no SDL keycode at index 233
    0,                  // 234 -> no SDL keycode at index 234
    0,                  // 235 -> no SDL keycode at index 235
    0,                  // 236 -> no SDL keycode at index 236
    0,                  // 237 -> no SDL keycode at index 237
    0,                  // 238 -> no SDL keycode at index 238
    0,                  // 239 -> no SDL keycode at index 239
    0,                  // 240 -> no SDL keycode at index 240
    0,                  // 241 -> no SDL keycode at index 241
    0,                  // 242 -> no SDL keycode at index 242
    0,                  // 243 -> no SDL keycode at index 243
    0,                  // 244 -> no SDL keycode at index 244
    0,                  // 245 -> no SDL keycode at index 245
    0,                  // 246 -> no SDL keycode at index 246
    0,                  // 247 -> no SDL keycode at index 247
    0,                  // 248 -> no SDL keycode at index 248
    0,                  // 249 -> no SDL keycode at index 249
    0,                  // 250 -> no SDL keycode at index 250
    0,                  // 251 -> no SDL keycode at index 251
    0,                  // 252 -> no SDL keycode at index 252
    0,                  // 253 -> no SDL keycode at index 253
    0,                  // 254 -> no SDL keycode at index 254
    0,                  // 255 -> no SDL keycode at index 255
    0,                  // 256 -> no SDL keycode at index 256
    XK_Mode_switch,     // 257 -> ModeSwitch
    XF86XK_AudioNext,   // 258 -> Audio/Media Next
    XF86XK_AudioPrev,   // 259 -> Audio/Media Prev
    XF86XK_AudioStop,   // 260 -> Audio/Media Stop
    XF86XK_AudioPause,  // 261 -> Audio/Media Play
    XF86XK_AudioMute,   // 262 -> Audio/Media Mute
    XF86XK_AudioMedia   // 263 -> Media Select
};

typedef struct FractalCursorTypes {
    HCURSOR CursorAppStarting;
    HCURSOR CursorArrow;
    HCURSOR CursorCross;
    HCURSOR CursorHand;
    HCURSOR CursorHelp;
    HCURSOR CursorIBeam;
    HCURSOR CursorIcon;
    HCURSOR CursorNo;
    HCURSOR CursorSize;
    HCURSOR CursorSizeAll;
    HCURSOR CursorSizeNESW;
    HCURSOR CursorSizeNS;
    HCURSOR CursorSizeNWSE;
    HCURSOR CursorSizeWE;
    HCURSOR CursorUpArrow;
    HCURSOR CursorWait;
} FractalCursorTypes;

struct FractalCursorTypes l_types = {0};
struct FractalCursorTypes* types = &l_types;

FractalCursorImage GetCursorImage(PCURSORINFO pci);

void LoadCursors();

void initCursors() { LoadCursors(); }

int GetX11KeySym(int sdl_keycode);

int GetX11KeySym(int sdl_keycode) { return x11_keysyms[sdl_keycode]; }

static Display* disp;

void initInputPlayback() {
    if (!disp) {
        disp = XOpenDisplay(NULL);
    }
}

void stopInputPlayback() {
    XCloseDisplay(disp);
    disp = NULL;
}

void SendKeyInput(int x11_keysym, int extraFlags) {
    KeyCode kcode = XKeysymToKeycode(disp, x11_keysym);

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

void updateKeyboardState(struct FractalClientMessage* fmsg) {
    if (fmsg->type != MESSAGE_KEYBOARD_STATE) {
        mprintf(
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
        mprintf("Caps lock out of sync, updating! From %s to %s\n",
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
        mprintf("Num lock out of sync, updating! From %s to %s\n",
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

/// @brief replays a user action taken on the client and sent to the server
/// @details parses the FractalClientMessage struct and send input to Windows OS
void ReplayUserInput(struct FractalClientMessage* fmsg) {
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

            Event.ki.dwFlags = KEYEVENTF_SCANCODE;
            Event.ki.wVk = 0;
            Event.ki.wScan = (WORD)MapVirtualKeyA(
                windows_keycodes[fmsg->keyboard.code], MAPVK_VK_TO_VSC_EX);

            if (Event.ki.wScan >> 8 == 0xE0) {
                Event.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                Event.ki.wScan &= 0xFF;
            }

            if (!fmsg->keyboard.pressed) {
                Event.ki.dwFlags |= KEYEVENTF_KEYUP;
            } else {
                Event.ki.dwFlags |= 0;
            }
            break;
        case MESSAGE_MOUSE_MOTION:
            // mouse motion event
            Event.type = INPUT_MOUSE;
            if (fmsg->mouseMotion.relative) {
                Event.mi.dx = (LONG)(fmsg->mouseMotion.x * 0.9);
                Event.mi.dy = (LONG)(fmsg->mouseMotion.y * 0.9);
                Event.mi.dwFlags = MOUSEEVENTF_MOVE;
            } else {
                Event.mi.dx =
                    (LONG)(fmsg->mouseMotion.x * (double)65536 / 1000000);
                Event.mi.dy =
                    (LONG)(fmsg->mouseMotion.y * (double)65536 / 1000000);
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
            // TODO: add clipboard
    }

    // send FMSG mapped to Windows event to Windows and return
    int num_events_sent =
        SendInput(1, &Event, sizeof(INPUT));  // 1 structure to send

    if (1 != num_events_sent) {
        mprintf("SendInput did not send all events! %d\n", num_events_sent);
    }
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

void LoadCursors() {
    types->CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
    types->CursorArrow = LoadCursor(NULL, IDC_ARROW);
    types->CursorCross = LoadCursor(NULL, IDC_CROSS);
    types->CursorHand = LoadCursor(NULL, IDC_HAND);
    types->CursorHelp = LoadCursor(NULL, IDC_HELP);
    types->CursorIBeam = LoadCursor(NULL, IDC_IBEAM);
    types->CursorIcon = LoadCursor(NULL, IDC_ICON);
    types->CursorNo = LoadCursor(NULL, IDC_NO);
    types->CursorSize = LoadCursor(NULL, IDC_SIZE);
    types->CursorSizeAll = LoadCursor(NULL, IDC_SIZEALL);
    types->CursorSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
    types->CursorSizeNS = LoadCursor(NULL, IDC_SIZENS);
    types->CursorSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);
    types->CursorSizeWE = LoadCursor(NULL, IDC_SIZEWE);
    types->CursorUpArrow = LoadCursor(NULL, IDC_UPARROW);
    types->CursorWait = LoadCursor(NULL, IDC_WAIT);
}

FractalCursorImage GetCursorImage(PCURSORINFO pci) {
    HCURSOR cursor = pci->hCursor;
    FractalCursorImage image = {0};

    if (cursor == types->CursorArrow) {
        image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
    } else if (cursor == types->CursorCross) {
        image.cursor_id = SDL_SYSTEM_CURSOR_CROSSHAIR;
    } else if (cursor == types->CursorHand) {
        image.cursor_id = SDL_SYSTEM_CURSOR_HAND;
    } else if (cursor == types->CursorIBeam) {
        image.cursor_id = SDL_SYSTEM_CURSOR_IBEAM;
    } else if (cursor == types->CursorNo) {
        image.cursor_id = SDL_SYSTEM_CURSOR_NO;
    } else if (cursor == types->CursorSizeAll) {
        image.cursor_id = SDL_SYSTEM_CURSOR_SIZEALL;
    } else if (cursor == types->CursorSizeNESW) {
        image.cursor_id = SDL_SYSTEM_CURSOR_SIZENESW;
    } else if (cursor == types->CursorSizeNS) {
        image.cursor_id = SDL_SYSTEM_CURSOR_SIZENS;
    } else if (cursor == types->CursorSizeNWSE) {
        image.cursor_id = SDL_SYSTEM_CURSOR_SIZENWSE;
    } else if (cursor == types->CursorSizeWE) {
        image.cursor_id = SDL_SYSTEM_CURSOR_SIZEWE;
    } else if (cursor == types->CursorWait) {
        image.cursor_id = SDL_SYSTEM_CURSOR_WAITARROW;
    } else {
        image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
    }

    return image;
}

FractalCursorImage GetCurrentCursor() {
    CURSORINFO pci;
    pci.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&pci);

    FractalCursorImage image = {0};
    image = GetCursorImage(&pci);

    image.cursor_state = pci.flags;

    return image;
}