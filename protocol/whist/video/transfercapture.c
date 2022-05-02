#include "transfercapture.h"
#include "capture/capture.h"

int transfer_capture(CaptureDevice* device, VideoEncoder* encoder, bool* force_iframe) {
    if (device == NULL) {
        LOG_ERROR("device is null");
    }
    if (encoder == NULL) {
        LOG_ERROR("encoder is null");
    }
    if (device->width != encoder->in_width || device->height != encoder->in_height) {
        LOG_ERROR(
            "Tried to pass in a captured frame of dimension %dx%d, "
            "into an encoder that accepts %dx%d as input",
            device->width, device->height, encoder->in_width, encoder->in_height);
        return -1;
    }
    if (transfer_screen(device)) {
        LOG_ERROR("Unable to transfer screen to CPU buffer.");
        return -1;
    }

#ifdef __linux__
    // Handle transfer capture for nvidia capture device
    // check if we need to switch our active encoder
    if (encoder->active_encoder == NVIDIA_ENCODER) {
        if (USING_NVIDIA_CAPTURE) {
            // We need to account for switching contexts
            NvidiaEncoder* old_encoder = encoder->nvidia_encoders[encoder->active_encoder_idx];
            if (old_encoder->cuda_context != *get_video_thread_cuda_context_ptr()) {
                LOG_INFO("Switching to other Nvidia encoder!");
                encoder->active_encoder_idx = encoder->active_encoder_idx == 1 ? 0 : 1;
                if (encoder->nvidia_encoders[encoder->active_encoder_idx] == NULL) {
                    encoder->nvidia_encoders[encoder->active_encoder_idx] = create_nvidia_encoder(
                        old_encoder->bitrate, old_encoder->codec_type, old_encoder->width,
                        old_encoder->height, old_encoder->vbv_size,
                        *get_video_thread_cuda_context_ptr());
                }
                *force_iframe = true;
            }
        }
        RegisteredResource resource_to_register = {0};
        resource_to_register.width = device->width;
        resource_to_register.height = device->height;
        resource_to_register.pitch = device->pitch;
        resource_to_register.device_type = device->last_capture_device;
        resource_to_register.texture_pointer = device->frame_data;
        return nvidia_encoder_frame_intake(encoder->nvidia_encoders[encoder->active_encoder_idx],
                                           resource_to_register);
    }
#endif  // linux

    if (encoder->active_encoder != FFMPEG_ENCODER) {
        LOG_INFO("Switching back to FFmpeg encoder for use with X11 capture!");
        encoder->active_encoder = FFMPEG_ENCODER;
        *force_iframe = true;
    }

    // CPU transfer, if hardware transfer doesn't work
    static int times_measured = 0;
    static double time_spent = 0.0;

    WhistTimer cpu_transfer_timer;
    start_timer(&cpu_transfer_timer);

#ifdef _WIN32
    if (ffmpeg_encoder_frame_intake(encoder->ffmpeg_encoder, device->frame_data, device->pitch)) {
#else  // __linux
    if (ffmpeg_encoder_frame_intake(encoder->ffmpeg_encoder, device->x11_capture_device->frame_data,
                                    device->x11_capture_device->pitch)) {
#endif
        LOG_ERROR("Unable to load data to AVFrame");
        return -1;
    }

    times_measured++;
    time_spent += get_timer(&cpu_transfer_timer);

    if (times_measured == 10) {
        LOG_INFO("Average time transferring frame from capture to encoder frame on CPU: %f",
                 time_spent / times_measured);
        times_measured = 0;
        time_spent = 0.0;
    }

    return 0;
}
