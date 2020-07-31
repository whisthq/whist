#include "cpucapturetransfer.h"

int cpu_transfer_capture(CaptureDevice* device, video_encoder_t* encoder) {
#ifndef _WIN32
    if (device->using_nvidia) {
        encoder->encoded_frame_data = device->nvidia_capture_device.frame;
	encoder->encoded_frame_size = device->nvidia_capture_device.size;
	encoder->is_iframe = device->nvidia_capture_device.is_iframe;
        encoder->already_captured = true;
    } else {
        encoder->already_captured = false;
    }
#else
    encoder->already_captured = false;
#endif

    static int times_measured = 0;
    static double time_spent = 0.;

    clock cpu_transfer_timer;
    StartTimer(&cpu_transfer_timer);

    if (TransferScreen(device)) {
        LOG_ERROR("Unable to transfer screen to CPU buffer.");
        return -1;
    }
    if (video_encoder_frame_intake(encoder, device->frame_data, device->pitch)) {
        LOG_ERROR("Unable to load data to AVFrame");
        return -1;
    }

    times_measured++;
    time_spent += GetTimer(cpu_transfer_timer);

    if (times_measured == 10) {
        LOG_INFO("Average time transferring dxgi data to encoder frame on CPU: %f",
                 time_spent / times_measured);
        times_measured = 0;
        time_spent = 0.0;
    }
    return 0;
}
