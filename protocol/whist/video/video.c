/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file video.c
 * @brief Common video helper functions.
 */
#include "video.h"

const char *video_frame_type_string(VideoFrameType type) {
    switch (type) {
        case VIDEO_FRAME_TYPE_NORMAL:
            return "normal frame";
        case VIDEO_FRAME_TYPE_INTRA:
            return "intra frame";
        case VIDEO_FRAME_TYPE_CREATE_LONG_TERM:
            return "frame creating long term reference";
        case VIDEO_FRAME_TYPE_REFER_LONG_TERM:
            return "frame using long term reference";
        default:
            return "invalid frame type";
    }
}
