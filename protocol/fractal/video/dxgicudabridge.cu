#include <cuda.h>
#include <libavcodec/avcodec.h>

#include "dxgicudabridge.h"

__global__ bool dxgi_cuda_transfer_data(CaptureDevice* device,
                                        video_encoder_t* encoder) {
    int i = 3;
    for (int j = 0; j < 52; ++j) {
        i += j;
    }
    return i;
}