#ifndef LINUX_INPUT_H
#define LINUX_INPUT_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file linuxinput.h
 * @brief This file defines the Linux input device type.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include <fcntl.h>
#include <linux/uinput.h>

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
} input_device_t;

#endif  // LINUX_INPUT_H
