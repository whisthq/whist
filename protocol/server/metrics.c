/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file metrics.c
 * @brief This file contains all code that log the server-side metrics.
 */

#include "metrics.h"
#include <fractal/core/fractal.h>
#include <fractal/utils/threads.h>

extern volatile bool exiting;

Metrics metrics = {0};

int multithreaded_metrics(void* opaque) {
    int interval = *(int*)opaque;
    while (!exiting) {
        Metrics prev_metrics = metrics;
        fractal_sleep(interval * MS_IN_SECOND);
        float video_fps_sent =
            (metrics.video.frames_sent - prev_metrics.video.frames_sent) / (float)interval;
        float video_fps_skipped_capture = (metrics.video.frames_skipped_in_capture -
                                           prev_metrics.video.frames_skipped_in_capture) /
                                          (float)interval;
        LOG_METRIC("\"video_fps_sent\" : %.1f \"video_fps_skipped_capture\" : %.1f", video_fps_sent,
                   video_fps_skipped_capture);
    }
    return 0;
}
