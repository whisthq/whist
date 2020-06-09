#ifndef CPU_CAPTURE_TRANSFER_H
#define CPU_CAPTURE_TRANSFER_H

#include "videoencode.h"
#if defined(_WIN32)
#include "dxgicapture.h"
#else
#include "x11capture.h"
#endif

bool cpu_transfer_capture(CaptureDevice* device, video_encoder_t* encoder);

#endif  // CPU_CAPTURE_TRANSFER_H