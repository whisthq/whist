#include "transfercapture.h"
#include "screencapture.h"
#ifdef _WIN32
#include "dxgicudatransfercapture.h"
#endif

void reinitialize_transfer_context(CaptureDevice* device, VideoEncoder* encoder) {
#ifdef _WIN32
    // If we're encoding using NVENC, we will want the dxgi cuda transfer context to be available
    if (encoder->type == NVENC_ENCODE) {
        // initialize the transfer context
        dxgi_cuda_start_transfer_context(device);
    } else if (device->dxgi_cuda_available) {
        // Otherwise, we can close the transfer context
        // end the transfer context
        dxgi_cuda_close_transfer_context(device);
    }
#else  // __linux__
    if (device->capture_is_on_nvidia) {
        if (encoder->nvidia_encoder) {
            // Copy new stuff
        }
    }
#endif
}

int transfer_capture(CaptureDevice* device, VideoEncoder* encoder) {
#ifdef _WIN32
    if (encoder->type == NVENC_ENCODE && device->dxgi_cuda_available && device->texture_on_gpu) {
        // if dxgi_cuda is setup and we have a dxgi texture on the gpu
        if (dxgi_cuda_transfer_capture(device, encoder) == 0) {
            return 0;
        } else {
            LOG_WARNING("Tried to do DXGI CUDA transfer, but transfer failed!");
            // otherwise, do the cpu transfer below
        }
    }
#else  // __linux__
    if (device->capture_is_on_nvidia) {
        if (encoder->nvidia_encoder) {
            nvidia_encoder_frame_intake(encoder->nvidia_encoder,
                                        device->nvidia_capture_device.p_gpu_texture, device->width,
                                        device->height);
            encoder->capture_is_on_nvidia = true;
            return 0;
        } else {
            encoder->capture_is_on_nvidia = false;
            LOG_ERROR(
                "Cannot transfer capture! If using Nvidia Capture SDK, "
                "then the Nvidia Encode API must be used!");
            return -1;
        }
    } else {
        encoder->capture_is_on_nvidia = false;
    }
#endif

    // CPU transfer, if hardware transfer doesn't work
    static int times_measured = 0;
    static double time_spent = 0.0;

    clock cpu_transfer_timer;
    start_timer(&cpu_transfer_timer);

    if (transfer_screen(device)) {
        LOG_ERROR("Unable to transfer screen to CPU buffer.");
        return -1;
    }
    if (video_encoder_frame_intake(encoder, device->frame_data, device->pitch)) {
        LOG_ERROR("Unable to load data to AVFrame");
        return -1;
    }

    times_measured++;
    time_spent += get_timer(cpu_transfer_timer);

    if (times_measured == 10) {
        LOG_INFO("Average time transferring frame from capture to encoder frame on CPU: %f",
                 time_spent / times_measured);
        times_measured = 0;
        time_spent = 0.0;
    }
    return 0;
}
