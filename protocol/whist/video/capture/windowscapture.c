/*
 * GPU screen capture on Windows.
 *
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 **/
#include "capture.h"
#include <whist/logging/logging.h>

int create_capture_device(CaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi) {
    LOG_WARNING("Not implemented for Windows servers!");
    return -1;
}

bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height,
                                uint32_t dpi) {
    LOG_WARNING("Not implemented for Windows servers!");
    return false;
}

int capture_screen(CaptureDevice* device) {
    LOG_WARNING("Not implemented for Windows servers!");
    return -1;
}

int transfer_screen(CaptureDevice* device) {
    LOG_WARNING("Not implemented for Windows servers!");
    return -1;
}

void destroy_capture_device(CaptureDevice* device) {
    LOG_WARNING("Not implemented for Windows servers!");
}
