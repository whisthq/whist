# Fractal Protocol

This repository contains the source code for the Fractal Protocol, which streams audio and video at ultra-low latency from a virtual machine, OS container or regular computer, and streams back user actions and files.

### Style

For .c and .h files, we are formatting using the clang format `{BasedOnStyle: Google, IndentWidth: 4}`. If using VSCode or Visual Studio, please set this up in your editor to format on save if possible (in Visual Studio, this is through the C/C++ extension settings, as well as the general 'Format on Save' setting/shortcut). Otherwise, please make sure to run your code through `clang` before commits.
We also have a custom build tagret in the CMake 'clang-format' with will run with this style over all .c and .h files in server/ desktop/ and fractal/
------------

### Server

This folder contains the code to run Fractal servers that are streamed to any of the clients listed below. The currently-supported servers are:

-   Windows
-   Linux Ubuntu

### Desktop

This folder contains the code to run Fractal clients on desktop operating systems. The currently-supported servers are:

-   Windows
-   MacOS
-   Linux

### Android/Chromebook

TBD.

### iOS/iPadOS

TBD.

### Web

TBD.
