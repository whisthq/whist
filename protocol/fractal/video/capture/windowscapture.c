/*
 * GPU screen capture on Windows.
 *
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 **/
#include "capture.h"
#include <fractal/logging/logging.h>

int create_capture_device(CaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi) {
    LOG_FATAL("Not implemented for Windows servers!");
    return -1;
}

bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height,
                                uint32_t dpi) {
    LOG_FATAL("Not implemented for Windows servers!");
    return false;
}

int capture_screen(CaptureDevice* device) {
    LOG_FATAL("Not implemented for Windows servers!");
    return -1;
}

int transfer_screen(CaptureDevice* device) {
    LOG_FATAL("Not implemented for Windows servers!");
    return -1;
}

void destroy_capture_device(CaptureDevice* device) {
    LOG_FATAL("Not implemented for Windows servers!");
}
