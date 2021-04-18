#ifndef UINPUT_INPUT_DRIVER_H
#define UINPUT_INPUT_DRIVER_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file uinput_input_driver.h
 * @brief This file defines the uinput input device type.
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

/*
============================
Custom Types
============================
*/

typedef struct InputDevice {
    int fd_absmouse;
    int fd_relmouse;
    int fd_keyboard;
    int keyboard_state[NUM_KEYCODES];
    bool caps_lock;
    bool num_lock;
    // Internal
    bool mouse_has_moved;
} InputDevice;

#endif  // UINPUT_INPUT_DRIVER_H
