/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file nvidiacapture.h
 * @brief This file contains the code to do screen capture via the Nvidia FBC SDK on Linux Ubuntu.
============================
Usage
============================

NvidiaCaptureDevice contains all the information used to interface with the Nvidia FBC SDK and the
data of a frame. Call create_nvidia_capture_device to initialize a device, nvidia_capture_screen to
capture the screen with said device, and destroy_nvidia_capture_device when done capturing frames.
*/

/*
============================
Includes
============================
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <npp.h>

#include "../cudacontext.h"
#include "x11capture.h"
#include "nvidiacapture.h"

// NOTE: Using Nvidia Capture SDK 8.0.4
// Please bump this comment if a newer Nvidia Capture SDK is going to be used

#define APP_VERSION 4

#define PRINT_STATUS false

#define CAPTURE_PIXEL_FORMAT NVFBC_BUFFER_FORMAT_BGRA

#define LIB_NVFBC_NAME "libnvidia-fbc.so.1"

static int get_pitch_to_width_ratio(NVFBC_BUFFER_FORMAT buffer_format) {
    switch (buffer_format) {
        case NVFBC_BUFFER_FORMAT_ARGB:
        case NVFBC_BUFFER_FORMAT_RGBA:
        case NVFBC_BUFFER_FORMAT_BGRA:
            return 4;
        case NVFBC_BUFFER_FORMAT_RGB:
            return 3;
        case NVFBC_BUFFER_FORMAT_NV12:
        case NVFBC_BUFFER_FORMAT_YUV444P:
            return 1;
        default:
            LOG_FATAL("Unknown NVFBC_BUFFER_FORMAT");
            return -1;
    }
}

/*
============================
Public Function Implementations
============================
*/



int nvidia_bind_context(NvidiaCaptureDevice* device) {
    if (device == NULL) {
        LOG_ERROR("nvidia_bind_context received null device, doing nothing!");
        return 0;
    }
    NVFBC_BIND_CONTEXT_PARAMS bind_params = {0};
    bind_params.dwVersion = NVFBC_BIND_CONTEXT_PARAMS_VER;
    NVFBCSTATUS status = device->p_fbc_fn.nvFBCBindContext(device->fbc_handle, &bind_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("Error %d: %s", status,
                  device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }
    return 0;
}

int nvidia_release_context(NvidiaCaptureDevice* device) {
    if (device == NULL) {
        LOG_ERROR("nvidia_release_context received null device, doing nothing!");
        return 0;
    }
    NVFBC_RELEASE_CONTEXT_PARAMS release_params = {0};
    release_params.dwVersion = NVFBC_RELEASE_CONTEXT_PARAMS_VER;
    NVFBCSTATUS status = device->p_fbc_fn.nvFBCReleaseContext(device->fbc_handle, &release_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("Error %d: %s", status,
                  device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }
    return 0;
}

#define SHOW_DEBUG_FRAMES false

int nvidia_capture_screen(NvidiaCaptureDevice* device) {
    /*
        Capture the screen with the given Nvidia capture device. The texture index of the captured
       screen is stored in device->dw_texture_index.

        Arguments:
            device (NvidiaCaptureDevice*): device to use to capture the screen

        Returns:
            (int): number of new frames captured (0 on failure). On the version 7 API, we always
       return 1 on success. On version 8, we return the number of missed frames + 1.
    */
    NVFBCSTATUS status;

#if SHOW_DEBUG_FRAMES
    uint64_t t1, t2;
    t1 = NvFBCUtilsGetTimeInMillis();
#endif

    NVFBC_TOCUDA_GRAB_FRAME_PARAMS grab_params = {0};
    NVFBC_FRAME_GRAB_INFO frame_info = {0};
    void* p_rgb_texture = NULL;

    grab_params.dwVersion = NVFBC_TOCUDA_GRAB_FRAME_PARAMS_VER;

    /*
     * Use blocking calls.
     *
     * The application will wait for new frames.  New frames are generated
     * when the mouse cursor moves or when the screen if refreshed.
     */
    grab_params.dwFlags = NVFBC_TOCUDA_GRAB_FLAGS_NOWAIT;

    /*
     * This structure will contain information about the captured frame.
     */
    grab_params.pFrameGrabInfo = &frame_info;

    grab_params.pCUDADeviceBuffer = &p_rgb_texture;

    /*
     * This structure will contain information about the encoding of
     * the captured frame.
     */
    // grab_params.pEncFrameInfo = &enc_frame_info;

    /*
     * Specify per-frame encoding configuration.  Here, keep the defaults.
     */
    // grab_params.pEncodeParams = NULL;

    /*
     * This pointer is allocated by the NvFBC library and will
     * contain the captured frame.
     *
     * Make sure this pointer stays the same during the capture session
     * to prevent memory leaks.
     */
    // grab_params.ppBitStreamBuffer = (void**)&device->frame;

    /*
     * Capture a new frame.
     */
    status = device->p_fbc_fn.nvFBCToCudaGrabFrame(device->fbc_handle, &grab_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        return -1;
    }

    // If the frame isn't new, just return 0
    if (!frame_info.bIsNewFrame) {
        return 0;
    }

    uint8_t top_left_corner[4];
    CUresult copy_status = cu_memcpy_dtoh_v2_ptr(top_left_corner, (CUdeviceptr)p_rgb_texture, 4);
    if (copy_status != CUDA_SUCCESS) {
        LOG_WARNING("Failed to fetch top-left-corner from CUDA RGB texture: %d.", copy_status);
    } else {
        device->corner_color.red = top_left_corner[2];
        device->corner_color.green = top_left_corner[1];
        device->corner_color.blue = top_left_corner[0];
    }

    CUDA_MEMCPY2D copy_params;
    copy_params.srcXInBytes = 0;
    copy_params.srcY = 0;
    copy_params.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    copy_params.srcDevice = (CUdeviceptr)p_rgb_texture;
    copy_params.srcPitch = frame_info.dwWidth * get_pitch_to_width_ratio(CAPTURE_PIXEL_FORMAT);
    copy_params.dstXInBytes = 0;
    copy_params.dstY = 0;
    copy_params.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    copy_params.dstDevice = device->p_gpu_texture;
    copy_params.dstPitch = frame_info.dwWidth * get_pitch_to_width_ratio(CAPTURE_PIXEL_FORMAT);
    copy_params.WidthInBytes = frame_info.dwWidth * get_pitch_to_width_ratio(CAPTURE_PIXEL_FORMAT);
    copy_params.Height = frame_info.dwHeight;
    copy_status = cu_memcpy_2d_v2_ptr(&copy_params);
    if (copy_status != CUDA_SUCCESS) {
        LOG_ERROR("Failed to memcpy to rounded CUDA RGB texture: %d.", copy_status);
    }

    // Set the device to use the newly captured width/height
    device->width = frame_info.dwWidth;
    device->height = frame_info.dwHeight;
    device->pitch = device->width * get_pitch_to_width_ratio(CAPTURE_PIXEL_FORMAT);

#if SHOW_DEBUG_FRAMES
    t2 = NvFBCUtilsGetTimeInMillis();

    LOG_INFO("New %dx%d frame or size %u grabbed in %llu ms", device->width, device->height, 0,
             (unsigned long long)(t2 - t1));
#endif

    // Return with the number of accumulated frames
    // NOTE: Can only do on SDK 8.0.4
    return 1;
}

void destroy_nvidia_capture_device(NvidiaCaptureDevice* device) {
    /*
        Destroy the device and its resources.

        Arguments
            device (NvidiaCaptureDevice*): device to destroy
    */
    if (device == NULL) {
        LOG_ERROR("destroy_nvidia_capture_device received NULL device!");
        return;
    }
    LOG_DEBUG("Called destroy_nvidia_capture_device");
    NVFBCSTATUS status;

    /*
     * Destroy capture session, tear down resources.
     */
    NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroy_capture_params = {0};
    destroy_capture_params.dwVersion = NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER;

    status =
        device->p_fbc_fn.nvFBCDestroyCaptureSession(device->fbc_handle, &destroy_capture_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
    }

    /*
     * Destroy session handle, tear down more resources.
     */
    NVFBC_DESTROY_HANDLE_PARAMS destroy_handle_params = {0};
    destroy_handle_params.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER;

    status = device->p_fbc_fn.nvFBCDestroyHandle(device->fbc_handle, &destroy_handle_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
    }

    if (device->p_gpu_texture) {
        CUresult res = cu_mem_free_ptr(device->p_gpu_texture);
        if (res != CUDA_SUCCESS) {
            LOG_ERROR("YUV buffer free failed %d", res);
        }
    }
    free(device);
}

NvidiaCaptureDevice* create_nvidia_capture_device(CaptureDeviceInfos* infos, const char *display) {
    /*
        Create an Nvidia capture device that attaches to the current display via gl_init. This will
       capture to OpenGL textures, and each capture is stored in the texture at index
       dw_texture_index. Captures are done without cursors; the cursor is added in client-side.

        Returns:
            (NvidiaCaptureDevice*): pointer to the newly created capture device
    */
    NvidiaCaptureDevice* device = (NvidiaCaptureDevice*)safe_zalloc(sizeof(*device));
    CaptureDeviceImpl *base = &device->base;

    char output_name[NVFBC_OUTPUT_NAME_LEN];
    uint32_t output_id = 0;

    NVFBCSTATUS status;

    NvFBCUtilsPrintVersions(APP_VERSION);

    /*
     * Dynamically load the NvFBC library.
     */
    void* lib_nvfbc = dlopen(LIB_NVFBC_NAME, RTLD_NOW);
    if (lib_nvfbc == NULL) {
        LOG_ERROR("Unable to open '%s' (%s)", LIB_NVFBC_NAME, dlerror());
        goto out_dlopen;
    }

    /*
     * Resolve the 'NvFBCCreateInstance' symbol that will allow us to get
     * the API function pointers.
     */
    PNVFBCCREATEINSTANCE nv_fbc_create_instance_ptr =
        (PNVFBCCREATEINSTANCE)dlsym(lib_nvfbc, "NvFBCCreateInstance");
    if (nv_fbc_create_instance_ptr == NULL) {
        LOG_ERROR("Unable to resolve symbol 'NvFBCCreateInstance'");
        goto out_dlsym;
    }

    /*
     * Create an NvFBC instance.
     *
     * API function pointers are accessible through p_fbc_fn.
     */
    device->p_fbc_fn.dwVersion = NVFBC_VERSION;

    status = nv_fbc_create_instance_ptr(&device->p_fbc_fn);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("Unable to create NvFBC instance (status: %d)", status);
        goto out_createinstance_ptr;
    }

    /*
     * Create a session handle that is used to identify the client.
     */
    NVFBC_CREATE_HANDLE_PARAMS create_handle_params = {0};
    create_handle_params.dwVersion = NVFBC_CREATE_HANDLE_PARAMS_VER;

    status = device->p_fbc_fn.nvFBCCreateHandle(&device->fbc_handle, &create_handle_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        goto out_fb_create_handle;
    }

    /*
     * Get information about the state of the display driver.
     *
     * This call is optional but helps the application decide what it should
     * do.
     */
    NVFBC_GET_STATUS_PARAMS status_params = {0};
    status_params.dwVersion = NVFBC_GET_STATUS_PARAMS_VER;

    status = device->p_fbc_fn.nvFBCGetStatus(device->fbc_handle, &status_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("Nvidia Error: %d %s", status,
                  device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        goto out_fbc_getstatus;
        return NULL;
    }

#if PRINT_STATUS
    NvFBCUtilsPrintStatus(&status_params);
#endif

    if (status_params.bCanCreateNow == NVFBC_FALSE) {
        LOG_ERROR("It is not possible to create a capture session on this system.");
        goto out_fbc_getstatus;
    }

    // Get width and height
    infos->width = status_params.screenSize.w;
    infos->height = status_params.screenSize.h;
    infos->pitch = infos->width * get_pitch_to_width_ratio(CAPTURE_PIXEL_FORMAT);

    if (infos->width % 4 != 0) {
        LOG_ERROR("Device width must be a multiple of 4!");
        goto out_fbc_sizechecks;
    }

    /*
     * Create a capture session.
     */
    LOG_INFO("Creating a capture session of NVidia compressed frames.");

    NVFBC_CREATE_CAPTURE_SESSION_PARAMS create_capture_params = {0};
    NVFBC_SIZE frame_size = {0, 0};
    create_capture_params.dwVersion = NVFBC_CREATE_CAPTURE_SESSION_PARAMS_VER;
    create_capture_params.eCaptureType = NVFBC_CAPTURE_SHARED_CUDA;
    create_capture_params.bWithCursor = NVFBC_FALSE;
    create_capture_params.frameSize = frame_size;
    create_capture_params.bRoundFrameSize = NVFBC_TRUE;
    create_capture_params.eTrackingType = NVFBC_TRACKING_DEFAULT;
    // Set to FALSE to avoid NVFBC_ERR_MUST_RECREATE error
    create_capture_params.bDisableAutoModesetRecovery = NVFBC_FALSE;

    status = device->p_fbc_fn.nvFBCCreateCaptureSession(device->fbc_handle, &create_capture_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        goto out_create_capture_session;
    }

    /*
     * Set up the capture session.
     */
    NVFBC_TOCUDA_SETUP_PARAMS setup_params = {0};
    setup_params.dwVersion = NVFBC_TOCUDA_SETUP_PARAMS_VER;
    setup_params.eBufferFormat = NVFBC_BUFFER_FORMAT_BGRA;

    status = device->p_fbc_fn.nvFBCToCudaSetUp(device->fbc_handle, &setup_params);
    if (status != NVFBC_SUCCESS) {
        LOG_ERROR("%s", device->p_fbc_fn.nvFBCGetLastErrorStr(device->fbc_handle));
        goto out_cuda_setup;
    }

    CUresult res = cu_mem_alloc_ptr(&device->p_gpu_texture, (device->base.width * device->base.height * 3) / 2);
	if (res != CUDA_SUCCESS) {
		LOG_ERROR("YUV buffer allocation failed %d", res);
		destroy_nvidia_capture_device(device);
		return NULL;
	}
    /*
    // Capture screen once to set p_gpu_texture
    if (nvidia_capture_screen(device) == 0) {
        LOG_ERROR("Preliminary capture screen failed!");
        return NULL;
    }
    */

    /*
     * We are now ready to start grabbing frames.
     */
    LOG_INFO(
        "Nvidia Frame capture session started. New frames will be captured when "
        "the display is refreshed or when the mouse cursor moves.");

    return device;

out_cuda_setup:
	NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroyParams = { NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER };
	device->p_fbc_fn.nvFBCDestroyCaptureSession(device->fbc_handle, &destroyParams);
out_create_capture_session:
out_fbc_sizechecks:
out_fbc_getstatus:
	device->p_fbc_fn.nvFBCDestroyHandle(device->fbc_handle, NULL);
out_fb_create_handle:
out_createinstance_ptr:
out_dlsym:
	dlclose(lib_nvfbc);
out_dlopen:
	free(device);
	return NULL;
}

