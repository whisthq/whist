#ifndef LINUX_INPUT_H
#define LINUX_INPUT_H

#include <fcntl.h>
#include <linux/uinput.h>

#include "fractal.h"

typedef struct input_device {
    int fd;
    struct uinput_setup* usetup;
} input_device;

#endif  // LINUX_INPUT_H