/**
 * Copyright Fractal Computers, Inc. 2021
 * @file x11capture.c
 * @brief This file contains the code to create a capture device and use it to capture the screen on
Linux.
============================
Usage
============================
We first try to create a capture device that uses Nvidia's FBC SDK for capturing frames. This
capture device must be paired with an Nvidia encoder. If those fail, we fall back to using X11's API
to create a capture device, which captures on the CPU, and encode using FFmpeg instead. See
videoencode.h for more details on encoding. The type of capture device currently in use is indicated
in active_capture_device.
*/

/*
============================
Includes
============================
*/
#include "linuxcapture.h"
#include "x11capture.h"
#include "nvidiacapture.h"

/*
============================
Private Functions
============================
*/
void get_wh(CaptureDevice* device, int* w, int* h);
bool is_same_wh(CaptureDevice* device);
void try_update_dimensions(CaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi);

/*
============================
Private Function Implementations
============================
*/
void get_wh(CaptureDevice* device, int* w, int* h) {
    /*
        Get the width and height of the display associated with device, and store them in w and h,
        respectively.

        Arguments:
            device (CaptureDevice*): device containing the display whose dimensions we are getting
            w (int*): pointer to store width
            h (int*): pointer to store hight
    */
    if (!device) return;

    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(device->display, device->root, &window_attributes)) {
        *w = 0;
        *h = 0;
        LOG_ERROR("Error while getting window attributes");
        return;
    }
    *w = window_attributes.width;
    *h = window_attributes.height;
}

bool is_same_wh(CaptureDevice* device) {
    /*
        Determine whether or not the device's width and height agree width actual display width and
        height.

        Arguments:
            device (CaptureDevice*): capture device to query

        Returns:
            (bool): true if width and height agree, false otherwise
    */

    int w, h;
    get_wh(device, &w, &h);
    return device->width == w && device->height == h;
}

void try_update_dimensions(CaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi) {
    /*
       Using XRandR, try updating the device's display to the given width, height, and DPI. Even if
       this fails to set the dimensions, device->width and device->height will always equal the
       actual dimensions of the screen.

        Arguments:
            device (CaptureDevice*): pointer to the device whose window we're resizing
            width (uint32_t): desired width
            height (uint32_t): desired height
            dpi (uint32_t): desired DPI
    */
    // TODO: Implement DPI changes

    // Update the device's width/height
    device->width = width;
    device->height = height;
    // If the device's width/height is already correct, just return
    if (is_same_wh(device)) {
        return;
    }

    char modename[128];
    char cmd[1024];

    snprintf(modename, sizeof(modename), "Fractal-%dx%d", width, height);

    char* display_name;
    runcmd("xrandr --current | grep \" connected\"", &display_name);
    *strchr(display_name, ' ') = '\0';

    snprintf(cmd, sizeof(cmd), "xrandr --delmode %s %s", display_name, modename);
    runcmd(cmd, NULL);
    snprintf(cmd, sizeof(cmd), "xrandr --rmmode %s", modename);
    runcmd(cmd, NULL);
    snprintf(cmd, sizeof(cmd),
             "xrandr --newmode %s $(cvt -r %d %d 60 | sed -n \"2p\" | "
             "cut -d' ' -f3-)",
             modename, width, height);
    runcmd(cmd, NULL);
    snprintf(cmd, sizeof(cmd), "xrandr --addmode %s %s", display_name, modename);
    runcmd(cmd, NULL);
    snprintf(cmd, sizeof(cmd), "xrandr --output %s --mode %s", display_name, modename);
    runcmd(cmd, NULL);

    // For the single-monitor case, we no longer need to do the below; we instead
    // assume the display server has been created with the correct DPI. The below
    // approach does not handle this case well because Linux does not yet support
    // changing DPI on the fly. For getting seamless performance on multi-monitor
    // setups, we may eventually need to instead get already-running X11
    // applications to respect DPI changes to the X server.

    // I believe this command sets the DPI, as checked by `xdpyinfo | grep resolution`
    // snprintf(cmd, sizeof(cmd), "xrandr --dpi %d", dpi);
    // runcmd(cmd, NULL);
    // while this command sets the font DPI setting
    // snprintf(cmd, sizeof(cmd), "echo Xft.dpi: %d | xrdb -merge", dpi);
    // runcmd(cmd, NULL);

    free(display_name);

    // If it's still not the correct dimensions
    if (!is_same_wh(device)) {
        LOG_ERROR("Could not force monitor to a given width/height. Tried to set to %dx%d");
        // Get the width/height that the device actually is though
        get_wh(device, &device->width, &device->height);
    }
}

/*
============================
Public Function Implementations
============================
*/
int create_capture_device(CaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi) {
    /*
       Initialize the capture device at device with the given width, height and DPI. We use Nvidia
       whenever possible, and fall back to X11 when not. See nvidiacapture.c and x11capture.c for
       internal details of each capture device.

       Arguments:
            device (CaptureDevice*): pointer to the device to initialize
            width (uint32_t): desired device width
            height (uint32_t): desired device height
            dpi (uint32_t): desired device DPI

        Returns:
            (int): 0 on success, -1 on failure
    */
    // make sure we can create the device
    if (device == NULL) {
        LOG_ERROR("NULL device was passed into create_capture_device");
        return -1;
    }

    // resize the X11 display to the appropriate width and height
    if (width <= 0 || height <= 0) {
        LOG_ERROR("Invalid width/height of %d/%d", width, height);
        return -1;
    }
    if (width < MIN_SCREEN_WIDTH) {
        LOG_ERROR("Requested width too small: %d when the minimum is %d! Rounding up.", width,
                  MIN_SCREEN_WIDTH);
        width = MIN_SCREEN_WIDTH;
    }
    if (height < MIN_SCREEN_HEIGHT) {
        LOG_ERROR("Requested height too small: %d when the minimum is %d! Rounding up.", height,
                  MIN_SCREEN_HEIGHT);
        height = MIN_SCREEN_HEIGHT;
    }
    if (width > MAX_SCREEN_WIDTH || height > MAX_SCREEN_HEIGHT) {
        LOG_ERROR(
            "Requested dimensions are too large! "
            "%dx%d when the maximum is %dx%d! Rounding down.",
            width, height, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
        width = MAX_SCREEN_WIDTH;
        height = MAX_SCREEN_HEIGHT;
    }

    // attempt to set display width, height, and DPI
    device->display = XOpenDisplay(NULL);
    if (!device->display) {
        LOG_ERROR("ERROR: CreateCaptureDevice display did not open");
        return -1;
    }
    device->root = DefaultRootWindow(device->display);

    try_update_dimensions(device, width, height, dpi);

    // if we can create the nvidia capture device, do so

#if USING_NVIDIA_CAPTURE_AND_ENCODE
    device->nvidia_capture_device = create_nvidia_capture_device();
    if (device->nvidia_capture_device) {
        device->active_capture_device = NVIDIA_DEVICE;
        LOG_INFO("Using Nvidia Capture SDK!");
        // TODO: also create the x11 device now in case nvidia fails
        return 0;
    } else {
        device->active_capture_device = X11_DEVICE;
        LOG_ERROR("USING_NVIDIA_CAPTURE_AND_ENCODE defined but unable to use Nvidia Capture SDK!");
    }
#else
    device->active_capture_device = X11_DEVICE;
#endif
    // Create the X11 capture devicek
    device->x11_capture_device = create_x11_capture_device(width, height, dpi);
    if (device->x11_capture_device) {
#if !USING_SHM
        device->x11_capture_device->image = NULL;
        if (capture_screen(device) < 0) {
            LOG_ERROR("Failed to call capture_screen for the first frame!");
            destroy_capture_device(device);
            return -1;
        }
#endif
        return 0;
    } else {
        LOG_ERROR("Failed to create X11 capture device!");
        return -1;
    }
}

int capture_screen(CaptureDevice* device) {
    /*
        Capture the screen that device is attached to. If using Nvidia, since we can't specify what
       display Nvidia should be using, we need to confirm that the Nvidia device's dimensions match
       up with device's dimensions.

        Arguments:
            device (CaptureDevice*): pointer to device we are using for capturing

        Returns:
            (int): 0 on success, -1 on failure
    */
    if (!device) {
        LOG_ERROR("Tried to call capture_screen with a NULL CaptureDevice! We shouldn't do this!");
        return -1;
    }
    switch (device->active_capture_device) {
        case NVIDIA_DEVICE: {
            int ret = nvidia_capture_screen(device->nvidia_capture_device);
            if (ret < 0) {
                LOG_ERROR("nvidia_capture_screen failed!");
                // TODO: look into falling back to X11
                return -1;
            } else {
                if (device->width != device->nvidia_capture_device->width ||
                    device->height != device->nvidia_capture_device->height) {
                    LOG_ERROR(
                        "Capture Device dimensions %dx%d does not match nvidia dimensions of "
                        "%dx%d!",
                        device->width, device->height, device->nvidia_capture_device->width,
                        device->nvidia_capture_device->height);
                    return -1;
                }
                return ret;
            }
        }
        case X11_DEVICE:
            return x11_capture_screen(device->x11_capture_device);
        default:
            LOG_ERROR("Unknown capture device type: %d", device->active_capture_device);
            return -1;
    }
}

bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height,
                                uint32_t dpi) {
    /*
        TODO: this function will be superseded by a function that will handle both recreation and
       updating.

       Attempt to reconfigure the capture device to the given width, height, and dpi. In
       Nvidia case, we resize the display and wait for the Nvidia device to internally update
       (possibly through recreation). In X11 case, we don't update in place.

        Arguments:
            device (CaptureDevice*): pointer to device we want to reconfigure
            width (uint32_t): new device width
            height (uint32_t): new device height
            dpi (uint32_t): new device DPI

        Returns:
            (bool): true if successful, false on failure
    */
    if (device == NULL) {
        LOG_ERROR("NULL device was passed into reconfigure_capture_device!");
        return false;
    }
    switch (device->active_capture_device) {
        case NVIDIA_DEVICE:
            try_update_dimensions(device, width, height, dpi);
            return true;
        case X11_DEVICE:
            return false;
        default:
            LOG_ERROR("Unknown capture device type: %d", device->active_capture_device);
            return false;
    }
}

void destroy_capture_device(CaptureDevice* device) {
    /*
        Destroy device by freeing its contents and the pointer itself.

        Arguments:
            device (CaptureDevice*): device to destroy
    */
    if (device == NULL) {
        // nothing to do!
        return;
    }
    if (device->nvidia_capture_device) {
        destroy_nvidia_capture_device(device->nvidia_capture_device);
    }
    if (device->x11_capture_device) {
        destroy_x11_capture_device(device->x11_capture_device);
    }
    XCloseDisplay(device->display);
}

int transfer_screen(CaptureDevice* device) { return 0; }
