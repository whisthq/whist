import os
import time
import uinput

BUS_USB = 0x03

# absmouse = uinput.Device([uinput.ABS_X, uinput.ABS_Y])
relmouse = uinput.Device([
    uinput.REL_X,
    uinput.REL_Y,
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT,
    uinput.REL_WHEEL,
    uinput.REL_HWHEEL
], name="Fractal Virtual Relative Input", bustype=BUS_USB, vendor=0xf4c1, product=0x1123, version=0x1)

os.system('clear')
print(f"pid: {os.getpid()}")
# print(f"absmouse file descriptor: {absmouse._Device__uinput_fd}")
print(f"relmouse file descriptor: {relmouse._Device__uinput_fd}")

while True:
    time.sleep(1)
    relmouse.emit_click(uinput.BTN_LEFT)
    print("clicking")


