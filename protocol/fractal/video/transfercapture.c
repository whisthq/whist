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
    if (encoder->active_encoder == NVIDIA_ENCODER) {
        switch (device->active_capture_device) {
            case NVIDIA_DEVICE:
                return nvidia_start_transfer_context(
                    device->nvidia_capture_device,
                    encoder->nvidia_encoders[encoder->active_encoder_idx]);
            case X11_DEVICE:
                return x11_start_transfer_context(
                    device->x11_capture_device,
                    encoder->nvidia_encoders[encoder->active_encoder_idx]);
            default:
                LOG_ERROR("Device type unknown: %d", device->active_capture_device);
                return -1;
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
    if (encoder->active_encoder == NVIDIA_ENCODER) {
        success =
            nvidia_close_transfer_context(encoder->nvidia_encoders[encoder->active_encoder_idx]);
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

#ifdef __linux__
    // Handle transfer capture for nvidia capture device
    // check if we need to switch our active encoder
    if (encoder->active_encoder == NVIDIA_ENCODER) {
        NvidiaEncoder* old_encoder = encoder->nvidia_encoders[encoder->active_encoder_idx];
        if (old_encoder->cuda_context != *get_video_thread_cuda_context_ptr()) {
                    LOG_INFO("Switching to other Nvidia encoder!");
            encoder->active_encoder_idx = 0 ? encoder->active_encoder_idx == 1 : 1;
            encoder->nvidia_encoders[encoder->active_encoder_idx] = create_nvidia_encoder(
                old_encoder->encoder_params.encodeConfig->rcParams.averageBitRate,
                old_encoder->codec_type, old_encoder->width, old_encoder->height,
                *get_video_thread_cuda_context_ptr());
                    encoder->active_encoder = NVIDIA_ENCODER;
                    video_encoder_set_iframe(encoder);
                    start_transfer_context(device, encoder);
                }
            }
        }
                nvidia_encoder_frame_intake(encoder->nvidia_encoders[encoder->active_encoder_idx], device->width, device->height);
                return 0;
    }
#endif

            if (encoder->active_encoder != FFMPEG_ENCODER) {
                LOG_INFO("Switching back to FFmpeg encoder for use with X11 capture!");
                encoder->active_encoder = FFMPEG_ENCODER;
                video_encoder_set_iframe(encoder);
            }
            // Handle the normal active_capture_device
#ifdef _WIN32
            // Handle cuda optimized transfer on windows
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

            if (transfer_screen(device)) {
                LOG_ERROR("Unable to transfer screen to CPU buffer.");
                return -1;
            }
#ifdef _WIN32
            if (ffmpeg_encoder_frame_intake(encoder->ffmpeg_encoder, device->frame_data,
                                            device->pitch)) {
#else  // __linux
    if (ffmpeg_encoder_frame_intake(encoder->ffmpeg_encoder, device->x11_capture_device->frame_data,
                                    device->x11_capture_device->pitch)) {
#endif
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
