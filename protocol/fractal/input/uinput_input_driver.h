#ifndef UINPUT_INPUT_DRIVER_H
#define UINPUT_INPUT_DRIVER_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file uinput_input_driver_h.h
 * @brief This file defines the Linux uinput kernel module input device type.
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
#include "../utils/sdlscreeninfo.h"

/*
============================
Custom Types
============================
*/

typedef struct input_device_t {
    int fd_absmouse;
    int fd_relmouse;
    int fd_keyboard;
    int keyboard_state[NUM_KEYCODES];
    bool caps_lock;
    bool num_lock;
} input_device_t;

#endif  // UINPUT_INPUT_DRIVER_H
