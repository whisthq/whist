#include "cpucapturetransfer.h"

bool cpu_transfer_capture(CaptureDevice* device, video_encoder_t* encoder) {
    if (TransferScreen(device)) {
        LOG_ERROR("Unable to transfer screen to CPU buffer.");
        return false;
    }
    if (video_encoder_frame_intake(encoder, device->frame_data,
                                   device->pitch)) {
        LOG_ERROR("Unable to load data to AVFrame");
        return false;
    }
    return true;
}