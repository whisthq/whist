#ifndef UINPUT_INPUT_DRIVER_H
#define UINPUT_INPUT_DRIVER_H
/**
 * Copyright 2022 Whist Technologies, Inc.
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

#include <whist/core/whist.h>

/*
============================
Custom Types
============================
*/

typedef struct InputDevice {
    int fd_absmouse;
    int fd_relmouse;
    int fd_keyboard;
    int keyboard_state[KEYCODE_UPPERBOUND];
    bool caps_lock;
    bool num_lock;
    // Internal
    bool mouse_has_moved;
} InputDevice;

#endif  // UINPUT_INPUT_DRIVER_H
