/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file x11capture.c
 * @brief This file contains the code to create a capture device and use it to capture the screen on
Linux.
============================
Usage
============================
We first try to create a capture device that uses Nvidia's FBC SDK for capturing frames. This
capture device must be paired with an Nvidia encoder. If those fail, we fall back to using X11's API
to create a capture device, which captures on the CPU, and encode using FFmpeg instead. See
codec/encode.h for more details on encoding. The type of capture device currently in use is
indicated in active_capture_device.
*/

/*
============================
Includes
============================
*/
#include "capture.h"
#include "x11capture.h"
#include "nvidiacapture.h"
#include "filecapture.h"
#include "westoncapture.h"

#include <whist/utils/window_info.h>



/*
============================
Private Functions
============================
*/
//static int32_t multithreaded_nvidia_device_manager(void* opaque);

/*
============================
Private Function Implementations
============================
*/
#if 0
static int32_t multithreaded_nvidia_device_manager(void* opaque) {
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

    while (true) {
        whist_wait_semaphore(device->nvidia_device_semaphore);
        if (device->pending_destruction) {
            break;
        }
        // set current context
        CUresult cu_res = cu_ctx_push_current_ptr(*get_nvidia_thread_cuda_context_ptr());
        if (cu_res != CUDA_SUCCESS) {
            LOG_ERROR("Failed to push current context, status %d!", cu_res);
        }
        cu_ctx_synchronize_ptr();
        // Nvidia requires recreation
        while (device->nvidia_capture_device == NULL) {
            LOG_INFO("Creating nvidia capture device...");
            device->nvidia_capture_device = create_nvidia_capture_device();
            if (device->nvidia_capture_device == NULL) {
                whist_sleep(500);
            }
        }
        LOG_INFO("Created nvidia capture device!");
        LOG_DEBUG("device handle: %" PRIu64, device->nvidia_capture_device->fbc_handle);
        nvidia_release_context(device->nvidia_capture_device);
        cu_res = cu_ctx_pop_current_ptr(get_video_thread_cuda_context_ptr());
        // Tell the main thread to bind the nvidia context again
        device->nvidia_context_is_stale = true;
        // Tell the main thread nvidia is active again
        device->active_capture_device = NVIDIA_DEVICE;
        whist_post_semaphore(device->nvidia_device_created);
    }
    return 0;
}
#endif



/*
============================
Public Function Implementations
============================
*/
int create_capture_device(CaptureDevice* device, CaptureDeviceType captureType, void* data, uint32_t width, uint32_t height, uint32_t dpi) {
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
    static int id = 1;
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

    CaptureDeviceImpl* impl = device->impl;
    uint32_t current_width, current_height, current_dpi;

    if (!impl)
    {
		switch (captureType)
		{
		case X11_DEVICE:
		    device->active_capture_device = X11_DEVICE;
		    device->impl = impl = create_x11_capture_device(&device->infos, data);
		    if (!impl) {
		    	LOG_ERROR("Failed to create X11 capture device!");
		        return -1;
		    }

			break;
		case FILE_DEVICE:
		    device->active_capture_device = FILE_DEVICE;
		    device->impl = impl = create_file_capture_device(&device->infos, data);
		    if (!impl) {
		    	LOG_ERROR("Failed to create X11 capture device!");
		        return -1;
		    }

			break;
		case WESTON_DEVICE:
		    device->active_capture_device = WESTON_DEVICE;
		    device->impl = impl = create_weston_capture_device(&device->infos, data);
		    if (!impl) {
		    	LOG_ERROR("Failed to create weston capture device!");
		        return -1;
		    }
		    break;
		case NVIDIA_DEVICE:
		default:
			break;
		}

	    if (impl->init(impl, width, height, dpi) < 0) {
	    	LOG_ERROR("Failed to initialize capture device!");
	    	return -1;
	    }

    }

#if 0
    if (impl->get_dimensions(impl, &current_width, &current_height, &current_dpi) < 0)
    	return -1;

    bool same_dpi = (!current_dpi || current_dpi != dpi);
    if (current_width != width || current_height != height || !same_dpi)
    {
    	impl->update_dimensions(impl, width, height, dpi);
    }
#endif

#if 0
    // if we can create the nvidia capture device, do so
    bool res;
    if (USING_NVIDIA_ENCODE) {
        // cuda_init the encoder context
        res = cuda_init(get_video_thread_cuda_context_ptr());
        if (!res) {
            LOG_ERROR("Failed to initialize cuda!");
        }
    }

    if (USING_NVIDIA_CAPTURE) {
        // If using the Nvidia device, we need a second context for second thread
        res = cuda_init(get_nvidia_thread_cuda_context_ptr());
        if (!res) {
            LOG_ERROR("Failed to initialize second cuda context!");
        }
        // pop the second context, since it will belong to nvidia
        CUresult cu_res = cu_ctx_pop_current_ptr(get_nvidia_thread_cuda_context_ptr());
        if (!res) {
            LOG_ERROR("Failed to initialize cuda!");
        }
        LOG_DEBUG("Nvidia context: %p, Video context: %p", *get_nvidia_thread_cuda_context_ptr(),
                  *get_video_thread_cuda_context_ptr());
        // set up semaphore and nvidia manager
        device->nvidia_device_semaphore = whist_create_semaphore(0);
        device->nvidia_device_created = whist_create_semaphore(0);
        device->nvidia_manager = whist_create_thread(multithreaded_nvidia_device_manager,
                                                     "multithreaded_nvidia_manager", device);
        whist_post_semaphore(device->nvidia_device_semaphore);
    }

    device->id = id;
    id++;
#endif

    return 0;
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

    /* TODO:
    LinkedList window_list;
    linked_list_init(&window_list);
    get_valid_windows(device, &window_list);
    // clear window data from previous pass
    for (int i = 0; i < MAX_WINDOWS; i++) {
        device->window_data[i].id = 0;
    }
    // just copy over the first MAX_WINDOW items
    int i = 0;
    linked_list_for_each(&window_list, WhistWindow, w) {
        if (i >= MAX_WINDOWS) {
            break;
        }
        device->window_data[i] = *w;
        i++;
    }
	*/


#if 0
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
                cu_ctx_synchronize_ptr();
                nvidia_bind_context(device->nvidia_capture_device);
                device->nvidia_context_is_stale = false;
            }
            int ret = nvidia_capture_screen(device->nvidia_capture_device);
            if (LOG_VIDEO && ret > 0) LOG_INFO("Captured with Nvidia!");
            if (ret >= 0) {
                if (device->width == device->nvidia_capture_device->width &&
                    device->height == device->nvidia_capture_device->height) {
                    device->last_capture_device = NVIDIA_DEVICE;
                    device->frame_data = (void*)device->nvidia_capture_device->p_gpu_texture;
                    // GPU captures need the pitch to just be width
                    device->pitch = device->nvidia_capture_device->pitch;
                    device->corner_color = device->nvidia_capture_device->corner_color;
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
            int ret = x11_capture_screen(device->x11_capture_device);
            if (LOG_VIDEO && ret > 0) LOG_INFO("Captured with X11!");
            if (ret >= 0) {
                device->frame_data = device->x11_capture_device->frame_data;
                device->pitch = device->x11_capture_device->pitch;
                device->corner_color = device->x11_capture_device->corner_color;
            }
            return ret;
        default:
            LOG_FATAL("Unknown capture device type: %d", device->active_capture_device);
            return -1;
    }
#endif

    CaptureDeviceImpl* impl= device->impl;
    int ret = impl->capture_screen(impl);
    impl->capture_get_data(impl, &device->frame_data, &device->infos.pitch);
    return ret;
}

bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height,
                                uint32_t dpi) {
    /*
       Attempt to reconfigure the capture device to the given width, height, and dpi. Resize the
       display. In Nvidia case, we resize the display and signal another thread to create the
       thread. In X11 case, we reconfigure the device.

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
#if 0
    try_update_dimensions(device, width, height, dpi);

    if (USING_NVIDIA_CAPTURE) {
        // If a nvidia capture device creation is in progress, then we should wait for it.
        // Otherwise nvidia drivers will fail/crash.
        whist_wait_semaphore(device->nvidia_device_created);
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
            cu_ctx_synchronize_ptr();
            nvidia_bind_context(device->nvidia_capture_device);
            device->nvidia_context_is_stale = false;
        }
    }

#endif

    return device->impl->reconfigure(device->impl, width, height, dpi) >= 0;
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
#if 0
    if (USING_NVIDIA_CAPTURE) {
        // tell the nvidia thread to stop
        device->pending_destruction = true;
        whist_post_semaphore(device->nvidia_device_semaphore);
        // wait for the nvidia thread to terminate
        whist_wait_thread(device->nvidia_manager, NULL);
        // now we can destroy the capture device
        if (device->nvidia_capture_device) {
            destroy_nvidia_capture_device(device->nvidia_capture_device);
        }
        cuda_destroy(*get_nvidia_thread_cuda_context_ptr());
        *get_nvidia_thread_cuda_context_ptr() = NULL;
    }
    cuda_destroy(*get_video_thread_cuda_context_ptr());
    *get_video_thread_cuda_context_ptr() = NULL;
    if (device->x11_capture_device) {
        destroy_x11_capture_device(device->x11_capture_device);
    }
    XCloseDisplay(device->display);
#endif
}

int transfer_screen(CaptureDevice* device) {
#if 0
    if (device->last_capture_device == X11_DEVICE) {
        device->frame_data = device->x11_capture_device->frame_data;
        device->pitch = device->x11_capture_device->pitch;
    }
#endif
    return 0;
}
