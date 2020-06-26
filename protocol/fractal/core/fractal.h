#ifndef FRACTAL_H
#define FRACTAL_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file fractal.h
 * @brief This file contains the core of Fractal custom structs and definitions
 *        used throughout.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32)
#pragma warning(disable : 4200)
#define _WINSOCKAPI_
#include <Audioclient.h>
#include <avrt.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <process.h>
#include <synchapi.h>
#include <windows.h>
#include <winuser.h>

#include "shellscalingapi.h"
#else
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_qsv.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#define SDL_MAIN_HANDLED
#include "../../include/SDL2/SDL.h"
#include "../../include/SDL2/SDL_thread.h"
#include "../clipboard/clipboard_synchronizer.h"
#include "../cursor/cursor.h"
#include "../network/network.h"
#include "../utils/clock.h"
#include "../utils/logging.h"

/*
============================
Defines
============================
*/

#define NUM_KEYCODES 265

#define PORT_CLIENT_TO_SERVER 32262
#define PORT_SERVER_TO_CLIENT 32263
#define PORT_SHARED_TCP 32264
#define PORT_SPECTATOR 32265

#define PRODUCTION_HOST "cube-celery-vm.herokuapp.com"
#define STAGING_HOST "cube-celery-staging.herokuapp.com"

#define USING_STUN true
#define USING_AUDIO_ENCODE_DECODE true

#if defined(_WIN32)
// possible on windows, so let's do it
#define USING_SERVERSIDE_SCALE true
#else
// not possible yet on linux
#define USING_SERVERSIDE_SCALE false
#endif

#define MAXIMUM_BITRATE 30000000
#define MINIMUM_BITRATE 2000000
#define ACK_REFRESH_MS 50

#define LARGEST_FRAME_SIZE 1000000
#define STARTING_BITRATE 10400000
#define STARTING_BURST_BITRATE 31800000

#define AUDIO_BITRATE 128000
#define FPS 50
#define MIN_FPS 10
#define OUTPUT_WIDTH 1280
#define OUTPUT_HEIGHT 720

#define PRIVATE_KEY \
    "\xED\x5E\xF3\x3C\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\x19\xEF"

#define MOUSE_SCALING_FACTOR 100000

#define WRITE_MPRINTF_TO_LOG true

/*
============================
Custom Types
============================
*/

typedef enum EncodeType {
    SOFTWARE_ENCODE = 0,
    NVENC_ENCODE = 1,
    QSV_ENCODE = 2
} EncodeType;

typedef enum CodecType {
    CODEC_TYPE_UNKNOWN = 0,
    CODEC_TYPE_H264 = 264,
    CODEC_TYPE_H265 = 265,
    __CODEC_TYPE_MAKE_32 = 0x7FFFFFFF
} CodecType;

typedef enum FractalCursorID {
    CURSOR_ID_APPSTARTING = 32650,
    CURSOR_ID_NORMAL = 32512,
    CURSOR_ID_CROSS = 32515,
    CURSOR_ID_HAND = 32649,
    CURSOR_ID_HELP = 32651,
    CURSOR_ID_IBEAM = 32513,
    CURSOR_ID_NO = 32648,
    CURSOR_ID_SIZEALL = 32646,
    CURSOR_ID_SIZENESW = 32643,
    CURSOR_ID_SIZENS = 32645,
    CURSOR_ID_SIZENWSE = 32642,
    CURSOR_ID_SIZEWE = 32644,
    CURSOR_ID_UP = 32516,
    CURSOR_ID_WAIT = 32514
} FractalCursorID;

typedef enum FractalKeycode {
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
} FractalKeycode;

/// @brief Modifier keys applied to keyboard input.
/// @details Codes for when keyboard input is modified. These values may be
/// bitwise OR'd together.
typedef enum FractalKeymod {
    MOD_NONE = 0x0000,    ///< No modifier key active.
    MOD_LSHIFT = 0x0001,  ///< `LEFT SHIFT` is currently active.
    MOD_RSHIFT = 0x0002,  ///< `RIGHT SHIFT` is currently active.
    MOD_LCTRL = 0x0040,   ///< `LEFT CONTROL` is currently active.
    MOD_RCTRL = 0x0080,   ///< `RIGHT CONTROL` is currently active.
    MOD_LALT = 0x0100,    ///< `LEFT ALT` is currently active.
    MOD_RALT = 0x0200,    ///< `RIGHT ALT` is currently active.
    MOD_NUM = 0x1000,     ///< `NUMLOCK` is currently active.
    MOD_CAPS = 0x2000,    ///< `CAPSLOCK` is currently active.
} FractalKeymod;

/// @brief Mouse button.
/// @details Codes for encoding mouse actions.
typedef enum FractalMouseButton {
    MOUSE_L = 1,       ///< Left mouse button.
    MOUSE_MIDDLE = 2,  ///< Middle mouse button.
    MOUSE_R = 3,       ///< Right mouse button.
    MOUSE_X1 = 4,      ///< Extra mouse button 1.
    MOUSE_X2 = 5,      ///< Extra mouse button 2.
    __MOUSE_MAKE_32 = 0x7FFFFFFF,
} FractalMouseButton;

/// @brief Cursor properties.
/// @details Track important information on cursor.
typedef struct FractalCursor {
    uint32_t size;  ///< Size in bytes of the cursor image buffer.
    uint32_t
        positionX;  ///< When leaving relative mode, the horizontal position
                    ///< in screen coordinates where the cursor reappears.
    uint32_t positionY;  ///< When leaving relative mode, the vertical position
                         ///< in screen coordinates where the cursor reappears.
    uint16_t width;      ///< Width of the cursor image in pixels.
    uint16_t height;     ///< Height of the cursor position in pixels.
    uint16_t hotX;  ///< Horizontal pixel position of the cursor hotspot within
                    ///< the image.
    uint16_t hotY;  ///< Vertical pixel position of the cursor hotspot within
                    ///< the image.
    bool modeUpdate;   ///< `true` if the cursor mode should be updated. The
                       ///< `relative`, `positionX`, and `positionY` members are
                       ///< valid.
    bool imageUpdate;  ///< `true` if the cursor image should be updated. The
                       ///< `width`, `height`, `hotX`, `hotY`, and `size`
                       ///< members are valid.
    bool relative;  ///< `true` if in relative mode, meaning the client should
                    ///< submit mouse motion in relative distances rather than
                    ///< absolute screen coordinates.
    uint8_t __pad[1];
} FractalCursor;

/// @brief Keyboard message.
/// @details Messages related to keyboard usage.
typedef struct FractalKeyboardMessage {
    FractalKeycode code;  ///< Keyboard input.
    FractalKeymod mod;    ///< Stateful modifier keys applied to keyboard input.
    bool pressed;         ///< `true` if pressed, `false` if released.
    uint8_t __pad[3];
} FractalKeyboardMessage;

/// @brief Mouse button message.
/// @details Message from mouse button.
typedef struct FractalMouseButtonMessage {
    FractalMouseButton button;  ///< Mouse button.
    bool pressed;               ///< `true` if clicked, `false` if released.
    uint8_t __pad[3];
} FractalMouseButtonMessage;

/// @brief Mouse wheel message.
/// @details Message from mouse wheel.
typedef struct FractalMouseWheelMessage {
    int32_t x;  ///< Horizontal delta of mouse wheel rotation. Negative values
                ///< scroll left.
    int32_t y;  ///< Vertical delta of mouse wheel rotation. Negative values
                ///< scroll up.
} FractalMouseWheelMessage;

/// @brief Mouse motion message.
/// @details Member of FractalMessage. Mouse motion can be sent in either
/// relative or absolute mode via the `relative` member. Absolute mode treats
/// the `x` and `y` values as the exact destination for where the cursor will
/// appear. These values are sent from the client in device screen coordinates
/// and are translated in accordance with the values set via
/// FractalClientSetDimensions. Relative mode `x` and `y` values are not
/// affected by FractalClientSetDimensions and move the cursor with a signed
/// delta value from its previous location.
typedef struct FractalMouseMotionMessage {
    int32_t x;  ///< The absolute horizontal screen coordinate of the cursor  if
                ///< `relative` is `false`, or the delta (can be negative) if
                ///< `relative` is `true`.
    int32_t y;  ///< The absolute vertical screen coordinate of the cursor if
                ///< `relative` is `false`, or the delta (can be negative) if
                ///< `relative` is `true`.
    bool relative;  ///< `true` for relative mode, `false` for absolute mode.
                    ///< See details.
    uint8_t __pad[3];
} FractalMouseMotionMessage;

typedef enum FractalClientMessageType {
    CMESSAGE_NONE = 0,     ///< No Message
    MESSAGE_KEYBOARD = 1,  ///< `keyboard` FractalKeyboardMessage is valid in
                           ///< FractClientMessage.
    MESSAGE_MOUSE_BUTTON = 2,  ///< `mouseButton` FractalMouseButtonMessage is
                               ///< valid in FractClientMessage.
    MESSAGE_MOUSE_WHEEL = 3,   ///< `mouseWheel` FractalMouseWheelMessage is
                               ///< valid in FractClientMessage.
    MESSAGE_MOUSE_MOTION = 4,  ///< `mouseMotion` FractalMouseMotionMessage is
                               ///< valid in FractClientMessage.
    MESSAGE_RELEASE = 5,  ///< Message instructing the host to release all input
                          ///< that is currently pressed.
    MESSAGE_MBPS = 6,     ///< `mbps` double is valid in FractClientMessage.
    MESSAGE_PING = 7,
    MESSAGE_DIMENSIONS = 8,  ///< `dimensions.width` int and `dimensions.height`
                             ///< int is valid in FractClientMessage
    MESSAGE_VIDEO_NACK = 9,
    MESSAGE_AUDIO_NACK = 10,
    MESSAGE_KEYBOARD_STATE = 11,
    CMESSAGE_CLIPBOARD = 12,
    MESSAGE_IFRAME_REQUEST = 13,
    MESSAGE_TIME = 14,
    CMESSAGE_QUIT = 100,
} FractalClientMessageType;

typedef struct FractalClientMessage {
    FractalClientMessageType type;  ///< Input message type.
    union {
        FractalKeyboardMessage keyboard;        ///< Keyboard message.
        FractalMouseButtonMessage mouseButton;  ///< Mouse button message.
        FractalMouseWheelMessage mouseWheel;    ///< Mouse wheel message.
        FractalMouseMotionMessage mouseMotion;  ///< Mouse motion message.

        // MESSAGE_MBPS
        double mbps;

        // MESSAGE_PING
        int ping_id;

        // MESSAGE_DIMENSIONS
        struct dimensions {
            int width;
            int height;
            int dpi;
            CodecType codec_type;
        } dimensions;

        // MESSAGE_VIDEO_NACK or MESSAGE_AUDIO_NACK
        struct nack_data {
            int id;
            int index;
        } nack_data;

        // MESSAGE_KEYBOARD_STATE
        struct {
            short num_keycodes;
            bool caps_lock;
            bool num_lock;
            char keyboard_state[NUM_KEYCODES];
        };

        // MESSAGE_IFRAME_REQUEST
        bool reinitialize_encoder;

        FractalTimeData time_data;
    };

    // CMESSAGE_CLIPBOARD
    ClipboardData clipboard;
} FractalClientMessage;

typedef enum FractalServerMessageType {
    SMESSAGE_NONE = 0,  ///< No Message
    MESSAGE_PONG = 1,
    MESSAGE_AUDIO_FREQUENCY = 2,
    SMESSAGE_CLIPBOARD = 3,
    MESSAGE_INIT = 4,
    SMESSAGE_QUIT = 100,
} FractalServerMessageType;

typedef struct FractalServerMessageInit {
    char filename[300];
    char username[50];
    int connection_id;
} FractalServerMessageInit;

typedef struct FractalServerMessage {
    FractalServerMessageType type;  ///< Input message type.
    union {
        int ping_id;
        int frequency;
        int spectator_port;
    };
    union {
        ClipboardData clipboard;
        char init_msg[0];
    };
} FractalServerMessage;

typedef struct FractalDestination {
    int host;
    int port;
} FractalDestination;

typedef struct Frame {
    FractalCursorImage cursor;
    int width;
    int height;
    CodecType codec_type;
    int size;
    bool is_iframe;
    unsigned char compressed_frame[];
} Frame;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Print the memory trace of a process
 */
void PrintMemoryInfo();

/**
 * @brief                          Print the system info of the computer
 */
void PrintSystemInfo();

/**
 * @brief                          Run a system command via command prompt or
 *                                 terminal
 *
 * @param cmdline                  String of the system command to run
 * @param response                 Terminal output from the cmdline
 *
 * @returns                        0 or value of pipe if success, else -1
 */
int runcmd(const char* cmdline, char** response);

/**
 * @brief                          Retrieves the public IPv4 of the computer it
 *                                 is run on
 *
 * @returns                        The string of the public IPv4 address of the
 *                                 computer
 */
char* get_ip();

/**
 * @brief                          Queries the webserver to ask if a VM is
 *                                 development VM
 *
 * @returns                        True if a VM is a "development" VM (dev
 *                                 protocol branch), False otherwise
 */
bool is_dev_vm();

/**
 * @brief                          Calculate the size of a FractalClientMessage
 *                                 struct
 *
 * @param fmsg                     The Fractal Client Message to find the size
 *
 * @returns                        The size of the Fractal Client Message struct
 */
int GetFmsgSize(FractalClientMessage* fmsg);

/**
 * @brief                          Retrieves the protocol branch this program is
 *                                 running by asking the webserver
 *
 * @returns                        The string of the branch name
 */
char* get_branch();

#endif  // FRACTAL_H
