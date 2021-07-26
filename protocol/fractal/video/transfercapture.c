#include "transfercapture.h"
#include "screencapture.h"
#ifdef _WIN32
#include "dxgicudatransfercapture.h"
#else
#include "nvidiatransfercapture.h"
#endif

int start_transfer_context(CaptureDevice* device, VideoEncoder* encoder) {
    /*
        Initialize the transfer context between the given device and encoder. Call this during
        resolution changes and creation of one of the device or encoder.

        Arguments:
            device (CaptureDevice*): device handling screen capturing
            encoder (VideoEncoder*): encoder encoding captured frames

        Returns:
            0 on success, -1 on failure.
    */
#ifdef _WIN32
    // If we're encoding using NVENC, we will want the dxgi cuda transfer context to be available
    // TODO: not sure what the behavior should be if not NVENC?
    if (encoder->ffmpeg_encoder->type == NVENC_ENCODE) {
        // initialize the transfer context
        return dxgi_cuda_start_transfer_context(device);
    }
    return 0;
#else  // __linux__
    if (device->active_capture_device == NVIDIA_DEVICE) {
        if (encoder->active_encoder == NVIDIA_ENCODER) {
            return nvidia_start_transfer_context(device->nvidia_capture_device,
                                                 encoder->nvidia_encoder);
        }
    }
    return 0;
#endif
}

int close_transfer_context(CaptureDevice* device, VideoEncoder* encoder) {
    /*
        Close the transfer context between the given device and encoder. Call this when the
        resolution has changed and during destruction of device or encoder.

        Arguments:
            device (CaptureDevice*): device handling screen capturing
            encoder (VideoEncoder*): encoder encoding captured frames

        Returns:
            0 on success, -1 on failure.
    */
    int success = 0;
#ifdef _WIN32
    // If we're encoding using NVENC, we will want the dxgi cuda transfer context to be available
    if (device->dxgi_cuda_available) {
        // initialize the transfer context
        if (dxgi_cuda_close_transfer_context(device) == -1) {
            success = -1;
        }
    }
#else  // __linux__
    if (device->active_capture_device == NVIDIA_DEVICE) {
        if (encoder->active_encoder == NVIDIA_ENCODER) {
            if (nvidia_close_transfer_context(encoder->nvidia_encoder) == -1) {
                success = -1;
            }
        }
    }
#endif
    return success;
}

int transfer_capture(CaptureDevice* device, VideoEncoder* encoder) {
    if (device->width != encoder->in_width || device->height != encoder->in_height) {
        LOG_ERROR(
            "Tried to pass in a captured frame of dimension %dx%d, "
            "into an encoder that accepts %dx%d as input",
            device->width, device->height, encoder->in_width, encoder->in_height);
        return -1;
    }

    // Handle transfer capture for linux nvidia capture device
#ifdef __linux__
    if (device->active_capture_device == NVIDIA_DEVICE) {
        if (encoder->active_encoder == NVIDIA_ENCODER) {
            nvidia_encoder_frame_intake(encoder->nvidia_encoder,
                                        device->width,
                                        device->height);
            return 0;
        } else {
            LOG_ERROR(
                "Cannot transfer capture! If using Nvidia Capture SDK, "
                "then the Nvidia Encode API must be used!");
            return -1;
        }
    }
#endif

    // Handle the normal active_capture_device
    if (encoder->active_encoder == FFMPEG_ENCODER) {
        // Handle cuda optimized transfer on windows
#ifdef _WIN32
        if (encoder->ffmpeg_encoder->type == NVENC_ENCODE && device->dxgi_cuda_available &&
            device->texture_on_gpu) {
            // if dxgi_cuda is setup and we have a dxgi texture on the gpu
            if (dxgi_cuda_transfer_capture(device, encoder) == 0) {
                return 0;
            } else {
                LOG_WARNING("Tried to do DXGI CUDA transfer, but transfer failed!");
                // otherwise, do the cpu transfer below
            }
        }
#endif

        // CPU transfer, if hardware transfer doesn't work
        static int times_measured = 0;
        static double time_spent = 0.0;

        clock cpu_transfer_timer;
        start_timer(&cpu_transfer_timer);

        // This will populate frame_data/pitch from DXGI/X11
        if (transfer_screen(device)) {
            LOG_ERROR("Unable to transfer screen to CPU buffer.");
            return -1;
        }

        if (ffmpeg_encoder_frame_intake(encoder->ffmpeg_encoder, device->frame_data,
                                        device->pitch)) {
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
    } else {
        LOG_ERROR("Using normal capture, but encoder wasn't ffmpeg!");
        return -1;
    }

    return 0;
}
