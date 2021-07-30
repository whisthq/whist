#ifndef CUDA_CONTEXT_H
#define CUDA_CONTEXT_H
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

typedef CUresult (*CUCTXSETCURRENTPROC)(CUcontext ctx);
extern CUCTXSETCURRENTPROC cu_ctx_set_current_ptr;
typedef CUresult (*CUCTXGETCURRENTPROC)(CUcontext* pctx);
extern CUCTXGETCURRENTPROC cu_ctx_get_current_ptr;
typedef CUresult (*CUCTXPOPCURRENTPROC)(CUcontext* pctx);
extern CUCTXPOPCURRENTPROC cu_ctx_pop_current_ptr;
typedef CUresult (*CUCTXPUSHCURRENTPROC)(CUcontext ctx);
extern CUCTXPUSHCURRENTPROC cu_ctx_push_current_ptr;
typedef CUresult (*CUCTXSYNCHRONIZEPROC)();
extern CUCTXSYNCHRONIZEPROC cu_ctx_synchronize_ptr;

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
NVFBC_BOOL cuda_init(CUcontext* cuda_context);

/**
 * @brief                          Returns a pointer to the active CUDA context.
 *
 * @returns                        Pointer to the active CUDA context
 */
CUcontext* get_video_thread_cuda_context_ptr();
CUcontext* get_nvidia_thread_cuda_context_ptr();
#endif
