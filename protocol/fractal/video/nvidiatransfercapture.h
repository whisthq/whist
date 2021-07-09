#ifndef NVIDIA_TRANSFER_CAPTURE_H
#define NVIDIA_TRANSFER_CAPTURE_H

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include "nvidia_encode.h"
#include "x11nvidiacapture.h"

int nvidia_start_transfer_context(NvidiaCaptureDevice* device, NvidiaEncoder* encoder);

int nvidia_close_transfer_context(NvidiaEncoder* encoder);

#endif
