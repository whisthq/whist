# Client-Side Protocol

Contains the code for the Raspbian client side, which receives the encoded RTSP stream, decodes it from H.264 using OpenMaxIL and displays it using DispmanX on the Raspberry Pi 4B.

The following is the README for the Raspberry Pi decode+display code:
----------------
This repository contains the source code for the ARM side libraries used on Raspberry Pi.
These typically are installed in /opt/vc/lib and includes source for the ARM side code to interface to:
EGL, mmal, GLESv2, vcos, openmaxil, vchiq_arm, bcm_host, WFC, OpenVG.

Use buildme to build. It is a modified version of userland-master that should build the hello_video.bin demo in build/bin. It requires cmake to be installed and an ARM cross compiler. For 32-bit cross compilation it is set up to use this one:
https://github.com/raspberrypi/tools/tree/master/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian

Whilst 64-bit userspace is not officially supported, some of the libraries will work for it. To cross compile, install gcc-aarch64-linux-gnu and g++-aarch64-linux-gnu first. For both native and cross compiles, add the option ```--aarch64``` to the buildme command.

Note that this repository does not contain the source for the edidparser and vcdbg binaries due to licensing restrictions.

You can then run the progam by going to build/bin and running ./hello_video.bin [filename.h264]
----------------

