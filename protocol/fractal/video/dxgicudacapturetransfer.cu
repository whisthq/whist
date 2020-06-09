#include <cuda.h>
#define WIN32_LEAN_AND_MEAN
#include <cuda_d3d11_interop.h>
#include <stdio.h>

#include "dxgicudacapturetransfer.h"

extern "C" {
bool cuda_is_available = true;
cudaGraphicsResource_t resource = NULL;

int dxgi_cuda_start_transfer_context(CaptureDevice* device) {
    static bool tried_cuda_check = false;
    if (!tried_cuda_check && cudaDeviceSynchronize() != cudaSuccess) {
        LOG_INFO("CUDA requested but not available on this device.");
        cuda_is_available = false;
        tried_cuda_check = true;
        return 1;  // cuda unavailable
    }

    for (int i = 0; i < device->hardware->n; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        device->hardware->adapters[i]->GetDesc1(&desc);
        LOG_INFO("Adapter %d: %S", i, desc.Description);
        int res1;
        cudaError_t res2;
        res2 = cudaD3D11GetDevice(&res1, device->hardware->adapters[i]);
        if (res2 != cudaSuccess) {
            LOG_INFO("%d NO CUDA SUPPORT: %s | %s", i, cudaGetErrorName(res2),
                     cudaGetErrorString(res2));
        } else {
            LOG_INFO("%d has cuda device %d", res1);
        }
    }

    unsigned int num = 0;

    int* list = NULL;
    cudaD3D11GetDevices(&num, list, 0, device->D3D11device,
                        cudaD3D11DeviceListAll);
    LOG_INFO("num %d", num);

    cudaError_t res = cudaGraphicsD3D11RegisterResource(
        &resource, device->screenshot.staging_texture, 0);
    if (res != cudaSuccess) {
        LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                  cudaGetErrorString(res));
    }

    return 0;
}

void dxgi_cuda_close_transfer_context() {
    if (cuda_is_available && resource) {
        cudaGraphicsUnregisterResource(resource);
    }
}

int dxgi_cuda_transfer_data(CaptureDevice* device, video_encoder_t* encoder) {
    if (cuda_is_available) {
        cudaError_t res;
        cudaArray_t mappedArray;

        LOG_INFO("a");
        LOG_INFO("resource @ %p", resource);

        if (!resource) {
            return -1;  // failure
        }

        cudaGraphicsResourceSetMapFlags(resource, cudaGraphicsMapFlagsReadOnly);
        res = cudaGraphicsMapResources(1, &resource, 0);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
        }

        res =
            cudaGraphicsSubResourceGetMappedArray(&mappedArray, resource, 0, 0);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
        }

        LOG_INFO("a");

        cudaGraphicsUnmapResources(1, &resource, 0);

        LOG_INFO("a");

        LOG_INFO("hwframe @ %p", encoder->hw_frame);
        LOG_INFO("data @ %p, val %p", encoder->hw_frame->data,
                 encoder->hw_frame->data[0]);
        LOG_INFO("linesize @ %p, val %d", encoder->hw_frame->linesize,
                 encoder->hw_frame->linesize[0]);
        LOG_INFO("w %d h %d", encoder->hw_frame->width,
                 encoder->hw_frame->height);
        LOG_INFO("mappedArray @ %p", mappedArray);

        cudaMemcpy2DFromArray(encoder->hw_frame->data[0],
                              encoder->hw_frame->linesize[0], mappedArray, 0, 0,
                              encoder->hw_frame->width * 4,
                              encoder->hw_frame->height, cudaMemcpyDefault);

        LOG_INFO("a");

        return 0;
    } else {
        return 1;  // cuda unavailable
    }
}
}