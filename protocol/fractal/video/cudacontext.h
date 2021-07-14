#include "nvidia-linux/cuda_drvapi_dynlink_cuda.h"

static NVFBC_BOOL cuda_init(CUcontext* cu_ctx);
static CUcontext* get_active_cuda_context_ptr();