#include "cudacontext.h"

#define LIB_CUDA_NAME "libcuda.so.1"

/*
 * CUDA entry points
 */
typedef CUresult (*CUINITPROC)(unsigned int flags);
typedef CUresult (*CUDEVICEGETPROC)(CUdevice* device, int ordinal);
typedef CUresult (*CUCTXCREATEV2PROC)(CUcontext* pctx, unsigned int flags, CUdevice dev);
typedef CUresult (*CUMEMCPYDTOHV2PROC)(void* dst_host, CUdeviceptr src_device, size_t byte_count);

static CUINITPROC cu_init_ptr = NULL;
static CUDEVICEGETPROC cu_device_get_ptr = NULL;
static CUCTXCREATEV2PROC cu_ctx_create_v2_ptr = NULL;
static CUMEMCPYDTOHV2PROC cu_memcpy_dtoh_v2_ptr = NULL;

CUContext active_cuda_context = NULL;

/**
 * Dynamically opens the CUDA library and resolves the symbols that are
 * needed for this application.
 *
 * \param [out] lib_cuda
 *   A pointer to the opened CUDA library.
 *
 * \return
 *   NVFBC_TRUE in case of success, NVFBC_FALSE otherwise.
 */
static NVFBC_BOOL cuda_load_library(void* lib_cuda) {
    lib_cuda = dlopen(LIB_CUDA_NAME, RTLD_NOW);
    if (lib_cuda == NULL) {
        fprintf(stderr, "Unable to open '%s'\n", LIB_CUDA_NAME);
        return NVFBC_FALSE;
    }

    cu_init_ptr = (CUINITPROC)dlsym(lib_cuda, "cuInit");
    if (cu_init_ptr == NULL) {
        fprintf(stderr, "Unable to resolve symbol 'cuInit'\n");
        return NVFBC_FALSE;
    }

    cu_device_get_ptr = (CUDEVICEGETPROC)dlsym(lib_cuda, "cuDeviceGet");
    if (cu_device_get_ptr == NULL) {
        fprintf(stderr, "Unable to resolve symbol 'cuDeviceGet'\n");
        return NVFBC_FALSE;
    }

    cu_ctx_create_v2_ptr = (CUCTXCREATEV2PROC)dlsym(lib_cuda, "cuCtxCreate_v2");
    if (cu_ctx_create_v2_ptr == NULL) {
        fprintf(stderr, "Unable to resolve symbol 'cuCtxCreate_v2'\n");
        return NVFBC_FALSE;
    }

    cu_memcpy_dtoh_v2_ptr = (CUMEMCPYDTOHV2PROC)dlsym(lib_cuda, "cuMemcpyDtoH_v2");
    if (cu_memcpy_dtoh_v2_ptr == NULL) {
        fprintf(stderr, "Unable to resolve symbol 'cuMemcpyDtoH_v2'\n");
        return NVFBC_FALSE;
    }

    return NVFBC_TRUE;
}

/**
 * Initializes CUDA and creates a CUDA context.
 *
 * \param [in] cu_ctx
 *   A pointer to the created CUDA context.
 *
 * \return
 *   NVFBC_TRUE in case of success, NVFBC_FALSE otherwise.
 */
static NVFBC_BOOL cuda_init(CUcontext* cu_ctx) {
    void* lib_cuda = NULL;
    if (cuda_load_library(lib_cuda) != NVFBC_TRUE) {
        LOG_ERROR("Failed to load CUDA library!");
        return NVFBC_FALSE;
    }

    CUresult cu_res;
    CUdevice cu_dev;

    cu_res = cu_init_ptr(0);
    if (cu_res != CUDA_SUCCESS) {
        fprintf(stderr, "Unable to initialize CUDA (result: %d)\n", cu_res);
        return NVFBC_FALSE;
    }

    cu_res = cu_device_get_ptr(&cu_dev, 0);
    if (cu_res != CUDA_SUCCESS) {
        fprintf(stderr, "Unable to get CUDA device (result: %d)\n", cu_res);
        return NVFBC_FALSE;
    }

    cu_res = cu_ctx_create_v2_ptr(cu_ctx, CU_CTX_SCHED_AUTO, cu_dev);
    if (cu_res != CUDA_SUCCESS) {
        fprintf(stderr, "Unable to create CUDA context (result: %d)\n", cu_res);
        return NVFBC_FALSE;
    }

    return NVFBC_TRUE;
}

CUContext* get_active_cuda_context_ptr() {
    return &active_cuda_context;
}