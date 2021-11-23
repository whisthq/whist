#ifndef SERVER_METRICS_H
#define SERVER_METRICS_H
/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file metrics.h
 * @brief This file contains all header code related to logging server-side metrics.
 */

#include <stdbool.h>

/*
============================
Public Macros
============================
*/
#define METRIC_REPORTING_INTERVAL 5

/*
============================
Public Structs
============================
*/
typedef struct {
    int frames_sent;
    int frames_skipped_in_capture;
} VideoMetrics;

typedef struct {
    VideoMetrics video;
} Metrics;

/*
============================
External Variables
============================
*/
extern Metrics metrics;

/*
============================
Public Functions
============================
*/
int multithreaded_metrics(void* opaque);

#endif  // SERVER_METRICS_H
