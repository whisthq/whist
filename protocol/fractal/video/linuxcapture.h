#ifndef LINUX_CAPTURE_H
#define LINUX_CAPTURE_H
#include <X11/Xlib.h>
#include <fractal/core/fractal.h>
#include "nvidiacapture.h"
#include "x11capture.h"

typedef enum CaptureDeviceType {
    NVIDIA_DEVICE,
    X11_DEVICE
} CaptureDeviceType;

typedef struct CaptureDevice {
    CaptureDeviceType active_capture_device;
    // TODO: put the next four elements in some kind of resize context
    int width;
    int height;
    Display* display;
    Window root;
    NvidiaCaptureDevice* nvidia_capture_device;
    X11CaptureDevice* x11_capture_device;
} CaptureDevice;

#endif
