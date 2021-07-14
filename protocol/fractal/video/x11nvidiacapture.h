#ifndef CAPTURE_X11NVIDIACAPTURE_H
#define CAPTURE_X11NVIDIACAPTURE_H

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include <fractal/core/fractal.h>

typedef struct {
    // Width and height of the capture session
    int width;
    int height;

    // Nvidia API structs
    NVFBC_SESSION_HANDLE fbc_handle;
    NVFBC_API_FUNCTION_LIST p_fbc_fn;
    // Contains pointers to the GPU textures that the Nvidia Capture Device will capture into
    NVFBC_TOGL_SETUP_PARAMS togl_setup_params;
    // TODO: document
    uint32_t dw_texture_index;
} NvidiaCaptureDevice;

/**
 * @brief                          Creates an nvidia capture device
 *
 * @param device                   Capture device struct to hold the capture
 *                                 device
 *
 * @returns                        0 on success, -1 on failure
 */
int create_nvidia_capture_device(NvidiaCaptureDevice* device);
/**
 * @brief                          Captures the screen into GPU pointers.
 *                                 This will also set width/height to the dimensions
 *                                 of the captured image.
 *
 * @param device                   Capture device to capture with
 *
 * @returns                        0 on success, -1 on failure
 */
int nvidia_capture_screen(NvidiaCaptureDevice* device);

/**
 * @brief                          Destroy the nvidia capture device
 *
 * @param device                   The Capture device to destroy
 */
void destroy_nvidia_capture_device(NvidiaCaptureDevice* device);

#endif  // CAPTURE_X11NVIDIACAPTURE_H
