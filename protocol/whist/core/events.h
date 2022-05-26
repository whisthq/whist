#ifndef PROTOCOL_CORE_EVENTS_H
#define PROTOCOL_CORE_EVENTS_H

/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file whist.h
 * @brief This file contains the core of Whist custom structs and definitions
 *        used throughout.
 */

#include <stdbool.h>

/**
 * @brief   Keycodes.
 * @details Different accepted keycodes from client input.
 */
typedef enum WhistKeycode {
    FK_UNKNOWN = 0,        ///< 0
    FK_A = 4,              ///< 4
    FK_B = 5,              ///< 5
    FK_C = 6,              ///< 6
    FK_D = 7,              ///< 7
    FK_E = 8,              ///< 8
    FK_F = 9,              ///< 9
    FK_G = 10,             ///< 10
    FK_H = 11,             ///< 11
    FK_I = 12,             ///< 12
    FK_J = 13,             ///< 13
    FK_K = 14,             ///< 14
    FK_L = 15,             ///< 15
    FK_M = 16,             ///< 16
    FK_N = 17,             ///< 17
    FK_O = 18,             ///< 18
    FK_P = 19,             ///< 19
    FK_Q = 20,             ///< 20
    FK_R = 21,             ///< 21
    FK_S = 22,             ///< 22
    FK_T = 23,             ///< 23
    FK_U = 24,             ///< 24
    FK_V = 25,             ///< 25
    FK_W = 26,             ///< 26
    FK_X = 27,             ///< 27
    FK_Y = 28,             ///< 28
    FK_Z = 29,             ///< 29
    FK_1 = 30,             ///< 30
    FK_2 = 31,             ///< 31
    FK_3 = 32,             ///< 32
    FK_4 = 33,             ///< 33
    FK_5 = 34,             ///< 34
    FK_6 = 35,             ///< 35
    FK_7 = 36,             ///< 36
    FK_8 = 37,             ///< 37
    FK_9 = 38,             ///< 38
    FK_0 = 39,             ///< 39
    FK_ENTER = 40,         ///< 40
    FK_ESCAPE = 41,        ///< 41
    FK_BACKSPACE = 42,     ///< 42
    FK_TAB = 43,           ///< 43
    FK_SPACE = 44,         ///< 44
    FK_MINUS = 45,         ///< 45
    FK_EQUALS = 46,        ///< 46
    FK_LBRACKET = 47,      ///< 47
    FK_RBRACKET = 48,      ///< 48
    FK_BACKSLASH = 49,     ///< 49
    FK_SEMICOLON = 51,     ///< 51
    FK_APOSTROPHE = 52,    ///< 52
    FK_BACKTICK = 53,      ///< 53
    FK_COMMA = 54,         ///< 54
    FK_PERIOD = 55,        ///< 55
    FK_SLASH = 56,         ///< 56
    FK_CAPSLOCK = 57,      ///< 57
    FK_F1 = 58,            ///< 58
    FK_F2 = 59,            ///< 59
    FK_F3 = 60,            ///< 60
    FK_F4 = 61,            ///< 61
    FK_F5 = 62,            ///< 62
    FK_F6 = 63,            ///< 63
    FK_F7 = 64,            ///< 64
    FK_F8 = 65,            ///< 65
    FK_F9 = 66,            ///< 66
    FK_F10 = 67,           ///< 67
    FK_F11 = 68,           ///< 68
    FK_F12 = 69,           ///< 69
    FK_PRINTSCREEN = 70,   ///< 70
    FK_SCROLLLOCK = 71,    ///< 71
    FK_PAUSE = 72,         ///< 72
    FK_INSERT = 73,        ///< 73
    FK_HOME = 74,          ///< 74
    FK_PAGEUP = 75,        ///< 75
    FK_DELETE = 76,        ///< 76
    FK_END = 77,           ///< 77
    FK_PAGEDOWN = 78,      ///< 78
    FK_RIGHT = 79,         ///< 79
    FK_LEFT = 80,          ///< 80
    FK_DOWN = 81,          ///< 81
    FK_UP = 82,            ///< 82
    FK_NUMLOCK = 83,       ///< 83
    FK_KP_DIVIDE = 84,     ///< 84
    FK_KP_MULTIPLY = 85,   ///< 85
    FK_KP_MINUS = 86,      ///< 86
    FK_KP_PLUS = 87,       ///< 87
    FK_KP_ENTER = 88,      ///< 88
    FK_KP_1 = 89,          ///< 89
    FK_KP_2 = 90,          ///< 90
    FK_KP_3 = 91,          ///< 91
    FK_KP_4 = 92,          ///< 92
    FK_KP_5 = 93,          ///< 93
    FK_KP_6 = 94,          ///< 94
    FK_KP_7 = 95,          ///< 95
    FK_KP_8 = 96,          ///< 96
    FK_KP_9 = 97,          ///< 97
    FK_KP_0 = 98,          ///< 98
    FK_KP_PERIOD = 99,     ///< 99
    FK_APPLICATION = 101,  ///< 101
    FK_F13 = 104,          ///< 104
    FK_F14 = 105,          ///< 105
    FK_F15 = 106,          ///< 106
    FK_F16 = 107,          ///< 107
    FK_F17 = 108,          ///< 108
    FK_F18 = 109,          ///< 109
    FK_F19 = 110,          ///< 110
    FK_MENU = 118,         ///< 118
    FK_MUTE = 127,         ///< 127
    FK_VOLUMEUP = 128,     ///< 128
    FK_VOLUMEDOWN = 129,   ///< 129
    FK_LCTRL = 224,        ///< 224
    FK_LSHIFT = 225,       ///< 225
    FK_LALT = 226,         ///< 226
    FK_LGUI = 227,         ///< 227
    FK_RCTRL = 228,        ///< 228
    FK_RSHIFT = 229,       ///< 229
    FK_RALT = 230,         ///< 230
    FK_RGUI = 231,         ///< 231
    FK_AUDIONEXT = 258,    ///< 258
    FK_AUDIOPREV = 259,    ///< 259
    FK_AUDIOSTOP = 260,    ///< 260
    FK_AUDIOPLAY = 261,    ///< 261
    FK_AUDIOMUTE = 262,    ///< 262
    FK_MEDIASELECT = 263,  ///< 263
} WhistKeycode;
// An (exclusive) upper bound on any keycode
#define KEYCODE_UPPERBOUND 265

/**
 * @brief   Modifier keys applied to keyboard input.
 * @details Codes for when keyboard input is modified. These values may be
 *          bitwise OR'd together.
 */
typedef enum WhistKeymod {
    MOD_NONE = 0x0000,    ///< No modifier key active.
    MOD_LSHIFT = 0x0001,  ///< `LEFT SHIFT` is currently active.
    MOD_RSHIFT = 0x0002,  ///< `RIGHT SHIFT` is currently active.
    MOD_LCTRL = 0x0040,   ///< `LEFT CONTROL` is currently active.
    MOD_RCTRL = 0x0080,   ///< `RIGHT CONTROL` is currently active.
    MOD_LALT = 0x0100,    ///< `LEFT ALT` is currently active.
    MOD_RALT = 0x0200,    ///< `RIGHT ALT` is currently active.
    MOD_NUM = 0x1000,     ///< `NUMLOCK` is currently active.
    MOD_CAPS = 0x2000,    ///< `CAPSLOCK` is currently active.
    MOD_LGUI = 0x4000,    ///< `LEFT GUI` is currently active.
    MOD_RGUI = 0x8000,    ///< `RIGHT GUI` is currently active.
} WhistKeymod;

/**
 * @brief   Mouse button.
 * @details Codes for encoding mouse actions.
 */
typedef enum WhistMouseButton {
    MOUSE_L = 1,       ///< Left mouse button.
    MOUSE_MIDDLE = 2,  ///< Middle mouse button.
    MOUSE_R = 3,       ///< Right mouse button.
    MOUSE_X1 = 4,      ///< Extra mouse button 1.
    MOUSE_X2 = 5,      ///< Extra mouse button 2.
    MOUSE_MAKE_32 = 0x7FFFFFFF,
} WhistMouseButton;

/**
 * @brief   Keyboard message.
 * @details Messages related to keyboard usage.
 */
typedef struct WhistKeyboardMessage {
    WhistKeycode code;  ///< Keyboard input.
    WhistKeymod mod;    ///< Stateful modifier keys applied to keyboard input.
    bool pressed;       ///< `true` if pressed, `false` if released.
    uint8_t __pad[3];
} WhistKeyboardMessage;

/**
 * @brief   Mouse button message.
 * @details Message from mouse button.
 */
typedef struct WhistMouseButtonMessage {
    WhistMouseButton button;  ///< Mouse button.
    bool pressed;             ///< `true` if clicked, `false` if released.
    uint8_t __pad[3];
} WhistMouseButtonMessage;

/**
 * @brief   Scroll momentum type.
 * @details The type of scroll momentum.
 */
typedef enum WhistMouseWheelMomentumType {
    MOUSEWHEEL_MOMENTUM_NONE = 0,
    MOUSEWHEEL_MOMENTUM_BEGIN = 1,
    MOUSEWHEEL_MOMENTUM_ACTIVE = 2,
    MOUSEWHEEL_MOMENTUM_END = 3,
} WhistMouseWheelMomentumType;

/**
 * @brief   Mouse wheel message.
 * @details Message from mouse wheel.
 */
typedef struct WhistMouseWheelMessage {
    int32_t x;        ///< Horizontal delta of mouse wheel rotation. Negative values
                      ///< scroll left. Only used for Windows server.
    int32_t y;        ///< Vertical delta of mouse wheel rotation. Negative values
                      ///< scroll up. Only used for Windows server.
    float precise_x;  ///< Horizontal floating delta of mouse wheel/trackpad scrolling.
    float precise_y;  ///< Vertical floating delta of mouse wheel/trackpad scrolling.
} WhistMouseWheelMessage;

/**
 * @brief   Mouse motion message.
 * @details Member of WhistMessage. Mouse motion can be sent in either
 *          relative or absolute mode via the `relative` member. Absolute mode treats
 *          the `x` and `y` values as the exact destination for where the cursor will
 *          appear. These values are sent from the client in device screen coordinates
 *          and are translated in accordance with the values set via
 *          WhistClientSetDimensions. Relative mode `x` and `y` values are not
 *          affected by WhistClientSetDimensions and move the cursor with a signed
 *          delta value from its previous location.
 */
typedef struct WhistMouseMotionMessage {
    int32_t x;      ///< The absolute horizontal screen coordinate of the cursor  if
                    ///< `relative` is `false`, or the delta (can be negative) if
                    ///< `relative` is `true`.
    int32_t y;      ///< The absolute vertical screen coordinate of the cursor if
                    ///< `relative` is `false`, or the delta (can be negative) if
                    ///< `relative` is `true`.
    bool relative;  ///< `true` for relative mode, `false` for absolute mode.
                    ///< See details.
    int x_nonrel;
    int y_nonrel;
    uint8_t __pad[3];
} WhistMouseMotionMessage;

/**
 * @brief   Multigesture type.
 * @details The type of multigesture.
 */
typedef enum WhistMultigestureType {
    MULTIGESTURE_NONE = 0,
    MULTIGESTURE_PINCH_OPEN = 1,
    MULTIGESTURE_PINCH_CLOSE = 2,
    MULTIGESTURE_ROTATE = 3,
    MULTIGESTURE_CANCEL = 4,
} WhistMultigestureType;

/**
 * @brief   OS type
 * @details An enum of OS types
 */
typedef enum WhistOSType {
    WHIST_UNKNOWN_OS = 0,
    WHIST_WINDOWS = 1,
    WHIST_APPLE = 2,
    WHIST_LINUX = 3,
} WhistOSType;

/**
 * @brief   Drag state
 * @details An enum of drag states
 */
typedef enum WhistDragState {
    START_DRAG = 0,
    IN_DRAG = 1,
    END_DRAG = 2,
} WhistDragState;

/**
 * @brief   Multigesture message.
 * @details Message from multigesture event on touchpad.
 */
typedef struct WhistMultigestureMessage {
    float d_theta;                       ///< The amount the fingers rotated.
    float d_dist;                        ///< The amount the fingers pinched.
    float x;                             ///< Normalized gesture x-axis center.
    float y;                             ///< Normalized gesture y-axis center.
    uint16_t num_fingers;                ///< Number of fingers used in the gesture.
    bool active_gesture;                 ///< Whether this multigesture is already active.
    WhistMultigestureType gesture_type;  ///< Multigesture type
} WhistMultigestureMessage;

/**
 * @brief   File drag message.
 * @details Message from file drag update event.
 */
typedef struct {
    int x;
    int y;
    WhistDragState drag_state;
    int group_id;      // This should be ascending with each new drag group
    char filename[0];  // Should only have contents when drag_state is START_DRAG
} WhistFileDragData;

#endif  // PROTOCOL_CORE_EVENTS_H
