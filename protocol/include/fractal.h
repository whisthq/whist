/*
 * This file contains the headers of the main functions used as part of the
 * streaming protocol.

 Protocol version: 1.0
 Last modification: 12/10/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef FRACTAL_H
#define FRACTAL_H

#include <stdint.h>
#include <stdbool.h>

#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavdevice/avdevice.h"
#include "ffmpeg/libavfilter/avfilter.h"
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavutil/avutil.h"
#include "ffmpeg/libavfilter/buffersink.h"
#include "ffmpeg/libavfilter/buffersrc.h"
#include "ffmpeg/libswscale/swscale.h"
#include "ffmpeg/libavutil/hwcontext.h"
#include "ffmpeg/libavutil/hwcontext_qsv.h"

#define SDL_MAIN_HANDLED
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

#if defined(_WIN32)
	#include <initguid.h>
	#include <mmdeviceapi.h>
	#include <Audioclient.h>
	#include <Functiondiscoverykeys_devpkey.h>
	#include <avrt.h>
	#include <windows.h>
	#include <winuser.h>
	#include <D3D11.h>
	#include <D3d11_1.h> 
	#include <dxgi1_2.h>
	#include <DXGITYPE.h>
	#pragma warning(disable: 4201)
#else
	#define SOCKET int
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

/*** DEFINITIONS START ***/

#define STUN_SERVER_IP "3.233.46.94"
#define PORT 48800 

#define SERVER_IP "52.186.125.178"
#define MAX_PAYLOAD_SIZE 1400
#define START_MAX_MBPS 300.0
#define ACK_REFRESH_MS 50

#define LARGEST_FRAME_SIZE 1000000
#define STARTING_BITRATE 5000
#define OUTPUT_WIDTH 1920
#define OUTPUT_HEIGHT 1080

/// @brief Default ports configurations to pass to FractalInit
/// @details Picked from unassigned range from IANA.org
/// Increment by one if these are taken, until a port is found available
#define FRACTAL_DEFAULTS {  							 \
  /* upnp          					 */ 1,         \
	/* clientPort RECV (UDP)   */ 48800,     \
  /* clientPort SEND (TCP)   */ 48900,     \
  /* serverPort RECV (TCP)   */ 48900,     \
	/* serverPort SEND (UDP)   */ 48800,     \
}

/// @brief Default server settings for streaming
#define FRACTAL_SERVER_DEFAULTS {    \
  /* resolutionX (pixels)   */ 0,    \
  /* resolutionY (pixels)   */ 0,    \
  /* refreshRate (fps)      */ 60,   \
  /* encoderFPS  (fps)      */ 0,    \
  /* encoderMaxBitrate      */ 10,   \
  /* encoderH265            */ 0,    \
}

/// @brief Default client settings for receiving stream
#define FRACTAL_CLIENT_DEFAULTS {    \
  /* SoftwareDecoder        */ 0,    \
  /* resolutionX (pixels)   */ 0,    \
  /* resolutionY (pixels)   */ 0,    \
  /* refreshRate (fps)      */ 60,   \
  /* audioBuffer            */ 6,    \
}

/*** DEFINITIONS END ***/

/*** ENUMERATIONS START ***/

/// @brief Status codes indicating success, warning, or error.
/// @details Returned by most Fractal functions. FRACTAL_OK is '0',
///	warnings are positive, errors are negative.
typedef enum FractalStatus {
	FRACTAL_OK                = 0,       ///< 0

	DECODE_WRN_CONTINUE       = 1000,    ///< 1000
	DECODE_WRN_ACCEPTED       = 1001,    ///< 1001
	DECODE_WRN_REINIT         = 1003,    ///< 1003

	NETWORK_WRN_TIMEOUT       = 2000,    ///< 2000

	AUDIO_WRN_NO_DATA         = 6000,    ///< 6000

	ERR_DEFAULT               = -1,      ///< -1

	DECODE_ERR_INIT           = -10,     ///< -10
	DECODE_ERR_LOAD           = -11,     ///< -11
	DECODE_ERR_MAP            = -13,     ///< -13
	DECODE_ERR_DECODE         = -14,     ///< -14
	DECODE_ERR_CLEANUP        = -15,     ///< -15
	DECODE_ERR_PARSE          = -16,     ///< -16
	DECODE_ERR_NO_SUPPORT     = -17,     ///< -17
	DECODE_ERR_PIXEL_FORMAT   = -18,     ///< -18
	DECODE_ERR_BUFFER         = -19,     ///< -19
	DECODE_ERR_RESOLUTION     = -20,     ///< -20

	WS_ERR_CONNECT            = -6101,   ///< -6101
	WS_ERR_POLL               = -3001,   ///< -3001
	WS_ERR_READ               = -3002,   ///< -3002
	WS_ERR_WRITE              = -3003,   ///< -3003
	WS_ERR_CLOSE              = -6105,   ///< -6105
	WS_ERR_PING               = -3005,   ///< -3005
	WS_ERR_PONG_TIMEOUT       = -3006,   ///< -3006
	WS_ERR_PONG               = -3007,   ///< -3007
	WS_ERR_AUTH               = -3008,   ///< -3008
	WS_ERR_GOING_AWAY         = -3009,   ///< -3009

	NAT_ERR_PEER_PHASE        = -6023,   ///< -6023
	NAT_ERR_STUN_PHASE        = -6024,   ///< -6024
	NAT_ERR_NO_CANDIDATES     = -6033,   ///< -6033
	NAT_ERR_JSON_ACTION       = -6111,   ///< -6111
	NAT_ERR_NO_SOCKET         = -6112,   ///< -6112

	OPENGL_ERR_CONTEXT        = -7000,   ///< -7000
	OPENGL_ERR_SHARE          = -7001,   ///< -7001
	OPENGL_ERR_PIXFORMAT      = -7002,   ///< -7002
	OPENGL_ERR_CURRENT        = -7003,   ///< -7003
	OPENGL_ERR_DC             = -7004,   ///< -7004
	OPENGL_ERR_SHADER         = -7005,   ///< -7005
	OPENGL_ERR_PROGRAM        = -7006,   ///< -7006
	OPENGL_ERR_VERSION        = -7007,   ///< -7007
	OPENGL_ERR_TEXTURE        = -7008,   ///< -7008

	AUDIO_ERR_CAPTURE_INIT    = -9000,   ///< -9000
	AUDIO_ERR_CAPTURE         = -9001,   ///< -9001
	AUDIO_ERR_NETWORK         = -9002,   ///< -9002
	AUDIO_ERR_FREE            = -9003,   ///< -9003

	AUDIO_OPUS_ERR_INIT       = -10000,  ///< -10000
	AUDIO_OPUS_ERR_DECODE     = -10001,  ///< -10001
	AUDIO_OPUS_ERR_ENCODE     = -10002,  ///< -10002

	NETWORK_ERR_BG_TIMEOUT    = -12007,  ///< -12007
	NETWORK_ERR_BAD_PACKET    = -12008,  ///< -12008
	NETWORK_ERR_BUFFER        = -12011,  ///< -12011
	NETWORK_ERR_SHUTDOWN      = -12017,  ///< -12017
	NETWORK_ERR_UNSUPPORTED   = -12018,  ///< -12018
	NETWORK_ERR_INTERRUPTED   = -12019,  ///< -12019

	SERVER_ERR_DISPLAY        = -13000,  ///< -13000
	SERVER_ERR_RESOLUTION     = -13008,  ///< -13008
	SERVER_ERR_MAX_RESOLUTION = -13009,  ///< -13009
	SERVER_ERR_NO_USER        = -13011,  ///< -13011
	SERVER_ERR_VIDEO_DONE     = -13013,  ///< -13013
	SERVER_ERR_CLIENT_ABORT   = -13014,  ///< -13014
	SERVER_ERR_CLIENT_GONE    = -13015,  ///< -13015

	CAPTURE_ERR_INIT          = -14003,  ///< -14003
	CAPTURE_ERR_TEXTURE       = -14004,  ///< -14004
	CAPTURE_ERR_DESTROY				= -14005,  ///< -14005

	ENCODE_ERR_INIT           = -15000,  ///< -15000
	ENCODE_ERR_ENCODE         = -15002,  ///< -15002
	ENCODE_ERR_BUFFER         = -15006,  ///< -15006
	ENCODE_ERR_PROPERTIES     = -15100,  ///< -15100
	ENCODE_ERR_LIBRARY        = -15101,  ///< -15101
	ENCODE_ERR_SESSION        = -15007,  ///< -15007
	ENCODE_ERR_SESSION1       = -15103,  ///< -15103
	ENCODE_ERR_SESSION2       = -15104,  ///< -15104
	ENCODE_ERR_OUTPUT_INIT    = -15105,  ///< -15105
	ENCODE_ERR_TEXTURE        = -15106,  ///< -15106
	ENCODE_ERR_OUTPUT         = -15107,  ///< -15107
	ENCODE_ERR_UNSUPPORTED    = -15108,  ///< -15108
	ENCODE_ERR_HANDLE         = -15109,  ///< -15109
	ENCODE_ERR_CAPS           = -15110,  ///< -15110

	UPNP_ERR                  = -19000,  ///< -19000

	D3D_ERR_TEXTURE           = -22000,  ///< -22000
	D3D_ERR_SHADER            = -22001,  ///< -22001
	D3D_ERR_BUFFER            = -22002,  ///< -22002
	D3D_ERR_LAYOUT            = -22003,  ///< -22003
	D3D_ERR_DEVICE            = -22004,  ///< -22004
	D3D_ERR_MT                = -22005,  ///< -22005
	D3D_ERR_ADAPTER           = -22006,  ///< -22006
	D3D_ERR_FACTORY           = -22007,  ///< -22007
	D3D_ERR_OUTPUT            = -22008,  ///< -22008
	D3D_ERR_CONTEXT           = -22009,  ///< -22009
	D3D_ERR_OUTPUT1           = -22010,  ///< -22010
	D3D_ERR_SWAP_CHAIN        = -22011,  ///< -22011
	D3D_ERR_DRAW              = -22012,  ///< -22012
	D3D_ERR_OUTPUT5           = -22013,  ///< -22013

	H26X_ERR_NOT_FOUND        = -23000,  ///< -23000

	AES_GCM_ERR_KEY_LEN       = -28000,  ///< -28000
	AES_GCM_ERR_BUFFER        = -28004,  ///< -28004

	SCTP_ERR_GLOBAL_INIT      = -32000,  ///< -32000
	SCTP_ERR_WRITE            = -32001,  ///< -32001
	SCTP_ERR_SOCKET           = -32002,  ///< -32002
	SCTP_ERR_BIND             = -32003,  ///< -32003
	SCTP_ERR_CONNECT          = -32004,  ///< -32004

	DTLS_ERR_BIO_WRITE        = -33000,  ///< -33000
	DTLS_ERR_BIO_READ         = -33001,  ///< -33001
	DTLS_ERR_SSL              = -33002,  ///< -33002
	DTLS_ERR_BUFFER           = -33003,  ///< -33003
	DTLS_ERR_NO_DATA          = -33004,  ///< -33004
	DTLS_ERR_CERT             = -33005,  ///< -33005

	STUN_ERR_PACKET           = -34000,  ///< -34000
	STUN_ERR_PARSE_HEADER     = -34001,  ///< -34001
	STUN_ERR_PARSE_ADDRESS    = -34002,  ///< -34002

	SO_ERR_OPEN               = -35000,  ///< -35000
	SO_ERR_SYMBOL             = -35001,  ///< -35001

	RESAMPLE_ERR_INIT         = -37000,  ///< -37000
	RESAMPLE_ERR_RESAMPLE     = -37001,  ///< -37001

	OPENSSL_ERR               = -600000, ///< `SSL_get_error` value will be subtracted from this value.

	#if defined(_WIN32)
	SOCKET_ERR                = -700000, ///< `WSAGetLastError` value will be subtracted from this value.
	#else
	SOCKET_ERR                = -800000, ///< `errno` value will be subtracted from this value.
	#endif

	__ERR_MAKE_32             = 0x7FFFFFFF,
} FractalStatus;

typedef enum EncodeType {
	SOFTWARE_ENCODE        = 0,
	NVENC_ENCODE           = 1
} EncodeType;

typedef enum DecodeType {
	SOFTWARE_DECODE        = 0,
	QSV_DECODE             = 1
} DecodeType;

/// @brief Keyboard input.
/// @details Integer code for each of the user keyboard inputs.
typedef enum FractalKeycode {
	KEY_A           = 4,   ///< 4
	KEY_B           = 5,   ///< 5
	KEY_C           = 6,   ///< 6
	KEY_D           = 7,   ///< 7
	KEY_E           = 8,   ///< 8
	KEY_F           = 9,   ///< 9
	KEY_G           = 10,  ///< 10
	KEY_H           = 11,  ///< 11
	KEY_I           = 12,  ///< 12
	KEY_J           = 13,  ///< 13
	KEY_K           = 14,  ///< 14
	KEY_L           = 15,  ///< 15
	KEY_M           = 16,  ///< 16
	KEY_N           = 17,  ///< 17
	KEY_O           = 18,  ///< 18
	KEY_P           = 19,  ///< 19
	KEY_Q           = 20,  ///< 20
	KEY_R           = 21,  ///< 21
	KEY_S           = 22,  ///< 22
	KEY_T           = 23,  ///< 23
	KEY_U           = 24,  ///< 24
	KEY_V           = 25,  ///< 25
	KEY_W           = 26,  ///< 26
	KEY_X           = 27,  ///< 27
	KEY_Y           = 28,  ///< 28
	KEY_Z           = 29,  ///< 29
	KEY_1           = 30,  ///< 30
	KEY_2           = 31,  ///< 31
	KEY_3           = 32,  ///< 32
	KEY_4           = 33,  ///< 33
	KEY_5           = 34,  ///< 34
	KEY_6           = 35,  ///< 35
	KEY_7           = 36,  ///< 36
	KEY_8           = 37,  ///< 37
	KEY_9           = 38,  ///< 38
	KEY_0           = 39,  ///< 39
	KEY_ENTER       = 40,  ///< 40
	KEY_ESCAPE      = 41,  ///< 41
	KEY_BACKSPACE   = 42,  ///< 42
	KEY_TAB         = 43,  ///< 43
	KEY_SPACE       = 44,  ///< 44
	KEY_MINUS       = 45,  ///< 45
	KEY_EQUALS      = 46,  ///< 46
	KEY_LBRACKET    = 47,  ///< 47
	KEY_RBRACKET    = 48,  ///< 48
	KEY_BACKSLASH   = 49,  ///< 49
	KEY_SEMICOLON   = 51,  ///< 51
	KEY_APOSTROPHE  = 52,  ///< 52
	KEY_BACKTICK    = 53,  ///< 53
	KEY_COMMA       = 54,  ///< 54
	KEY_PERIOD      = 55,  ///< 55
	KEY_SLASH       = 56,  ///< 56
	KEY_CAPSLOCK    = 57,  ///< 57
	KEY_F1          = 58,  ///< 58
	KEY_F2          = 59,  ///< 59
	KEY_F3          = 60,  ///< 60
	KEY_F4          = 61,  ///< 61
	KEY_F5          = 62,  ///< 62
	KEY_F6          = 63,  ///< 63
	KEY_F7          = 64,  ///< 64
	KEY_F8          = 65,  ///< 65
	KEY_F9          = 66,  ///< 66
	KEY_F10         = 67,  ///< 67
	KEY_F11         = 68,  ///< 68
	KEY_F12         = 69,  ///< 69
	KEY_PRINTSCREEN = 70,  ///< 70
	KEY_SCROLLLOCK  = 71,  ///< 71
	KEY_PAUSE       = 72,  ///< 72
	KEY_INSERT      = 73,  ///< 73
	KEY_HOME        = 74,  ///< 74
	KEY_PAGEUP      = 75,  ///< 75
	KEY_DELETE      = 76,  ///< 76
	KEY_END         = 77,  ///< 77
	KEY_PAGEDOWN    = 78,  ///< 78
	KEY_RIGHT       = 79,  ///< 79
	KEY_LEFT        = 80,  ///< 80
	KEY_DOWN        = 81,  ///< 81
	KEY_UP          = 82,  ///< 82
	KEY_NUMLOCK     = 83,  ///< 83
	KEY_KP_DIVIDE   = 84,  ///< 84
	KEY_KP_MULTIPLY = 85,  ///< 85
	KEY_KP_MINUS    = 86,  ///< 86
	KEY_KP_PLUS     = 87,  ///< 87
	KEY_KP_ENTER    = 88,  ///< 88
	KEY_KP_1        = 89,  ///< 89
	KEY_KP_2        = 90,  ///< 90
	KEY_KP_3        = 91,  ///< 91
	KEY_KP_4        = 92,  ///< 92
	KEY_KP_5        = 93,  ///< 93
	KEY_KP_6        = 94,  ///< 94
	KEY_KP_7        = 95,  ///< 95
	KEY_KP_8        = 96,  ///< 96
	KEY_KP_9        = 97,  ///< 97
	KEY_KP_0        = 98,  ///< 98
	KEY_KP_PERIOD   = 99,  ///< 99
	KEY_APPLICATION = 101, ///< 101
	KEY_F13         = 104, ///< 104
	KEY_F14         = 105, ///< 105
	KEY_F15         = 106, ///< 106
	KEY_F16         = 107, ///< 107
	KEY_F17         = 108, ///< 108
	KEY_F18         = 109, ///< 109
	KEY_F19         = 110, ///< 110
	KEY_MENU        = 118, ///< 118
	KEY_MUTE        = 127, ///< 127
	KEY_VOLUMEUP    = 128, ///< 128
	KEY_VOLUMEDOWN  = 129, ///< 129
	KEY_LCTRL       = 224, ///< 224
	KEY_LSHIFT      = 225, ///< 225
	KEY_LALT        = 226, ///< 226
	KEY_LGUI        = 227, ///< 227
	KEY_RCTRL       = 228, ///< 228
	KEY_RSHIFT      = 229, ///< 229
	KEY_RALT        = 230, ///< 230
	KEY_RGUI        = 231, ///< 231
	KEY_AUDIONEXT   = 258, ///< 258
	KEY_AUDIOPREV   = 259, ///< 259
	KEY_AUDIOSTOP   = 260, ///< 260
	KEY_AUDIOPLAY   = 261, ///< 261
	KEY_AUDIOMUTE   = 262, ///< 262
	KEY_MEDIASELECT = 263, ///< 263
	__KEY_MAKE_32   = 0x7FFFFFFF,
} FractalKeycode;

/// @brief Modifier keys applied to keyboard input.
/// @details Codes for when keyboard input is modified. These values may be bitwise OR'd together.
typedef enum FractalKeymod {
	MOD_NONE      = 0x0000, ///< No modifier key active.
	MOD_LSHIFT    = 0x0001, ///< `LEFT SHIFT` is currently active.
	MOD_RSHIFT    = 0x0002, ///< `RIGHT SHIFT` is currently active.
	MOD_LCTRL     = 0x0040, ///< `LEFT CONTROL` is currently active.
	MOD_RCTRL     = 0x0080, ///< `RIGHT CONTROL` is currently active.
	MOD_LALT      = 0x0100, ///< `LEFT ALT` is currently active.
	MOD_RALT      = 0x0200, ///< `RIGHT ALT` is currently active.
	MOD_NUM	      = 0x1000, ///< `NUMLOCK` is currently active.
	MOD_CAPS      = 0x2000, ///< `CAPSLOCK` is currently active.
	__MOD_MAKE_32 = 0x7FFFFFFF,
} FractalKeymod;

/// @brief Mouse button.
/// @details Codes for encoding mouse actions.
typedef enum FractalMouseButton {
	MOUSE_L         = 1, ///< Left mouse button.
	MOUSE_MIDDLE    = 2, ///< Middle mouse button.
	MOUSE_R         = 3, ///< Right mouse button.
	MOUSE_X1        = 4, ///< Extra mouse button 1.
	MOUSE_X2        = 5, ///< Extra mouse button 2.
	__MOUSE_MAKE_32 = 0x7FFFFFFF,
} FractalMouseButton;

/// @brief Color formats for raw image data.
/// @details Used to encode/decode images.
typedef enum FractalColorFormat {
	FORMAT_UNKNOWN   = 0,
	FORMAT_NV12      = 1, ///< 4:2:0 full width/height Y plane followed by an interleaved half width/height UV plane.
	FORMAT_I420      = 2, ///< 4:2:0 full width/height Y plane followed by a half width/height U plane followed by a half width/height V plane.
	FORMAT_NV16      = 3, ///< 4:2:2 full width/height Y plane followed by an interleaved half width full height UV plane.
	FORMAT_I422      = 4, ///< 4:2:2 full width/height Y plane followed by a half width full height U plane followed by a half width full height V plane.
	FORMAT_BGRA      = 5, ///< 32-bits per pixel, 8-bits per channel BGRA.
	FORMAT_RGBA      = 6, ///< 32-bits per pixel, 8-bits per channel RGBA.
	__FORMAT_MAKE_32 = 0x7FFFFFFF,
} FractalColorFormat;

/// @brief Network protocol used for peer-to-peer connections.
/// @details Two modes depending on whther this is web or native
typedef enum FractalProtocol {
	PROTO_MODE_UDP       = 1, ///< Fractal's low-latency optimized UDP protocol.
	PROTO_MODE_SCTP      = 2, ///< SCTP protocol compatible with WebRTC data channels.
	__PROTO_MODE_MAKE_32 = 0x7FFFFFFF,
} FractalProtocol;

/// @brief Video stream container.
/// @details Used for the client configuration
typedef enum FractalContainer {
	CONTAINER_FRACTAL    = 0, ///< Fractal's custom container compatible with native decoding.
	CONTAINER_MP4       = 2, ///< MP4 box container compatible with web browser Media Source Extensions.
	__CONTAINER_MAKE_32 = 0x7FFFFFFF,
} FractalContainer;

/// @brief PCM audio format.
/// @details Passed to audio submission on host.
typedef enum FractalPCMFormat {
	PCM_FORMAT_FLOAT     = 1, ///< 32-bit floating point samples.
	PCM_FORMAT_INT16     = 2, ///< 16-bit signed integer samples.
	__PCM_FORMAT_MAKE_32 = 0x7FFFFFFF,
} FractalPCMFormat;

/*** ENUMERATIONS END ***/

/*** STRUCTS START ***/

/// @brief Fractal instance configuration.
/// @details Passed to FractalInit to generate config
/// serve as the first port used when the `bind` call is made internally. If the port is already in use,
/// the next port will be tried until an open port has been found or 50 attempts have been made.
typedef struct FractalConfig {
	int32_t upnp;       		///< `1` enables and maintains UPnP to assist NAT traversal, `0` disables it.
	int32_t clientPortRECV; ///< First port tried for client connections. A value of `0` uses a pseudo random default.
	int32_t clientPortSEND; ///< First port tried for client connections. A value of `0` uses a pseudo random default.
	int32_t serverPortRECV; ///< First port used to accept host connections. A value of `0` uses a pseudo random default.
	int32_t serverPortSEND; ///< First port used to accept host connections. A value of `0` uses a pseudo random default.
} FractalConfig;

/// @brief Video frame properties.
/// @details Used for rendering frames
typedef struct FractalFrame {
	FractalColorFormat format; ///< Color format.
	uint32_t size;            ///< Size in bytes of the `image` buffer parameter of FrameCallback
	uint32_t width;           ///< Width in pixels of the visible area of the frame.
	uint32_t height;          ///< Height in pixels of the visible area of the frame.
	uint32_t fullWidth;       ///< Actual width of the frame including padding.
	uint32_t fullHeight;      ///< Actual height of the frame including padding.
} FractalFrame;

/// @brief Cursor properties.
/// @details Track important information on cursor.
typedef struct FractalCursor {
	uint32_t size;      ///< Size in bytes of the cursor image buffer.
	uint32_t positionX; ///< When leaving relative mode, the horizontal position in screen coordinates where the cursor reappears.
	uint32_t positionY; ///< When leaving relative mode, the vertical position in screen coordinates where the cursor reappears.
	uint16_t width;     ///< Width of the cursor image in pixels.
	uint16_t height;    ///< Height of the cursor position in pixels.
	uint16_t hotX;      ///< Horizontal pixel position of the cursor hotspot within the image.
	uint16_t hotY;      ///< Vertical pixel position of the cursor hotspot within the image.
	bool modeUpdate;    ///< `true` if the cursor mode should be updated. The `relative`, `positionX`, and `positionY` members are valid.
	bool imageUpdate;   ///< `true` if the cursor image should be updated. The `width`, `height`, `hotX`, `hotY`, and `size` members are valid.
	bool relative;      ///< `true` if in relative mode, meaning the client should submit mouse motion in relative distances rather than absolute screen coordinates.
	uint8_t __pad[1];
} FractalCursor;

/// @brief Latency performance metrics.
/// @details Latency metrics for the client
typedef struct FractalMetrics {
	float encodeLatency;  ///< Average time in milliseconds for the host to encode a frame.
	float decodeLatency;  ///< Average time in milliseconds for the client to decode a frame.
	float networkLatency; ///< Average round trip time between the client and host.
} FractalMetrics;

/// @brief Keyboard message.
/// @details Messages related to keyboard usage.
typedef struct FractalKeyboardMessage {
	FractalKeycode code;  ///< Keyboard input.
	FractalKeymod mod;    ///< Stateful modifier keys applied to keyboard input.
	bool pressed;        ///< `true` if pressed, `false` if released.
	uint8_t __pad[3];
} FractalKeyboardMessage;

/// @brief Mouse button message.
/// @details Message from mouse button.
typedef struct FractalMouseButtonMessage {
	FractalMouseButton button; ///< Mouse button.
	bool pressed;             ///< `true` if clicked, `false` if released.
	uint8_t __pad[3];
} FractalMouseButtonMessage;

/// @brief Mouse wheel message.
/// @details Message from mouse wheel.
typedef struct FractalMouseWheelMessage {
	int32_t x; ///< Horizontal delta of mouse wheel rotation. Negative values scroll left.
	int32_t y; ///< Vertical delta of mouse wheel rotation. Negative values scroll up.
} FractalMouseWheelMessage;

/// @brief Mouse motion message.
/// @details Member of FractalMessage. Mouse motion can be sent in either relative or absolute mode via
/// the `relative` member. Absolute mode treats the `x` and `y` values as the exact destination for where
/// the cursor will appear. These values are sent from the client in device screen coordinates and are translated
/// in accordance with the values set via FractalClientSetDimensions. Relative mode `x` and `y` values are not
/// affected by FractalClientSetDimensions and move the cursor with a signed delta value from its previous location.
typedef struct FractalMouseMotionMessage {
	int32_t x;     ///< The absolute horizontal screen coordinate of the cursor  if `relative` is `false`, or the delta (can be negative) if `relative` is `true`.
	int32_t y;     ///< The absolute vertical screen coordinate of the cursor if `relative` is `false`, or the delta (can be negative) if `relative` is `true`.
	bool relative; ///< `true` for relative mode, `false` for absolute mode. See details.
	uint8_t __pad[3];
} FractalMouseMotionMessage;

/// @brief Client configuration.
/// @details Passed to FractalClientConnect. Regarding `resolutionX`, `resolutionY`, and `refreshRate`:
/// These settings apply only in HOST_DESKTOP if the client is the first client to connect, and that client is
/// the owner of the computer. Setting `resolutionX` or `resolutionY` to `0` will leave the host resolution unaffected,
/// otherwise the host will attempt to find the closest matching resolution / refresh rate.
typedef struct FractalClientConfig {
	int32_t decoderSoftware; ///< `true` to force decoding of video frames via a software implementation.
	int32_t mediaContainer;  ///< ::FractalContainer value.
	int32_t protocol;        ///< ::FractalProtocol value.
	int32_t resolutionX;     ///< See details.
	int32_t resolutionY;     ///< See details.
	int32_t refreshRate;     ///< See details.
	uint32_t audioBuffer;    ///< Audio buffer in 20ms packets, i.e. a setting of `6` would be a 120ms buffer. When audio received exceeds this buffer, the client will fast forward the audio to the size of the buffer divided by `2`.
	bool pngCursor;          ///< `true` to return compressed PNG cursor images during FractalClientPollEvents, `false` to return a 32-bit RGBA image.
	uint8_t __pad[3];
} FractalClientConfig;

/// @brief Cursor mode/image update event.
/// @details Member of FractalClientEvent.
typedef struct FractalClientCursorEvent {
	FractalCursor cursor; ///< Cursor properties.
	uint32_t key;         ///< Buffer lookup key passed to FractalGetBuffer to retrieve the cursor image, if available.
} FractalClientCursorEvent;

typedef enum FractalPacketType {
	PACKET_AUDIO,
	PACKET_VIDEO,
	PACKET_MESSAGE,
} FractalPacketType;

typedef enum FractalClientMessageType {
	CMESSAGE_NONE           = 0, ///< No Message
	MESSAGE_KEYBOARD        = 1, ///< `keyboard` FractalKeyboardMessage is valid in FractClientMessage.
	MESSAGE_MOUSE_BUTTON    = 2, ///< `mouseButton` FractalMouseButtonMessage is valid in FractClientMessage.
	MESSAGE_MOUSE_WHEEL     = 3, ///< `mouseWheel` FractalMouseWheelMessage is valid in FractClientMessage.
	MESSAGE_MOUSE_MOTION    = 4, ///< `mouseMotion` FractalMouseMotionMessage is valid in FractClientMessage.
	MESSAGE_RELEASE         = 5, ///< Message instructing the host to release all input that is currently pressed.
	MESSAGE_MBPS            = 6, ///< `mbps` double is valid in FractClientMessage.
	MESSAGE_PING            = 7,
	MESSAGE_DIMENSIONS      = 8, ///< `dimensions.width` int and `dimensions.height` int is valid in FractClientMessage
	MESSAGE_VIDEO_NACK      = 9,
	MESSAGE_AUDIO_NACK      = 10,
	MESSAGE_QUIT = 100,
} FractalClientMessageType;

typedef struct FractalClientMessage {
	FractalClientMessageType type;                     ///< Input message type.
	union {
		FractalKeyboardMessage keyboard;           ///< Keyboard message.
		FractalMouseButtonMessage mouseButton;     ///< Mouse button message.
		FractalMouseWheelMessage mouseWheel;       ///< Mouse wheel message.
		FractalMouseMotionMessage mouseMotion;     ///< Mouse motion message.
		double mbps;
		int ping_id;
		struct dimensions {
			int width;
			int height;
		} dimensions;
		struct nack_data {
			int id;
			int index;
		} nack_data;
	};
} FractalClientMessage;


typedef enum FractalServerMessageType {
	SMESSAGE_NONE = 0, ///< No Message
	MESSAGE_PONG = 1,
} FractalServerMessageType;

typedef struct FractalServerMessage {
	FractalServerMessageType type;                     ///< Input message type.
	union {
		int ping_id;
	};
} FractalServerMessage;

typedef struct FractalDestination
{
    int host;
    short port;
} FractalDestination;

typedef struct SocketContext
{
    SOCKET s;
    struct sockaddr_in addr;
    int ack;
} SocketContext;

// Real Packet Size = sizeof(RTPPacket) - sizeof(RTPPacket.data) + RTPPacket.payload_size
struct RTPPacket {
	// hash at the beginning of the struct, which is the hash of the rest of the packet
	uint32_t hash;
	FractalPacketType type;
	int index;
	int payload_size;
	int id;
	bool is_ending;
	// data at the end of the struct, in the case of a truncated packet
	uint8_t data[MAX_PAYLOAD_SIZE];
};

#define MAX_PACKET_SIZE (sizeof(struct RTPPacket))

typedef struct Frame {
	int width;
	int height;
	int size;
	unsigned char compressed_frame[];
} Frame;

/*** STRUCTS END ***/

/*** FRACTAL FUNCTIONS START ***/

/*
/// @brief destroy the server sockets and threads, and WSA for windows
/// @details if full=true, destroys everything, else only current connection
FractalStatus ServerDestroy(SOCKET sockets[], HANDLE threads[], bool full);

/// @brief initialize the listen socket (TCP path)
/// @details initializes windows socket, creates and binds our listen socket
SOCKET ServerInit(SOCKET listensocket, FractalConfig config);
*/

/// @brief replays a user action taken on the client and sent to the server
/// @details parses the FractalMessage struct and send input to Windows OS
FractalStatus ReplayUserInput(struct FractalClientMessage fmsg[6], int len);

int CreateUDPContext(struct SocketContext* context, char* origin, char* destination, int recvfrom_timeout_s, int stun_timeout_ms);

int SendAck(struct SocketContext *context, int reps);

#if defined(_WIN32)
	#define clock LARGE_INTEGER
#else
	#define clock int
#endif

void initMultiThreadedPrintf();
void destroyMultiThreadedPrintf();
void mprintf(const char* fmtStr, ...);

void StartTimer(clock* timer);
double GetTimer(clock timer);

uint32_t Hash(void* key, size_t len);

/*** FRACTAL FUNCTIONS END ***/

// renable Windows warning
#if defined(_WIN32)
	#pragma warning(default: 4201)
#endif

#endif // FRACTAL_H
