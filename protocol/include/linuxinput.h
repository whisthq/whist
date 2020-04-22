/*
 * User input processing on Linux Ubuntu.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef LINUX_INPUT_H
#define LINUX_INPUT_H

#include <fcntl.h>
#include <linux/uinput.h>

#define _FRACTAL_IOCTL_TRY(FD, PARAMS...)                                    \
    if (ioctl(FD, PARAMS) == -1) {                                           \
        mprintf("Failure at setting " #PARAMS " on fd " #FD ". Error: %s\n", \
                strerror(errno));                                            \
        return NULL;                                                         \
    }

#include "fractal.h"

typedef struct input_device_t {
    int fd_absmouse;
    int fd_relmouse;
    int fd_keyboard;
} input_device_t;

#endif // LINUX_INPUT_H