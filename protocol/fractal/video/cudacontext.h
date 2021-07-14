#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/cuda_drvapi_dynlink_cuda.h"
#include <fractal/core/fractal.h>

NVFBC_BOOL cuda_init(CUcontext* cu_ctx);
CUcontext* get_active_cuda_context_ptr();
