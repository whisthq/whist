#include "nvidia-linux/NvFBCUtils.h"

typedef struct NvidiaCaptureDevice {
    NVFBC_SESSION_HANDLE fbcHandle;
    void *frame;
    unsigned int size;
} NvidiaCaptureDevice;

int CreateNvidiaCaptureDevice(NvidiaCaptureDevice* device);
int NvidiaCaptureScreen(NvidiaCaptureDevice* device);
void DestroyNvidiaCaptureDevice(NvidiaCaptureDevice* device);

