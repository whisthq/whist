#include "x11capture.h"
#include "nvidiacapture.h"
#include "nvx11capture.h"

typedef struct {
    CaptureDeviceImpl base;
    CaptureDeviceImpl* x11impl;
    CaptureDeviceImpl* nvimpl;

    bool use_nvidia_device;                 // the device currently used for capturing
    CaptureDeviceType last_capture_device;  // the device used for the last capture, so we can pick
                                            // the right encoder

    bool pending_destruction;
    WhistThread nvidia_manager;
    WhistSemaphore nvidia_device_semaphore;
    WhistSemaphore nvidia_device_created;
    bool nvidia_context_is_stale;
    CUcontext nvidia_cuda_context;
} NvX11CaptureDevice;

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
    NvX11CaptureDevice* device = (NvX11CaptureDevice*)opaque;
    WhistTimer running_timer;

    start_timer(&running_timer);

    while (true) {
        whist_wait_semaphore(device->nvidia_device_semaphore);
        if (device->pending_destruction) {
            break;
        }

        // set current context
        CUresult cu_res = cu_ctx_push_current_ptr(device->nvidia_cuda_context);
        if (cu_res != CUDA_SUCCESS) {
            LOG_ERROR("Failed to push current context, status %d!", cu_res);
        }
        cu_ctx_synchronize_ptr();

        // Nvidia requires recreation
        while (device->nvimpl == NULL) {
            LOG_INFO("Creating nvidia capture device...");
            device->nvimpl = create_nvidia_capture_device(device->base.infos, NULL);
            if (device->nvimpl == NULL) {
                /* retry every 200 ms at the beginning and after 4s, retry every 2 seconds */
                uint32_t sleep_delay = 200;

                if (get_timer(&running_timer) > 4) sleep_delay = 2 * 1000;

                whist_sleep(sleep_delay);
            }
        }

        LOG_INFO("Created nvidia capture device!");
        NvidiaCaptureDevice* nv_capture = (NvidiaCaptureDevice*)device->nvimpl;
        LOG_DEBUG("device handle: %" PRIu64, nv_capture->fbc_handle);
        nvidia_release_context((NvidiaCaptureDevice*)device->nvimpl);

        cu_res = cu_ctx_pop_current_ptr(&device->nvidia_cuda_context);
        if (cu_res) {
            LOG_INFO("an error happened when popping the nvidia context (status = %d)", cu_res);
        }

        device->nvidia_context_is_stale = true;
        device->use_nvidia_device = true;
        whist_post_semaphore(device->nvidia_device_created);
    }
    return 0;
}

static int nvx11_capture_device_init(NvX11CaptureDevice* dev, uint32_t width, uint32_t height,
                                     uint32_t dpi) {
    int ret = dev->x11impl->init(dev->x11impl, width, height, dpi);
    whist_post_semaphore(dev->nvidia_device_semaphore);
    return ret;
}

static int nvx11_reconfigure_capture_device(NvX11CaptureDevice* dev, uint32_t width,
                                            uint32_t height, uint32_t dpi) {
    int ret;

    ret = dev->x11impl->reconfigure(dev->x11impl, width, height, dpi);
    if (ret != 0) return ret;

    if (dev->use_nvidia_device) {
        ret = dev->nvimpl->reconfigure(dev->nvimpl, width, height, dpi);
    }
    return ret;
}

static int nvx11_capture_screen(NvX11CaptureDevice* dev, uint32_t* width, uint32_t* height,
                                uint32_t* pitch) {
    CaptureDeviceImpl* impl = dev->x11impl;
    if (dev->use_nvidia_device) {
        impl = dev->nvimpl;

        if (dev->nvidia_context_is_stale) {
            CUresult cu_res = cu_ctx_push_current_ptr(dev->nvidia_cuda_context);
            if (cu_res != CUDA_SUCCESS) {
                LOG_ERROR("Failed to swap contexts!");
            }
            cu_ctx_synchronize_ptr();

            nvidia_bind_context((NvidiaCaptureDevice*)impl);
            dev->nvidia_context_is_stale = false;
            impl->infos->last_capture_device = NVIDIA_DEVICE;
        }

        uint32_t new_w, new_h, new_pitch;
        int ret = impl->capture_screen(impl, &new_w, &new_h, &new_pitch);
        if (LOG_VIDEO && ret > 0) LOG_INFO("Captured with Nvidia!");

        if (ret >= 0) {
            dev->last_capture_device = NVIDIA_DEVICE;
        }
    }
    return impl->capture_screen(impl, width, height, pitch);
}

static int nvx11_get_dimensions(NvX11CaptureDevice* dev, uint32_t* w, uint32_t* h, uint32_t* dpi) {
    return dev->x11impl->get_dimensions(dev->x11impl, w, h, dpi);
}

static int nvx11_capture_getdata(NvX11CaptureDevice* dev, void** buf, uint32_t* stride) {
    CaptureDeviceImpl* impl = dev->use_nvidia_device ? dev->nvimpl : dev->x11impl;
    return impl->capture_get_data(impl, buf, stride);
}

static int nvx11_capture_transfer_screen(NvX11CaptureDevice* dev) {
    CaptureDeviceImpl* impl = dev->use_nvidia_device ? dev->nvimpl : dev->x11impl;
    if (impl && impl->transfer_screen) return impl->transfer_screen(impl);

    return 0;
}

static int nvx11_capture_cuda_context(NvX11CaptureDevice* dev, CUcontext* context) {
    if (dev->use_nvidia_device)
        *context = dev->nvidia_cuda_context;
    else
        *context = *get_video_thread_cuda_context_ptr();

    return 0;
}

static void nvx11_destroy_capture_device(NvX11CaptureDevice** pdev) {
    NvX11CaptureDevice* dev = *pdev;
    int ret;

    dev->pending_destruction = true;
    whist_post_semaphore(dev->nvidia_device_semaphore);
    whist_wait_thread(dev->nvidia_manager, &ret);

    dev->x11impl->destroy(&dev->x11impl);
    if (dev->nvimpl) dev->nvimpl->destroy(&dev->nvimpl);
    free(dev);
    *pdev = NULL;
}

CaptureDeviceImpl* create_nvx11_capture_device(CaptureDeviceInfos* infos, const char* display) {
    NvX11CaptureDevice* dev = (NvX11CaptureDevice*)safe_zalloc(sizeof(X11CaptureDevice));
    CaptureDeviceImpl* impl = &dev->base;

    impl->device_type = NVX11_DEVICE;
    impl->infos = infos;
    impl->init = (CaptureDeviceInitFn)nvx11_capture_device_init;
    impl->reconfigure = (CaptureDeviceReconfigureFn)nvx11_reconfigure_capture_device;
    impl->capture_screen = (CaptureDeviceCaptureScreenFn)nvx11_capture_screen;
    impl->capture_get_data = (CaptureDeviceGetDataFn)nvx11_capture_getdata;
    impl->transfer_screen = (CaptureDeviceTransferScreenFn)nvx11_capture_transfer_screen;
    impl->get_dimensions = (CaptureDeviceGetDimensionsFn)nvx11_get_dimensions;
    impl->cuda_context = (CaptureDeviceCudaContext)nvx11_capture_cuda_context;
    impl->destroy = (CaptureDeviceDestroyFn)nvx11_destroy_capture_device;

    dev->x11impl = create_x11_capture_device(infos, display);
    if (!dev->x11impl) goto error_x11;

    bool res = cuda_init(&dev->nvidia_cuda_context, false);
    if (!res) {
        LOG_ERROR("Failed to initialize second cuda context!");
    }

    // pop the second context, since it will belong to nvidia
    /*CUresult cu_res = cu_ctx_pop_current_ptr(get_nvidia_thread_cuda_context_ptr());
    if (!res) {
        LOG_ERROR("Failed to initialize cuda!");
    }*/
    LOG_DEBUG("Nvidia context: %p, Video context: %p", dev->nvidia_cuda_context,
              *get_video_thread_cuda_context_ptr());
    // set up semaphore and nvidia manager
    dev->nvidia_device_semaphore = whist_create_semaphore(0);
    dev->nvidia_device_created = whist_create_semaphore(0);
    dev->nvidia_manager = whist_create_thread(multithreaded_nvidia_device_manager,
                                              "multithreaded_nvidia_manager", dev);
    return impl;

error_x11:
    free(dev);
    return NULL;
}

X11CaptureDevice* nvx11_capture_get_x11(CaptureDeviceImpl* impl) {
    NvX11CaptureDevice* nvx11 = (NvX11CaptureDevice*)impl;
    return (X11CaptureDevice*)nvx11->x11impl;
}
