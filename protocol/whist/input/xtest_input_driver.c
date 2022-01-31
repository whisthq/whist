#include "input.h"
#include "input_internal.h"

#define XK_LATIN1
#define XK_MISCELLANY
#define XK_3270

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/keysymdef.h>

/*
============================
Custom Types
============================
*/

typedef struct InputDeviceXTest {
    InputDevice base;
    Display* display;
    Window root;
    int keyboard_state[KEYCODE_UPPERBOUND];
    bool caps_lock;
    bool num_lock;
} InputDeviceXTest;

// @brief Linux keycodes for replaying Whist user inputs on server
// @details index is Whist keycode, value is Linux keycode
#define GetX11KeySym(whist_keycode) x11_keysyms[whist_keycode]
static const int x11_keysyms[KEYCODE_UPPERBOUND] = {
    0,                        // Whist keycodes start at index 4
    0,                        // Whist keycodes start at index 4
    0,                        // Whist keycodes start at index 4
    0,                        // Whist keycodes start at index 4
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
    0,                        // 50 -> no Whist keycode at index 50
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
    0,                        // 100 -> no Whist keycode at index 100
    XK_Menu,                  // 101 -> Application
    0,                        // 102 -> no Whist keycode at index 102
    0,                        // 103 -> no Whist keycode at index 103
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
    0,                        // 120 -> no Whist keycode at index 120
    0,                        // 121 -> no Whist keycode at index 121
    0,                        // 122 -> no Whist keycode at index 122
    0,                        // 123 -> no Whist keycode at index 123
    0,                        // 124 -> no Whist keycode at index 124
    0,                        // 125 -> no Whist keycode at index 125
    0,                        // 126 -> no Whist keycode at index 126
    XF86XK_AudioMute,         // 127 -> Mute
    XF86XK_AudioRaiseVolume,  // 128 -> Volume Up
    XF86XK_AudioLowerVolume,  // 129 -> Volume Down
    0,                        // 130 -> no Whist keycode at index 130
    0,                        // 131 -> no Whist keycode at index 131
    0,                        // 132 -> no Whist keycode at index 132
    0,                        // 133 -> no Whist keycode at index 133
    0,                        // 134 -> no Whist keycode at index 134
    0,                        // 135 -> no Whist keycode at index 135
    0,                        // 136 -> no Whist keycode at index 136
    0,                        // 137 -> no Whist keycode at index 137
    0,                        // 138 -> no Whist keycode at index 138
    0,                        // 139 -> no Whist keycode at index 139
    0,                        // 140 -> no Whist keycode at index 140
    0,                        // 141 -> no Whist keycode at index 141
    0,                        // 142 -> no Whist keycode at index 142
    0,                        // 143 -> no Whist keycode at index 143
    0,                        // 144 -> no Whist keycode at index 144
    0,                        // 145 -> no Whist keycode at index 145
    0,                        // 146 -> no Whist keycode at index 146
    0,                        // 147 -> no Whist keycode at index 147
    0,                        // 148 -> no Whist keycode at index 148
    0,                        // 149 -> no Whist keycode at index 149
    0,                        // 150 -> no Whist keycode at index 150
    0,                        // 151 -> no Whist keycode at index 151
    0,                        // 152 -> no Whist keycode at index 152
    0,                        // 153 -> no Whist keycode at index 153
    0,                        // 154 -> no Whist keycode at index 154
    0,                        // 155 -> no Whist keycode at index 155
    0,                        // 156 -> no Whist keycode at index 156
    0,                        // 157 -> no Whist keycode at index 157
    0,                        // 158 -> no Whist keycode at index 158
    0,                        // 159 -> no Whist keycode at index 159
    0,                        // 160 -> no Whist keycode at index 160
    0,                        // 161 -> no Whist keycode at index 161
    0,                        // 162 -> no Whist keycode at index 162
    0,                        // 163 -> no Whist keycode at index 163
    0,                        // 164 -> no Whist keycode at index 164
    0,                        // 165 -> no Whist keycode at index 165
    0,                        // 166 -> no Whist keycode at index 166
    0,                        // 167 -> no Whist keycode at index 167
    0,                        // 168 -> no Whist keycode at index 168
    0,                        // 169 -> no Whist keycode at index 169
    0,                        // 170 -> no Whist keycode at index 170
    0,                        // 171 -> no Whist keycode at index 171
    0,                        // 172 -> no Whist keycode at index 172
    0,                        // 173 -> no Whist keycode at index 173
    0,                        // 174 -> no Whist keycode at index 174
    0,                        // 175 -> no Whist keycode at index 175
    0,                        // 176 -> no Whist keycode at index 176
    0,                        // 177 -> no Whist keycode at index 177
    0,                        // 178 -> no Whist keycode at index 178
    0,                        // 179 -> no Whist keycode at index 179
    0,                        // 180 -> no Whist keycode at index 180
    0,                        // 181 -> no Whist keycode at index 181
    0,                        // 182 -> no Whist keycode at index 182
    0,                        // 183 -> no Whist keycode at index 183
    0,                        // 184 -> no Whist keycode at index 184
    0,                        // 185 -> no Whist keycode at index 185
    0,                        // 186 -> no Whist keycode at index 186
    0,                        // 187 -> no Whist keycode at index 187
    0,                        // 188 -> no Whist keycode at index 188
    0,                        // 189 -> no Whist keycode at index 189
    0,                        // 190 -> no Whist keycode at index 190
    0,                        // 191 -> no Whist keycode at index 191
    0,                        // 192 -> no Whist keycode at index 192
    0,                        // 193 -> no Whist keycode at index 193
    0,                        // 194 -> no Whist keycode at index 194
    0,                        // 195 -> no Whist keycode at index 195
    0,                        // 196 -> no Whist keycode at index 196
    0,                        // 197 -> no Whist keycode at index 197
    0,                        // 198 -> no Whist keycode at index 198
    0,                        // 199 -> no Whist keycode at index 199
    0,                        // 200 -> no Whist keycode at index 200
    0,                        // 201 -> no Whist keycode at index 201
    0,                        // 202 -> no Whist keycode at index 202
    0,                        // 203 -> no Whist keycode at index 203
    0,                        // 204 -> no Whist keycode at index 204
    0,                        // 205 -> no Whist keycode at index 205
    0,                        // 206 -> no Whist keycode at index 206
    0,                        // 207 -> no Whist keycode at index 207
    0,                        // 208 -> no Whist keycode at index 208
    0,                        // 209 -> no Whist keycode at index 209
    0,                        // 210 -> no Whist keycode at index 210
    0,                        // 211 -> no Whist keycode at index 211
    0,                        // 212 -> no Whist keycode at index 212
    0,                        // 213 -> no Whist keycode at index 213
    0,                        // 214 -> no Whist keycode at index 214
    0,                        // 215 -> no Whist keycode at index 215
    0,                        // 216 -> no Whist keycode at index 216
    0,                        // 217 -> no Whist keycode at index 217
    0,                        // 218 -> no Whist keycode at index 218
    0,                        // 219 -> no Whist keycode at index 219
    0,                        // 220 -> no Whist keycode at index 220
    0,                        // 221 -> no Whist keycode at index 221
    0,                        // 222 -> no Whist keycode at index 222
    0,                        // 223 -> no Whist keycode at index 223
    XK_Control_L,             // 224 -> Left Ctrl
    XK_Shift_L,               // 225 -> Left Shift
    XK_Alt_L,                 // 226 -> Left Alt
    XK_Super_L,               // 227 -> Left GUI (Windows Key)
    XK_Control_R,             // 228 -> Right Ctrl
    XK_Shift_R,               // 229 -> Right Shift
    XK_Alt_R,                 // 230 -> Right Alt
    XK_Super_R,               // 231 -> Right GUI (Windows Key)
    0,                        // 232 -> no Whist keycode at index 232
    0,                        // 233 -> no Whist keycode at index 233
    0,                        // 234 -> no Whist keycode at index 234
    0,                        // 235 -> no Whist keycode at index 235
    0,                        // 236 -> no Whist keycode at index 236
    0,                        // 237 -> no Whist keycode at index 237
    0,                        // 238 -> no Whist keycode at index 238
    0,                        // 239 -> no Whist keycode at index 239
    0,                        // 240 -> no Whist keycode at index 240
    0,                        // 241 -> no Whist keycode at index 241
    0,                        // 242 -> no Whist keycode at index 242
    0,                        // 243 -> no Whist keycode at index 243
    0,                        // 244 -> no Whist keycode at index 244
    0,                        // 245 -> no Whist keycode at index 245
    0,                        // 246 -> no Whist keycode at index 246
    0,                        // 247 -> no Whist keycode at index 247
    0,                        // 248 -> no Whist keycode at index 248
    0,                        // 249 -> no Whist keycode at index 249
    0,                        // 250 -> no Whist keycode at index 250
    0,                        // 251 -> no Whist keycode at index 251
    0,                        // 252 -> no Whist keycode at index 252
    0,                        // 253 -> no Whist keycode at index 253
    0,                        // 254 -> no Whist keycode at index 254
    0,                        // 255 -> no Whist keycode at index 255
    0,                        // 256 -> no Whist keycode at index 256
    XK_Mode_switch,           // 257 -> ModeSwitch
    XF86XK_AudioNext,         // 258 -> Audio/Media Next
    XF86XK_AudioPrev,         // 259 -> Audio/Media Prev
    XF86XK_AudioStop,         // 260 -> Audio/Media Stop
    XF86XK_AudioPause,        // 261 -> Audio/Media Play
    XF86XK_AudioMute,         // 262 -> Audio/Media Mute
    XF86XK_AudioMedia         // 263 -> Media Select
};

static void get_input_dimensions(InputDeviceXTest* input_device, int32_t* w, int32_t* h) {
    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(input_device->display, input_device->root, &window_attributes)) {
        *w = 0;
        *h = 0;
        LOG_ERROR("Error while getting window attributes");
        return;
    }
    *w = window_attributes.width;
    *h = window_attributes.height;
}

static void xtest_destroy_input_device(InputDeviceXTest* input_device) {
    if (input_device) {
        XCloseDisplay(input_device->display);
        free(input_device);
    }
}

static int xtest_get_keyboard_modifier_state(InputDeviceXTest* input_device,
                                             WhistKeycode whist_keycode) {
    switch (whist_keycode) {
        case FK_CAPSLOCK:
            return input_device->caps_lock;
        case FK_NUMLOCK:
            return input_device->num_lock;
        default:
            LOG_WARNING("Not a modifier!");
            return -1;
    }
}

static int xtest_get_keyboard_key_state(InputDeviceXTest* input_device,
                                        WhistKeycode whist_keycode) {
    if ((int)whist_keycode >= KEYCODE_UPPERBOUND) {
        return 0;
    } else {
        return input_device->keyboard_state[whist_keycode];
    }
}

static int xtest_ignore_key_state(InputDeviceXTest* input_device, WhistKeycode whist_keycode,
                                  bool active_pinch) {
    return 0;
}

static int xtest_emit_key_event(InputDeviceXTest* input_device, WhistKeycode whist_keycode,
                                int pressed) {
    XLockDisplay(input_device->display);
    KeyCode kcode = XKeysymToKeycode(input_device->display, GetX11KeySym(whist_keycode));
    if (!kcode) {
        return -1;
    }
    XTestFakeKeyEvent(input_device->display, kcode, pressed, CurrentTime);
    XSync(input_device->display, false);

    XUnlockDisplay(input_device->display);

    input_device->keyboard_state[whist_keycode] = pressed;

    if (whist_keycode == FK_CAPSLOCK && pressed) {
        input_device->caps_lock = !input_device->caps_lock;
    }
    if (whist_keycode == FK_NUMLOCK && pressed) {
        input_device->num_lock = !input_device->num_lock;
    }
    return 0;
}

static int xtest_emit_mouse_motion_event(InputDeviceXTest* input_device, int32_t x, int32_t y,
                                         int relative) {
    XLockDisplay(input_device->display);
    if (relative) {
        XTestFakeRelativeMotionEvent(input_device->display, x, y, CurrentTime);
    } else {
        int32_t w, h;
        get_input_dimensions(input_device, &w, &h);
        XTestFakeMotionEvent(input_device->display, -1, (int)(x * w / MOUSE_SCALING_FACTOR),
                             (int)(y * h / MOUSE_SCALING_FACTOR), CurrentTime);
    }
    XSync(input_device->display, false);
    XUnlockDisplay(input_device->display);
    return 0;
}

static int xtest_emit_mouse_button_event(InputDeviceXTest* input_device, WhistMouseButton button,
                                         int pressed) {
    XLockDisplay(input_device->display);
    XTestFakeButtonEvent(input_device->display, button, pressed, CurrentTime);
    XSync(input_device->display, false);
    XUnlockDisplay(input_device->display);
    return 0;
}

static int xtest_emit_low_res_mouse_wheel_event(InputDeviceXTest* input_device, int32_t x,
                                                int32_t y) {
    XLockDisplay(input_device->display);

    if (y > 0) {
        // Up
        XTestFakeButtonEvent(input_device->display, 4, 1, CurrentTime);
        XTestFakeButtonEvent(input_device->display, 4, 0, CurrentTime);
    } else if (y < 0) {
        // Down
        XTestFakeButtonEvent(input_device->display, 5, 1, CurrentTime);
        XTestFakeButtonEvent(input_device->display, 5, 0, CurrentTime);
    }
    if (x < 0) {
        // Left
        XTestFakeButtonEvent(input_device->display, 6, 1, CurrentTime);
        XTestFakeButtonEvent(input_device->display, 6, 0, CurrentTime);
    } else if (x > 0) {
        // Right
        XTestFakeButtonEvent(input_device->display, 7, 1, CurrentTime);
        XTestFakeButtonEvent(input_device->display, 7, 0, CurrentTime);
    }

    XSync(input_device->display, false);
    XUnlockDisplay(input_device->display);
    return 0;
}

static int xtest_emit_multigesture_event(InputDeviceXTest* input_device, float d_theta,
                                         float d_dist, WhistMultigestureType gesture_type,
                                         bool active_gesture) {
    UNUSED(input_device);
    UNUSED(d_theta);
    UNUSED(d_dist);
    UNUSED(gesture_type);
    UNUSED(active_gesture);

    LOG_WARNING("Multigesture events not implemented for XTest driver! ");
    return -1;
}

InputDevice* xtest_create_input_device(void) {
    LOG_INFO("creating xtest input driver");

    InputDeviceXTest* ret = safe_malloc(sizeof(*ret));
    ret->display = XOpenDisplay(NULL);
    ret->root = DefaultRootWindow(ret->display);

    InputDevice* base = &ret->base;
    base->device_type = WHIST_INPUT_DEVICE_XTEST;
    base->get_keyboard_key_state =
        (InputDeviceGetKeyboardModifierStateFn)xtest_get_keyboard_key_state;
    base->get_keyboard_modifier_state =
        (InputDeviceGetKeyboardModifierStateFn)xtest_get_keyboard_modifier_state;
    base->ignore_key_state = (InputDeviceIgnoreKeyStateFn)xtest_ignore_key_state;
    base->emit_key_event = (InputDeviceEmitKeyEventFn)xtest_emit_key_event;
    base->emit_mouse_motion_event =
        (InputDeviceEmitMouseMotionEventFn)xtest_emit_mouse_motion_event;
    base->emit_mouse_button_event =
        (InputDeviceEmitMouseButtonEventFn)xtest_emit_mouse_button_event;
    base->emit_low_res_mouse_wheel_event =
        (InputDeviceEmitLowResMouseWheelEventFn)xtest_emit_low_res_mouse_wheel_event;
    base->emit_high_res_mouse_wheel_event = NULL;
    base->emit_multigesture_event =
        (InputDeviceEmitMultiGestureEventFn)xtest_emit_multigesture_event;
    base->destroy = (InputDeviceDestroyFn)xtest_destroy_input_device;

    return base;
}
