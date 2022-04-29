#ifndef WHIST_H
#define WHIST_H

/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file whist.h
 * @brief This file contains the core of Whist custom structs and definitions
 *        used throughout.
 */

/*
============================
Includes
============================
*/

// In order to use accept4 we have to allow non-standard extensions
#if !defined(_GNU_SOURCE) && defined(__linux__)
#define _GNU_SOURCE
#endif

#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32)
#define _WINSOCKAPI_
#include <Audioclient.h>
#include <avrt.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <process.h>
#include <synchapi.h>
#include <windows.h>
#include <winuser.h>
#include <direct.h>

#include "shellscalingapi.h"
#else
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
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
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include <whist/core/whist_memory.h>
#include <whist/utils/threads.h>
#include <whist/clipboard/clipboard_synchronizer.h>
#include <whist/file/file_synchronizer.h>
#include <whist/utils/color.h>
#include <whist/utils/clock.h>
#include <whist/logging/logging.h>
#include <whist/utils/os_utils.h>

/*
============================
Defines
============================
*/

#define UNUSED(var) (void)(var)

#define CAPTURE_SPECIAL_WINDOWS_KEYS false

#define BASE_UDP_PORT 32263
#define BASE_TCP_PORT 32273

// Various control flags
#define USING_FFMPEG_IFRAME_FLAG false
// Toggle verbose logs
#define LOG_VIDEO false
#define LOG_AUDIO false
#define LOG_NACKING false
#define LOG_NETWORKING false
#define LOG_LONG_TERM_REFERENCE_FRAMES false
#define LOG_FEC_ENCODE false
#define LOG_FEC_DECODE false

#define WINAPI_INPUT_DRIVER 1
#define XTEST_INPUT_DRIVER 2
#define UINPUT_INPUT_DRIVER 3

#ifdef _WIN32

// possible on windows, so let's do it
#define USING_SERVERSIDE_SCALE true
#define INPUT_DRIVER WINAPI_INPUT_DRIVER
#define USING_NVIDIA_ENCODE false

#else

// not possible yet on linux
#define USING_SERVERSIDE_SCALE false
#define INPUT_DRIVER UINPUT_INPUT_DRIVER
#define USING_NVIDIA_CAPTURE true
#define USING_NVIDIA_ENCODE true

#endif

// Create platform-independent POSIX-style functions
#if defined(_WIN32)
#define STDOUT_FILENO _fileno(stdout)
#define safe_mkdir(dir) _mkdir(dir)
#define safe_dup(fd) _dup(fd)
#define safe_dup2(fd1, fd2) _dup2(fd1, fd2)
#define safe_open(path, flags) _open(path, flags, 0666)
#define safe_close(fd) _close(fd)
#define strerror_r(errno, buf, len) strerror_s(buf, len, errno)
#define strdup(str) _strdup(str)
#else
#define safe_mkdir(dir) mkdir(dir, 0777)
#define safe_dup(fd) dup(fd)
#define safe_dup2(fd1, fd2) dup2(fd1, fd2)
#define safe_open(path, flags) open(path, flags, 0666)
#define safe_close(fd) close(fd)
#endif

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define VSYNC_ON false

// Milliseconds between sending resize events from client to server
// Used to throttle resize event spam.
#define WINDOW_RESIZE_MESSAGE_INTERVAL 200

#define MAX_VIDEO_PACKETS 500
// Maximum allowed FEC ratio. Used for allocation of static buffers
// Don't let this get too close to 1, e.g. 0.99, or memory usage will explode
// Don't forget to modify MAX_PACKETS, when this is changed
#define MAX_FEC_RATIO 0.5
// The below value should also change if MAX_FEC_RATIO is modified.
// MAX_PACKETS = get_num_fec_packets(MAX_VIDEO_PACKETS, MAX_FEC_RATIO)
// We need a compile time integer constant for struct member allocation
#define MAX_PACKETS (MAX_VIDEO_PACKETS * 2)

#define ACK_REFRESH_MS 50

// 16:10 is the Mac aspect ratio, but we set the minimum screen to
// 500x500 since these are the Chrome minimum dimensions
#define MIN_SCREEN_WIDTH 500
#define MIN_SCREEN_HEIGHT 500
#define MAX_SCREEN_WIDTH 8192
#define MAX_SCREEN_HEIGHT 4096

// Set max FPS to 60, or 16ms
#define MAX_FPS 60
// We need to maintain a constant FPS for the video encoder to produce the bitrate requested by the
// client. Maintaining the requested video encoder bitrate is very important for any congestion
// control algorithm to work correctly.
#define MIN_FPS MAX_FPS
// The MIN_FPS mentioned will be enforced over an average of 1 second.
#define AVG_FPS_DURATION 1.0  // In seconds
// Average duration of one video frame in microseconds
#define AVG_FRAME_DURATION_IN_US (US_IN_SECOND / MIN_FPS)
// Number of identical frames to send before turning the encoder off
#define CONSECUTIVE_IDENTICAL_FRAMES 60
// FPS to send when the encoder is off
#define DISABLED_ENCODER_FPS 10
// Sometimes congestion control chooses a really low bitrate at which the visual quality is
// unbearable. To avoid that we set a lower bound on video quality and drop frames if they are going
// to exceed the bitrate chosen by congestion control algorithm. Higher the QP, Lower the bitrate
// and Lower the visual quality. Allowed QP range is 0-51 (for all kind of frames).
#define MAX_QP 40                  // Max QP value of video frames
#define MAX_INTRA_QP (MAX_QP + 6)  // Higher QP value for intra frames if supported by the encoder

#define OUTPUT_WIDTH 1280
#define OUTPUT_HEIGHT 720

#define DEFAULT_BINARY_PRIVATE_KEY \
    ((const void*)"\xED\x5E\xF3\x3C\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\x19\xEF")
#define DEFAULT_HEX_PRIVATE_KEY "ED5EF33CD728D17DB8064581428D19EF"

#define MOUSE_SCALING_FACTOR 100000

// MAXLENs are the max length of the string they represent, _without_ the null character.
// Therefore, whenever arrays are created or length of the string is compared, we should be
// comparing to *MAXLEN + 1
#define WHIST_IDENTIFIER_MAXLEN 31
// Dynamically-created D-Bus addresses are typically less than 100 char.
#define DBUS_ADDRESS_MAXLEN 100
// this maxlen is the determined Whist environment max length (the upper bound on all flags passed
// into the protocol)
#define WHIST_ARGS_MAXLEN 255
// Most browsers allow URLs of length up to 2048 characters
#define MAX_URL_LENGTH 2048
// Arbitrary max number of tabs to import from other browsers
#define MAX_NEW_TAB_URLS 1000

#define AUDIO_FREQUENCY 48000

/**
 * Global constructor function.
 *
 * This defines a function which will run before main() is called
 * whenever the object containing it has been built into a program.
 */
#if defined(__GNUC__)
#define CONSTRUCTOR(func) static void __attribute__((constructor)) func(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define CONSTRUCTOR(func)                                                   \
    static void func(void);                                                 \
    __declspec(allocate(".CRT$XCU")) void (*construct_##func)(void) = func; \
    static void func(void)
#else
// cppcheck-suppress preprocessorErrorDirective
#error "No constructor function implementation for this compiler."
#endif

/*
============================
Constants
============================
*/

#define MS_IN_SECOND 1000
#define US_IN_SECOND 1000000
#define US_IN_MS 1000
#define NS_IN_US 1000
#define NS_IN_MS 1000000
#define BYTES_IN_KILOBYTE 1024
#define BITS_IN_BYTE 8.0
#ifdef _WIN32
#define DEFAULT_DPI 96.0
#else
#define DEFAULT_DPI 72.0
#endif

// noreturn silences warnings if used for a function that should never return
#ifdef _WIN32
#define NORETURN __declspec(noreturn)
#else
#define NORETURN __attribute__((noreturn))
#endif

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_JOIN(left, right) left "\\" right
#else
/**
 * Character used to separate directory components in paths.
 */
#define PATH_SEPARATOR '/'
/**
 * Join two constant strings representing file path components.
 *
 * Adds the platform-specific path separator.
 */
#define PATH_JOIN(left, right) left "/" right
#endif

/*
============================
Custom Types
============================
*/

// Limit chunk size to 1 megabyte.
//     This is not because of limitations of TCP, but rather to
//     keep the TCP thread from hanging
#define CHUNK_SIZE 1000000

/**
 * @brief                          Data packet description
 */
typedef enum {
    PACKET_AUDIO = 0,
    PACKET_VIDEO = 1,
    PACKET_MESSAGE = 2,
    NUM_PACKET_TYPES = 3,
} WhistPacketType;

/**
 * @brief   Codec types.
 * @details The codec type being used for video encoding.
 */
typedef enum CodecType {
    CODEC_TYPE_UNKNOWN = 0,
    CODEC_TYPE_H264 = 264,
    CODEC_TYPE_H265 = 265,
    CODEC_TYPE_MAKE_32 = 0x7FFFFFFF
} CodecType;

#include <whist/network/network.h>

/**
 * @brief           Enum indicating whether we are using the Nvidia or X11 capture device. If we
 * discover a third option for capturing, update this enum and the CaptureDevice struct below.
 */
typedef enum CaptureDeviceType { NVIDIA_DEVICE, X11_DEVICE } CaptureDeviceType;

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
    int group_id; // This should be ascending with each new drag group
    char filename[0]; // Should only have contents when drag_state is START_DRAG
} WhistFileDragData;

/**
 * @brief   Client init message.
 * @details Init packet to be sent from client to server.
 */
typedef struct ClientInitMessage {
    char user_email[WHIST_ARGS_MAXLEN + 1];
    WhistOSType os;
} ClientInitMessage;

/**
 * @brief   Init reply message.
 * @details Message sent by server in response to a ClientInitMessage.
 */
typedef struct ServerInitReplyMessage {
    int connection_id;

    /**
     * git revision of the server.
     *
     * In general, the client should match this exactly.  If it doesn't
     * then there might be incompatibility.
     */
    char git_revision[16];
    /**
     * Feature mask being used by the server.
     *
     * The client will use this to set its own features.  If this is
     * incompatible then the client will refuse to connect.
     */
    uint32_t feature_mask;
} ServerInitReplyMessage;

/**
 * @brief   Client message type.
 * @details Each message will have a specified type to indicate what information
 *          the packet is carrying between client and server.
 */
typedef enum WhistClientMessageType {
    CMESSAGE_NONE = 0,     ///< No Message
    MESSAGE_KEYBOARD = 1,  ///< `keyboard` WhistKeyboardMessage is valid in
                           ///< WhistClientMessage.
    MESSAGE_KEYBOARD_STATE = 2,
    MESSAGE_MOUSE_BUTTON = 3,  ///< `mouseButton` WhistMouseButtonMessage is
                               ///< valid in FractClientMessage.
    MESSAGE_MOUSE_WHEEL = 4,   ///< `mouseWheel` WhistMouseWheelMessage is
                               ///< valid in FractClientMessage.
    MESSAGE_MOUSE_MOTION = 5,  ///< `mouseMotion` WhistMouseMotionMessage is

    MESSAGE_MULTIGESTURE = 6,       ///< Gesture Event
    MESSAGE_RELEASE = 7,            ///< Message instructing the host to release all input
                                    ///< that is currently pressed.
    MESSAGE_STOP_STREAMING = 105,   ///< Message asking server to stop encoding/sending frames
    MESSAGE_START_STREAMING = 106,  ///< Message asking server to resume encoding/sending frames
    MESSAGE_DIMENSIONS = 110,       ///< `dimensions.width` int and `dimensions.height`
                                    ///< int is valid in FractClientMessage
    CMESSAGE_CLIPBOARD = 113,
    CMESSAGE_INIT = 115,

    MESSAGE_OPEN_URL = 117,

    CMESSAGE_FILE_METADATA = 119,   ///< file metadata
    CMESSAGE_FILE_DATA = 120,       ///< file chunk
    CMESSAGE_FILE_GROUP_END = 121,  ///< file type group end

    MESSAGE_FRAME_ACK = 122,  ///< Frame has been received.

    MESSAGE_FILE_UPLOAD_CANCEL = 123,  ///< User has hit cancel on file upload dialog

    CMESSAGE_FILE_DRAG = 124,

    CMESSAGE_QUIT = 999,
} WhistClientMessageType;

/**
 * @brief   Integer exit code.
 * @details So the parent process of the protocol can receive the exit code.
 */
typedef enum WhistExitCode {
    WHIST_EXIT_SUCCESS = 0,
    WHIST_EXIT_FAILURE = 1,
    WHIST_EXIT_CLI = 2
} WhistExitCode;

typedef struct {
    short num_keycodes;
    bool caps_lock;
    bool num_lock;
    char state[KEYCODE_UPPERBOUND];
    bool active_pinch;
    WhistKeyboardLayout layout;
} WhistKeyboardState;

/**
 * Frame acknowledgement message.
 *
 * The client sends this to indicate that it has received the frame with
 * the given ID, and also all of its transitive dependencies.  If it is
 * lost there is little value in resending it because it will be
 * superseded by an ack of any later frame (since either that frame will
 * depend on this one and therefore imply its presence, or it won't and
 * this frame is useless).
 */
typedef struct {
    uint32_t frame_id;  ///< ID of frame we are acking.
} WhistFrameAckMessage;

/* position of bit within character */
#define BIT_CHAR(bit) ((bit) / CHAR_BIT)

/* array index for character containing bit */
#define BIT_IN_CHAR(bit) (1 << (CHAR_BIT - 1 - ((bit) % CHAR_BIT)))

/* number of characters required to contain number of bits */
#define BITS_TO_CHARS(bits) ((((bits)-1) / CHAR_BIT) + 1)
/**
 * @brief   Client message.
 * @details Message from a Whist client to a Whist server.
 */
typedef struct WhistClientMessage {
    WhistClientMessageType type;  ///< Input message type.
    unsigned int id;
    union {
        // MESSAGE_INIT
        ClientInitMessage init_message;

        // MESSAGE_KEYBOARD
        WhistKeyboardMessage keyboard;

        // MESSAGE_MOUSE_BUTTON
        WhistMouseButtonMessage mouseButton;

        // MESSAGE_MOUSE_WHEEL
        WhistMouseWheelMessage mouseWheel;

        // MESSAGE_MOUSE_MOTION
        WhistMouseMotionMessage mouseMotion;

        // MESSAGE_MULTIGESTURE
        WhistMultigestureMessage multigesture;  ///< Multigesture message.

        // MESSAGE_DIMENSIONS
        struct {
            int width;
            int height;
            int dpi;
        } dimensions;

        // MESSAGE_KEYBOARD_STATE
        WhistKeyboardState keyboard_state;

        // MESSAGE_FRAME_ACK
        WhistFrameAckMessage frame_ack;

        // CMESSAGE_END_FILE_GROUP
        FileGroupEnd file_group_end;
    };

    // Any type of message that has an additional `data[]` (or equivalent)
    //     member at the end should be a part of this union
    union {
        ClipboardData clipboard;     // CMESSAGE_CLIPBOARD
        FileMetadata file_metadata;  // CMESSAGE_FILE_METADATA
        FileData file;               // CMESSAGE_FILE_DATA
        WhistFileDragData file_drag_data;  // CMESSAGE_FILE_DRAG
        char urls_to_open[0];  // MESSAGE_OPEN_URL
    };
} WhistClientMessage;

/**
 * @brief   Server message type.
 * @details Type of message being sent from a Whist server to a Whist client.
 */
typedef enum WhistServerMessageType {
    SMESSAGE_NONE = 0,  ///< No Message
    MESSAGE_PONG = 1,
    MESSAGE_TCP_PONG = 2,
    SMESSAGE_INIT_REPLY = 3,
    SMESSAGE_CLIPBOARD = 4,
    SMESSAGE_WINDOW_TITLE = 5,
    SMESSAGE_OPEN_URI = 7,
    SMESSAGE_FULLSCREEN = 8,
    SMESSAGE_FILE_METADATA = 9,
    SMESSAGE_FILE_DATA = 10,
    SMESSAGE_FILE_GROUP_END = 11,
    SMESSAGE_NOTIFICATION = 12,
    SMESSAGE_INITIATE_UPLOAD = 13,
    SMESSAGE_FILE_DRAG = 14,
    SMESSAGE_QUIT = 100,
} WhistServerMessageType;

/**
 * @brief   Server message.
 * @details Message from a Whist server to a Whist client.
 */
typedef struct WhistServerMessage {
    WhistServerMessageType type;  ///< Input message type.
    union {
        int ping_id;
        int fullscreen;
    };
    union {
        ServerInitReplyMessage init_reply;
        ClipboardData clipboard;
        FileMetadata file_metadata;
        FileData file;
        FileGroupEnd file_group_end;
        char window_title[0];
        char requested_uri[0];
        WhistNotification notif;
    };
} WhistServerMessage;

/* @brief   Packet destination. (unused)
 * @details Host and port of a message destination.
 */
typedef struct WhistDestination {
    int host;
    int port;
} WhistDestination;

/* @brief   Bit array object.
 * @details Number of bits in the bitarray and bitarray in unsigned char format.
 */
typedef struct BitArray {
    unsigned char* array;  // pointer to array containing bits
    unsigned int numBits;  // number of bits in array
} BitArray;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize any and all static state
 *                                 that needs to be initialized
 *
 * @note                           This must be called after parsing arguments
 *
 * @note                           TODO: Make this function take in parsed arguments as a struct,
 *                                 rather than having parse_arguments manipulate `extern` globals
 */
void whist_init_subsystems(void);

/**
 * @brief                          Print the memory trace of a process
 */
void print_memory_info(void);

/**
 * @brief                          Print the system info of the computer
 */
void print_system_info(void);

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
 * @brief                          Reads a 16-byte hexidecimal string and copies
 *                                 it into private_key
 *
 * @param hex_string               The hexidecimal string to copy
 * @param binary_private_key       The 16-byte buffer to copy the bytes into
 * @param hex_private_key          The 33-byte buffer to fill with a hex string
 *                                 representation of the private key.
 *
 * @returns                        True if hex_string was a 16-byte hexadecimal
 *                                 value, otherwise false
 */
bool read_hexadecimal_private_key(const char* hex_string, char* binary_private_key,
                                  char* hex_private_key);

/**
 * @brief                          Calculate the size of a WhistClientMessage
 *                                 struct
 *
 * @param wcmsg                     The Whist Client Message to find the size
 *
 * @returns                        The size of the Whist Client Message struct
 */
int get_wcmsg_size(WhistClientMessage* wcmsg);

/**
 * @brief                          Terminates the protocol
 */
NORETURN void terminate_protocol(WhistExitCode exit_code);

/**
 * @brief                          Safely copy a string from source to destination.
 *
 * @details                        Copies at most `num` bytes from `source` to `destination`. Bytes
 *                                 after the first null character of `source` are not copied.
 *                                 If no null character is encountered within the first `num` bytes
 *                                 of `source`, `destination[num - 1]` will be manually set to zero,
 *                                 so `destination` is guaranteed to be null terminated, unless
 *                                 `num` is zero, in which case the `destination` is left unchanged.
 *
 * @param destination              Address to copy to. Should have at least `num` bytes available.
 *
 * @param source                   Address to copy from.
 *
 * @param num                      Number of bytes to copy.
 *
 * @returns                        True if all bytes of source were copied (i.e. strlen(source) <=
 *                                 num - 1)
 */
bool safe_strncpy(char* destination, const char* source, size_t num);

/**
 * @brief                          This will exit on error, and log a warning if this
 *                                 function is used when the logger is already initialized.
 *                                 If the logger is initialized, please use LOG_INFO.
 *
 * @param fmt                      The format string
 * @param ...                      The rest of the arguments
 */
#define safe_printf UNINITIALIZED_LOG

// All bit array functions below are acknowledged to https://github.com/MichaelDipperstein/bitarray
/**
 * @brief                          Allocate a BitArray object
 *
 * @details                        This function allocates a bit array large enough to
 *                                 contain the specified number of bits.  The contents of the
 *                                 array are not initialized.
 *
 * @param bits                     The number of bits in the array to be allocated.
 *
 * @returns                        A pointer to allocated bit array or NULL if array may not
 *                                 be allocated.  errno will be set in the event that the
 *                                 array may not be allocated.
 */
BitArray* bit_array_create(const unsigned int bits);

/**
 * @brief                          This function frees the memory allocated for a bit array.
 *
 * @param ba                       The pointer to bit array to be freed
 */
void bit_array_free(BitArray* ba);

/**
 * @brief                          This function sets every bit to 0 in the bit array passed
 *                                 as a parameter.
 *
 * @param ba                       The pointer to bit array
 */
void bit_array_clear_all(const BitArray* const ba);

/**
 * @brief                          This function takes a null-terminated string and modifies
 *                                 it to remove any dangling unicode characters that may have
 *                                 been left over from a previous truncation. This is necessary
 *                                 because dangling unicode characters will cause horrible crashes
 *                                 if they are not removed.
 *
 * @param str                      The utf8 string to be trimmed.
 */
void trim_utf8_string(char* str);
/**
 * @brief                          This function sets every bit to 1 in the bit array passed
 *                                 as a parameter.
 *
 * @details                        Each of the bits used in the bit array are set to 1.
 *                                 Unused (spare) bits are set to 0. This is function uses
 *                                 UCHAR_MAX, so it is crucial that the machine implementation
 *                                 of unsigned char utilizes all the bits in the memory allocated
 *                                 for an unsigned char.
 *
 * @param ba                       The pointer to bit array
 */
void bit_array_set_all(const BitArray* const ba);

/**
 * @brief                          This function sets the specified bit to 1 in the bit array
 *                                 passed as a parameter.
 *
 * @param ba                       The pointer to the bit array
 *
 * @param bit                      The index of the bit to set
 *
 * @returns                        0 for success, -1 for failure.  errno will be set in the
 *                                 event of a failure.
 */
int bit_array_set_bit(const BitArray* const ba, const unsigned int bit);

/**
 * @brief                          This function returns a pointer to the array of unsigned
 *                                 char containing actual bits.
 *
 * @param ba                       The pointer to bit array
 *
 * @returns                        A pointer to array containing bits
 */
void* bit_array_get_bits(const BitArray* const ba);

/**
 * @brief                          This function tests the specified bit in the bit array
 *                                 passed as a parameter. A non-zero will be returned if the
 *                                 tested bit is set.
 *
 * @param ba                       The pointer to bit array
 *
 * @param bit                      The index of the bit to set
 *
 * @returns                        Non-zero if bit is set, otherwise 0.  This function does
 *                                 not check the input.  Tests on invalid input will produce
 *                                 unknown results.
 */
int bit_array_test_bit(const BitArray* const ba, const unsigned int bit);

/**
 * @brief                          calculate a/b, roundup
 *
 * @param a                        The dividend
 *
 * @param b                        The divisor
 *
 * @returns                        a/b, roundup
 */
int int_div_roundup(int a, int b);

/**
 * @brief                          Returns a short string representing the current git commit
 *                                 of whist at the time of compilation
 *
 * @returns                        The git commit
 */
const char* whist_git_revision(void);

// TODO: Resolve circular references
#include "whist_frame.h"

#endif  // WHIST_H
