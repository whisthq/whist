/**
 * Copyright Fractal Computers, Inc. 2021
 * @file cudacontext.h
 * @brief This file contains the code to create a CUDA context
============================
Usage
============================
The CUDA context is required for the NVIDIA encoder. First call
`cuda_init()` and then use the initialized CUDA context by
calling `get_active_cuda_context_ptr()`.
*/

/*
============================
Includes
============================
*/

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/cuda_drvapi_dynlink_cuda.h"
#include <fractal/core/fractal.h>

/*
============================
Public Functions
============================
*/

/**
 * Initializes CUDA and creates a CUDA context.
 *
 * \return
 *   NVFBC_TRUE in case of success, NVFBC_FALSE otherwise.
 */
NVFBC_BOOL cuda_init();

/**
 * @brief                          Returns a pointer to the active CUDA context.
 *
 * @returns                        Pointer to the active CUDA context
 */
CUcontext* get_active_cuda_context_ptr();
