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
int32_t multithreaded_nvidia_device_manager(void* opaque);
int unbind_fbc_cuda_context(CaptureDevice* device);
int bind_fbc_cuda_context(CaptureDevice* device);

/*
============================
Private Function Implementations
============================
*/
int32_t multithreaded_nvidia_device_manager(void* opaque) {
    /*
        Multithreaded function to asynchronously destroy and create the nvidia capture device when
       necessary. nvidia_device_semaphore will be posted to if capture_screen on nvidia fails,
       indicating a need to recreate the capture device, or when the whole device is being torn
       down. In the former case, the thread will keep attempting to create a new nvidia device until
       successful. In the latter case, the thread will exit.

        Arguments:
            opaque (void*): pointer to the capture device

        Returns:
            (int): 0 on exit
    */
    CaptureDevice* device = (CaptureDevice*)opaque;
    /*
    CUcontext nvidia_manager_cuda_context = NULL;
    cuda_init(&nvidia_manager_cuda_context);
    */

    while (true) {
        fractal_wait_semaphore(device->nvidia_device_semaphore);
        if (device->pending_destruction) {
            break;
        }
        // set current context
        CUresult cu_res = cu_ctx_push_current_ptr(*get_nvidia_thread_cuda_context_ptr());
        if (cu_res != CUDA_SUCCESS) {
            LOG_ERROR("Failed to push current context, status %d!");
        }
        // Nvidia requires recreation
        while (device->nvidia_capture_device == NULL) {
            LOG_INFO("Creating nvidia capture device...");
            device->nvidia_capture_device = create_nvidia_capture_device();
            fractal_sleep(500);
        }
        LOG_INFO("Created nvidia capture device!");
        cu_res = cu_ctx_pop_current_ptr(get_video_thread_cuda_context_ptr());
        nvidia_release_context(device->nvidia_capture_device);
        // Tell the main thread to bind the nvidia context again
        device->nvidia_context_is_stale = true;
        // Tell the main thread nvidia is active again
        device->active_capture_device = NVIDIA_DEVICE;
    }
    return 0;
}

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

    char cmd[1024];

    // Update the device's width/height
    device->width = width;
    device->height = height;

    // If the device's width/height must be updated
    if (!is_same_wh(device)) {
        char modename[128];

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

        free(display_name);

        // If it's still not the correct dimensions
        if (!is_same_wh(device)) {
            LOG_ERROR("Could not force monitor to a given width/height. Tried to set to %dx%d");
            // Get the width/height that the device actually is though
            get_wh(device, &device->width, &device->height);
        }
    }

    // This script must be built in to the Mandelbox. It writes new DPI for X11 and
    // AwesomeWM, and uses SIGHUP to XSettingsd to trigger application and window
    // manager refreshes to use the new DPI.
    static uint32_t last_set_dpi = 96;
    if (dpi != last_set_dpi) {
        snprintf(cmd, sizeof(cmd), "/usr/share/fractal/update-fractal-dpi.sh %d", dpi);
        runcmd(cmd, NULL);
        last_set_dpi = dpi;
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
    memset(device, 0, sizeof(CaptureDevice));

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
    // cuda_init two contexts
    bool res = cuda_init(get_video_thread_cuda_context_ptr());
    if (!res) {
        LOG_ERROR("Failed to initialize cuda!");
    }
    res = cuda_init(get_nvidia_thread_cuda_context_ptr());
    if (!res) {
        LOG_ERROR("Failed to initialize second cuda context!");
    }
    // pop the second context, since it will belong to nvidia
    CUresult cu_res = cu_ctx_pop_current_ptr(get_nvidia_thread_cuda_context_ptr());
    if (!res) {
        LOG_ERROR("Failed to initialize cuda!");
    }
    LOG_DEBUG("Nvidia context: %x, Video context: %x", *get_nvidia_thread_cuda_context_ptr(), *get_video_thread_cuda_context_ptr());
    // set up semaphore and nvidia manager
    device->nvidia_device_semaphore = fractal_create_semaphore(0);
    device->nvidia_manager = fractal_create_thread(multithreaded_nvidia_device_manager,
                                                   "multithreaded_nvidia_manager", device);
    fractal_post_semaphore(device->nvidia_device_semaphore);
#endif

    // Create the X11 capture device; when the nvidia manager thread finishes creation, active
    // capture device will change
    device->active_capture_device = X11_DEVICE;
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
            static CUcontext current_context;
            // first check if we just switched to nvidia
            if (device->nvidia_context_is_stale) {
                // the nvidia device's context should be in video_context_ptr
                CUresult cu_res = cu_ctx_pop_current_ptr(get_nvidia_thread_cuda_context_ptr());
                if (cu_res != CUDA_SUCCESS) {
                    LOG_ERROR("Failed to pop video thread context into nvidia thread context");
                }
                cu_res = cu_ctx_push_current_ptr(*get_video_thread_cuda_context_ptr());
                if (cu_res != CUDA_SUCCESS) {
                    LOG_ERROR("Failed to swap contexts!");
                }
                nvidia_bind_context(device->nvidia_capture_device);
                device->nvidia_context_is_stale = false;
            }
            int ret = nvidia_capture_screen(device->nvidia_capture_device);
            // if (ret > 0) LOG_INFO("Capturing with Nvidia!");
            if (ret >= 0) {
                if (device->width == device->nvidia_capture_device->width &&
                    device->height == device->nvidia_capture_device->height) {
                    device->last_capture_device = NVIDIA_DEVICE;
                    device->frame_data = device->nvidia_capture_device->p_gpu_texture;
                    return ret;
                } else {
                    LOG_ERROR(
                        "Capture Device is configured for dimensions %dx%d, which "
                        "does not match nvidia's captured dimensions of %dx%d!",
                        device->width, device->height, device->nvidia_capture_device->width,
                        device->nvidia_capture_device->height);
                }
            }
            // otherwise, nvidia failed!
            device->active_capture_device = X11_DEVICE;
        }
        case X11_DEVICE:
            device->last_capture_device = X11_DEVICE;
            // LOG_INFO("X11 capture!");
            int ret = x11_capture_screen(device->x11_capture_device);
            if (ret >= 0) {
                device->frame_data = device->x11_capture_device->frame_data;
            }
            return ret;
        default:
            LOG_FATAL("Unknown capture device type: %d", device->active_capture_device);
            return -1;
    }
}

bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height,
                                uint32_t dpi) {
    /*
       Attempt to reconfigure the capture device to the given width, height, and dpi. In
       Nvidia case, we resize the display and wait for the Nvidia device to tell us we must recreate
       it. In X11 case, we don't update in place.

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
    try_update_dimensions(device, width, height, dpi);
    if (device->active_capture_device == NVIDIA_DEVICE) {
        // destroy the device
        destroy_nvidia_capture_device(device->nvidia_capture_device);
        device->active_capture_device = X11_DEVICE;
        fractal_post_semaphore(device->nvidia_device_semaphore);
    }
    return reconfigure_x11_capture_device(device->x11_capture_device, width, height, dpi);
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
    // tell the nvidia thread to stop
    device->pending_destruction = true;
    fractal_post_semaphore(device->nvidia_device_semaphore);
    // wait for the nvidia thread to terminate
    fractal_wait_thread(device->nvidia_manager, NULL);
    // now we can destroy the capture device
    if (device->nvidia_capture_device) {
        destroy_nvidia_capture_device(device->nvidia_capture_device);
    }
    cuda_destroy(*get_video_thread_cuda_context_ptr());
    *get_video_thread_cuda_context_ptr() = NULL;
    cuda_destroy(*get_nvidia_thread_cuda_context_ptr());
    *get_nvidia_thread_cuda_context_ptr() = NULL;
    if (device->x11_capture_device) {
        destroy_x11_capture_device(device->x11_capture_device);
    }
    XCloseDisplay(device->display);
}

int transfer_screen(CaptureDevice* device) {
    device->frame_data = device->x11_capture_device->frame_data;
    device->pitch = device->x11_capture_device->pitch;
    return 0;
}
