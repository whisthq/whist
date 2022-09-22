#ifndef LOG_STATISTIC_H
#define LOG_STATISTIC_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file log_statistic.h
 * @brief This contains logging utils to log repeated statistics, where we
 *        don't want to print every time.
============================
Usage
============================

Call log_double_statistic(index, value) repeatedly within a loop with the metric's index to store a
double `val`. If it's been STATISTICS_FREQUENCY_IN_SEC, the call will also print some statistics
about the stored values for each key and flush the data.
*/

#include <stdbool.h>
#include <stdint.h>
#include <whist/debug/plotter.h>

/*
============================
Public Constants
============================
*/
#define STATISTICS_FREQUENCY_IN_SEC 10

#ifndef LOG_DATA_FOR_PLOTTER
#define LOG_DATA_FOR_PLOTTER false
#endif

// The size of the plot data can get big relatively quickly. A usual 2mins-long E2E test produces
// ~5MB of data for each of the client and the server. Here, we are setting the size of the buffer
// to 100MB to have a reasonable amount of space. If size scales linearly, 100MB will allow us to
// run tests of ~40mins duration.
#define PLOT_DATA_SIZE 100000000
// Save the plotting data in the same folder (on the mandelbox) as the {client,server}.log files to
// facilitate retrieval. This value should be changed when testing on a local machine.
#define PLOT_DATA_FILENAME "/usr/share/whist/plot_data.json"

/*
============================
Public enums
============================
*/

typedef enum {
    // Common metrics
    NETWORK_RTT_UDP,

    // Server side metrics
    AUDIO_ENCODE_TIME,
    CLIENT_HANDLE_USERINPUT_TIME,
    NETWORK_THROTTLED_PACKET_DELAY,
    NETWORK_THROTTLED_PACKET_DELAY_RATE,
    VIDEO_CAPTURE_CREATE_TIME,
    VIDEO_CAPTURE_UPDATE_TIME,
    VIDEO_CAPTURE_SCREEN_TIME,
    VIDEO_CAPTURE_TRANSFER_TIME,
    VIDEO_ENCODER_UPDATE_TIME,
    VIDEO_ENCODE_TIME,
    VIDEO_FPS_SENT,
    VIDEO_FRAMES_SKIPPED_IN_CAPTURE,
    VIDEO_FRAME_SIZE,
    VIDEO_FRAME_PROCESSING_TIME,
    VIDEO_GET_CURSOR_TIME,
    VIDEO_FRAME_QP,
    VIDEO_FRAME_SATD,
    VIDEO_NUM_RECOVERY_FRAMES,
    VIDEO_SEND_TIME,
    DBUS_MSGS_RECEIVED,
    SERVER_CPU_USAGE,

    // Client side metrics
    AUDIO_RECEIVE_TIME,
    AUDIO_FRAMES_SKIPPED,
    NETWORK_READ_PACKET_TCP,
    NETWORK_READ_PACKET_UDP,
    SERVER_HANDLE_MESSAGE_TCP,
    VIDEO_AVCODEC_RECEIVE_TIME,
    VIDEO_AV_HWFRAME_TRANSFER_TIME,
    VIDEO_CURSOR_UPDATE_TIME,
    VIDEO_DECODE_SEND_PACKET_TIME,
    VIDEO_DECODE_GET_FRAME_TIME,
    VIDEO_FPS_RENDERED,
    VIDEO_PIPELINE_LATENCY,
    VIDEO_CAPTURE_LATENCY,
    VIDEO_E2E_LATENCY,
    VIDEO_RECEIVE_TIME,
    VIDEO_RENDER_TIME,
    VIDEO_TIME_BETWEEN_FRAMES,
    NOTIFICATIONS_RECEIVED,
    CLIENT_CPU_USAGE,

    NUM_METRICS,
} Metrics;

/*
============================
Macros
============================
*/

/**
 * @brief                          This macro runs the code in `line`, timing it, and then logging
 *                                 the result with `log_double_statistic`. It depends on a `clock`
 *                                 being passed in as a parameter to compute the time delta.
 */
#define TIME_RUN(line, name, timer) \
    start_timer(&timer);            \
    line;                           \
    log_double_statistic(name, get_timer(&timer) * MS_IN_SECOND);

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the statistic logger.
 */
void whist_init_statistic_logger(int interval);

/**
 * @brief                          Note a specific stat value associated with a key. If enough time
 *                                 has passed, also prints all statistics and flushes the data
 *                                 store.
 *
 * @param index                    The predefined index of the metric.
 *
 * @param val                      The double value to store.
 */
void log_double_statistic(uint32_t index, double val);

/**
 * @brief                          Destroy the statistic logger.
 */
void destroy_statistic_logger(void);

#endif  // LOG_STATISTIC_H
