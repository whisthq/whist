#ifndef XTEST_INPUT_DRIVER_H
#define XTEST_INPUT_DRIVER_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file xtest_input_driver.h
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

#include <fractal/core/fractal.h>

#include <X11/Xlib.h>

/*
============================
Custom Types
============================
*/

typedef struct InputDevice {
    Display* display;
    Window root;
    int keyboard_state[KEYCODE_UPPERBOUND];
    bool caps_lock;
    bool num_lock;
} InputDevice;

#endif  // XTEST_INPUT_DRIVER_H
