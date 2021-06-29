import os
import time
import uinput

BUS_USB = 0x03

# hotfix uinput to include new hi res scrolling
uinput.REL_WHEEL_HI_RES = (0x02, 0x0B)
uinput.REL_HWHEEL_HI_RES = (0x02, 0x0C)

# absmouse = uinput.Device([uinput.ABS_X, uinput.ABS_Y])
relmouse = uinput.Device(
    [
        uinput.REL_X,
        uinput.REL_Y,
        uinput.BTN_LEFT,
        uinput.BTN_RIGHT,
        uinput.REL_WHEEL,
        uinput.REL_HWHEEL,
        uinput.REL_WHEEL_HI_RES,
        uinput.REL_HWHEEL_HI_RES,
    ],
    name="Fractal Virtual Relative Input",
    bustype=BUS_USB,
    vendor=0xF4C1,
    product=0x1123,
    version=0x1,
)

keycodes = [
    uinput.KEY_A,
    uinput.KEY_B,
    uinput.KEY_C,
    uinput.KEY_D,
    uinput.KEY_E,
    uinput.KEY_F,
    uinput.KEY_G,
    uinput.KEY_H,
    uinput.KEY_I,
    uinput.KEY_J,
    uinput.KEY_K,
    uinput.KEY_L,
    uinput.KEY_M,
    uinput.KEY_N,
    uinput.KEY_O,
    uinput.KEY_P,
    uinput.KEY_Q,
    uinput.KEY_R,
    uinput.KEY_S,
    uinput.KEY_T,
    uinput.KEY_U,
    uinput.KEY_V,
    uinput.KEY_W,
    uinput.KEY_X,
    uinput.KEY_Y,
    uinput.KEY_Z,
    uinput.KEY_1,
    uinput.KEY_2,
    uinput.KEY_3,
    uinput.KEY_4,
    uinput.KEY_5,
    uinput.KEY_6,
    uinput.KEY_7,
    uinput.KEY_8,
    uinput.KEY_9,
    uinput.KEY_0,
    uinput.KEY_ENTER,
    uinput.KEY_ESC,
    uinput.KEY_BACKSPACE,
    uinput.KEY_TAB,
    uinput.KEY_SPACE,
    uinput.KEY_MINUS,
    uinput.KEY_EQUAL,
    uinput.KEY_LEFTBRACE,
    uinput.KEY_RIGHTBRACE,
    uinput.KEY_BACKSLASH,
    uinput.KEY_SEMICOLON,
    uinput.KEY_APOSTROPHE,
    uinput.KEY_GRAVE,
    uinput.KEY_COMMA,
    uinput.KEY_DOT,
    uinput.KEY_SLASH,
    uinput.KEY_CAPSLOCK,
    uinput.KEY_F1,
    uinput.KEY_F2,
    uinput.KEY_F3,
    uinput.KEY_F4,
    uinput.KEY_F5,
    uinput.KEY_F6,
    uinput.KEY_F7,
    uinput.KEY_F8,
    uinput.KEY_F9,
    uinput.KEY_F10,
    uinput.KEY_F11,
    uinput.KEY_F12,
    uinput.KEY_SYSRQ,
    uinput.KEY_SCROLLLOCK,
    uinput.KEY_PAUSE,
    uinput.KEY_INSERT,
    uinput.KEY_HOME,
    uinput.KEY_PAGEUP,
    uinput.KEY_DELETE,
    uinput.KEY_END,
    uinput.KEY_PAGEDOWN,
    uinput.KEY_RIGHT,
    uinput.KEY_LEFT,
    uinput.KEY_DOWN,
    uinput.KEY_UP,
    uinput.KEY_NUMLOCK,
    uinput.KEY_KPSLASH,
    uinput.KEY_KPASTERISK,
    uinput.KEY_KPMINUS,
    uinput.KEY_KPPLUS,
    uinput.KEY_KPENTER,
    uinput.KEY_KP1,
    uinput.KEY_KP2,
    uinput.KEY_KP3,
    uinput.KEY_KP4,
    uinput.KEY_KP5,
    uinput.KEY_KP6,
    uinput.KEY_KP7,
    uinput.KEY_KP8,
    uinput.KEY_KP9,
    uinput.KEY_KP0,
    uinput.KEY_KPDOT,
    uinput.KEY_COMPOSE,
    uinput.KEY_F13,
    uinput.KEY_F14,
    uinput.KEY_F15,
    uinput.KEY_F16,
    uinput.KEY_F17,
    uinput.KEY_F18,
    uinput.KEY_F19,
    uinput.KEY_F20,
    uinput.KEY_F21,
    uinput.KEY_F22,
    uinput.KEY_F23,
    uinput.KEY_F24,
    uinput.KEY_HELP,
    uinput.KEY_MENU,
    uinput.KEY_SELECT,
    uinput.KEY_MUTE,
    uinput.KEY_VOLUMEUP,
    uinput.KEY_VOLUMEDOWN,
    uinput.KEY_LEFTCTRL,
    uinput.KEY_LEFTSHIFT,
    uinput.KEY_LEFTALT,
    uinput.KEY_LEFTMETA,
    uinput.KEY_RIGHTCTRL,
    uinput.KEY_RIGHTSHIFT,
    uinput.KEY_RIGHTALT,
    uinput.KEY_RIGHTMETA,
    uinput.KEY_MODE,
    uinput.KEY_NEXTSONG,
    uinput.KEY_PREVIOUSSONG,
    uinput.KEY_STOPCD,
    uinput.KEY_PLAYPAUSE,
    uinput.KEY_MUTE,
    uinput.KEY_SELECT,
]

keyboard = uinput.Device(
    keycodes,
    name="Fractal Virtual Keyboard",
    bustype=BUS_USB,
    vendor=0xF4C1,
    product=0x1122,
    version=0x1,
)

os.system("clear")
print(f"pid: {os.getpid()}")
# print(f"absmouse file descriptor: {absmouse._Device__uinput_fd}") # pylint: disable=protected-access, line-too-long
print(
    f"relmouse file descriptor: {relmouse._Device__uinput_fd}"
)  # pylint: disable=protected-access, line-too-long
print(
    f"keyboard file descriptor: {keyboard._Device__uinput_fd}"
)  # pylint: disable=protected-access, line-too-long

while True:
    # time.sleep(1)
    # relmouse.emit_click(uinput.BTN_LEFT)
    # print("clicking")
    # time.sleep(1)
    # keyboard.emit_click(uinput.KEY_A)
    # print("typing A")
    time.sleep(1)
    relmouse.emit(uinput.REL_WHEEL_HI_RES, 30)
    print("scrolling")

# import socket
# SOCKET_PATH = "/tmp/uinput.sock"

# if os.path.exists(SOCKET_PATH):
#     os.remove(SOCKET_PATH)

# server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
# server.bind(SOCKET_PATH)
# server.listen(1)

# print(f"listening at {SOCKET_PATH}")
# while True:
#     print(f"waiting for connection")
#     conn, addr = server.accept()
#     try:
#         while True:
#             data = conn.recv(1024)
#             if data:
#                 print("data received:")
#                 print(data)
#             else:
#                 print("end of stream")
#                 break
#     finally:
#         conn.close()
