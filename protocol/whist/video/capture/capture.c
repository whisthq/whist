/**
 *
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 **/
#include <whist/logging/logging.h>

#include "capture.h"

#if 0
int create_capture_device(CaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi) {
    LOG_WARNING("Not implemented for Windows servers!");
    return -1;
}

bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi) {
    return device->impl->reconfigure(device->impl, width, height, dpi);
}

int capture_screen(CaptureDevice* device) {
    return device->impl->capture_screen(device->impl);
}

int transfer_screen(CaptureDevice* device) {
    return device->impl->transfer_screen(device->impl);
}


void destroy_capture_device(CaptureDevice* device) {
}
#endif

int capture_get_data(CaptureDevice* device, void** buf, uint32_t* stride) {
	return device->impl->capture_get_data(device->impl, buf, stride);
}
