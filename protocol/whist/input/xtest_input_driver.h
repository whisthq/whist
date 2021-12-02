#ifndef XTEST_INPUT_DRIVER_H
#define XTEST_INPUT_DRIVER_H
/**
 * Copyright 2021 Whist Technologies, Inc.
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

#include <whist/core/whist.h>

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
