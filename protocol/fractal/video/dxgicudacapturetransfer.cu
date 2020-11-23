#include <cuda.h>
#define WIN32_LEAN_AND_MEAN
#include <cuda_d3d11_interop.h>
#include <stdio.h>

#include "dxgicudacapturetransfer.h"

extern "C" {
bool cuda_is_available = false;
bool active_transfer_context = false;
cudaGraphicsResource_t resource = NULL;

int dxgi_cuda_start_transfer_context(CaptureDevice* device) {
    static bool tried_cuda_check = false;
    if (!tried_cuda_check) {
        tried_cuda_check = true;

        if (cudaDeviceSynchronize() != cudaSuccess) {
            LOG_INFO("CUDA requested but not available on this device.");
            return 1;  // cuda unavailable
        } else {
            cuda_is_available = true;
        }
    }

    if (cuda_is_available) {
        if (active_transfer_context) return 0;

        cudaError_t res = cudaGraphicsD3D11RegisterResource(
            &resource, device->screenshot.staging_texture, 0);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
            return -1;
        }

        active_transfer_context = true;

        return 0;
    } else {
        return 1;  // cuda unavailable
    }
}

void dxgi_cuda_close_transfer_context() {
    if (cuda_is_available && active_transfer_context && resource) {
        cudaGraphicsUnregisterResource(resource);
        active_transfer_context = false;
    }
}

int dxgi_cuda_transfer_capture(CaptureDevice* device,
                               video_encoder_t* encoder) {
    if (cuda_is_available && active_transfer_context) {
        cudaError_t res;
        cudaArray_t mappedArray;

        static int times_measured = 0;
        static double time_spent = 0.;

        clock dxgi_cuda_transfer_timer;
        StartTimer(&dxgi_cuda_transfer_timer);

        res = cudaGraphicsResourceSetMapFlags(resource,
                                              cudaGraphicsMapFlagsReadOnly);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
            return -1;
        }

        res = cudaGraphicsMapResources(1, &resource, 0);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
            return -1;
        }

        res =
            cudaGraphicsSubResourceGetMappedArray(&mappedArray, resource, 0, 0);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
            return -1;
        }

        res = cudaGraphicsUnmapResources(1, &resource, 0);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
            return -1;
        }

        encoder->sw_frame->pts++;
        encoder->hw_frame->pict_type = encoder->sw_frame->pict_type;

        res = cudaMemcpy2DFromArray(
            encoder->hw_frame->data[0], encoder->hw_frame->linesize[0],
            mappedArray, 0, 0, encoder->hw_frame->width * 4,
            encoder->hw_frame->height, cudaMemcpyDefault);
        if (res != cudaSuccess) {
            LOG_ERROR("Error: %s | %s", cudaGetErrorName(res),
                      cudaGetErrorString(res));
            return -1;
        }

        times_measured++;
        time_spent += GetTimer(dxgi_cuda_transfer_timer);

        if (times_measured == 10) {
            LOG_INFO(
                "Average time transferring dxgi data to encoder frame on CUDA "
                "GPU: "
                "%f",
                time_spent / times_measured);
            times_measured = 0;
            time_spent = 0.0;
        }
        return 0;
    } else {
        return 1;  // cuda unavailable
    }
}
}