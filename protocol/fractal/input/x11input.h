#ifndef LINUX_INPUT_H
#define LINUX_INPUT_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file x11input.h
 * @brief This file defines the X11 input device type.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"
// #include "../utils/sdlscreeninfo.h"

#include <X11/Xlib.h>

/*
============================
Custom Types
============================
*/

typedef struct input_device_t {
    Display* display;
} input_device_t;

#endif  // LINUX_INPUT_H
