/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file log_statistic.c
 * @brief This contains logging utils to log repeated statistics, where we
 *        don't want to print every time.
============================
Usage
============================

Call log_double_statistic(key, value) repeatedly within a loop with the same string `key` to store a
double `val`. If it's been STATISTICS_FREQUENCY_IN_SEC, the call will also print some statistics
about the stored values for each key and flush the data.
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include "logging.h"
#include "log_statistic.h"

#define LOG_STATISTICS true

static WhistMutex log_statistic_mutex;

static WhistTimer print_statistic_clock;

typedef enum { AVERAGE, AVERAGE_OVER_TIME, SUM } AggregationType;

typedef struct {
    const char *key;
    bool is_max_needed;
    bool is_min_needed;
    AggregationType aggregation_type;
} StatisticInfo;

static StatisticInfo statistic_info[NUM_METRICS] = {
    // {"key", is_max_needed, is_min_needed, aggregation_type};
    // Common metrics
    [NETWORK_RTT_UDP] = {"NETWORK_RTT_UDP", true, true, AVERAGE},

    // Server side metrics
    [AUDIO_ENCODE_TIME] = {"AUDIO_ENCODE_TIME", true, false, AVERAGE},
    [CLIENT_HANDLE_USERINPUT_TIME] = {"HANDLE_USERINPUT_TIME", true, false, AVERAGE},
    [NETWORK_THROTTLED_PACKET_DELAY] = {"THROTTLED_PACKET_DELAY", true, false, AVERAGE},
    [NETWORK_THROTTLED_PACKET_DELAY_RATE] = {"THROTTLED_PACKET_DELAY_RATE", true, false, AVERAGE},
    [NETWORK_THROTTLED_PACKET_DELAY_LOOPS] = {"THROTTLED_PACKET_DELAY_LOOPS", true, false, AVERAGE},
    [VIDEO_CAPTURE_CREATE_TIME] = {"VIDEO_CAPTURE_CREATE_TIME", true, false, AVERAGE},
    [VIDEO_CAPTURE_UPDATE_TIME] = {"VIDEO_CAPTURE_UPDATE_TIME", true, false, AVERAGE},
    [VIDEO_CAPTURE_SCREEN_TIME] = {"VIDEO_CAPTURE_SCREEN_TIME", true, false, AVERAGE},
    [VIDEO_CAPTURE_TRANSFER_TIME] = {"VIDEO_CAPTURE_TRANSFER_TIME", true, false, AVERAGE},
    [VIDEO_ENCODER_UPDATE_TIME] = {"VIDEO_ENCODER_UPDATE_TIME", true, false, AVERAGE},
    [VIDEO_ENCODE_TIME] = {"VIDEO_ENCODE_TIME", true, false, AVERAGE},
    [VIDEO_FPS_SENT] = {"VIDEO_FPS_SENT", false, false, AVERAGE_OVER_TIME},
    [VIDEO_FRAMES_SKIPPED_IN_CAPTURE] = {"VIDEO_FRAMES_SKIPPED_IN_CAPTURE", false, false, SUM},
    [VIDEO_FRAME_SIZE] = {"VIDEO_FRAME_SIZE", true, false, AVERAGE},
    [VIDEO_FRAME_PROCESSING_TIME] = {"VIDEO_FRAME_PROCESSING_TIME", true, false, AVERAGE},
    [VIDEO_GET_CURSOR_TIME] = {"GET_CURSOR_TIME", true, false, AVERAGE},
    [VIDEO_FRAME_QP] = {"VIDEO_FRAME_QP", true, false, AVERAGE},
    [VIDEO_FRAME_SATD] = {"VIDEO_FRAME_SATD", true, false, AVERAGE},
    [VIDEO_NUM_RECOVERY_FRAMES] = {"VIDEO_NUM_RECOVERY_FRAMES", false, false, SUM},
    [VIDEO_SEND_TIME] = {"VIDEO_SEND_TIME", true, false, AVERAGE},
    [DBUS_MSGS_RECEIVED] = {"DBUS_MSGS_RECEIVED", false, false, SUM},
    [SERVER_CPU_USAGE] = {"SERVER_CPU_USAGE", false, false, AVERAGE},

    // Client side metrics
    [AUDIO_RECEIVE_TIME] = {"AUDIO_RECEIVE_TIME", true, false, AVERAGE},
    [AUDIO_UPDATE_TIME] = {"AUDIO_UPDATE_TIME", true, false, AVERAGE},
    [AUDIO_FRAMES_SKIPPED] = {"AUDIO_FRAMES_SKIPPED", false, false, SUM},
    [NETWORK_READ_PACKET_TCP] = {"READ_PACKET_TIME_TCP", true, false, AVERAGE},
    [NETWORK_READ_PACKET_UDP] = {"READ_PACKET_TIME_UDP", true, false, AVERAGE},
    [SERVER_HANDLE_MESSAGE_TCP] = {"HANDLE_SERVER_MESSAGE_TIME_TCP", true, false, AVERAGE},
    [SERVER_HANDLE_MESSAGE_UDP] = {"HANDLE_SERVER_MESSAGE_TIME_UDP", true, false, AVERAGE},
    [VIDEO_AVCODEC_RECEIVE_TIME] = {"AVCODEC_RECEIVE_TIME", true, false, AVERAGE},
    [VIDEO_AV_HWFRAME_TRANSFER_TIME] = {"AV_HWFRAME_TRANSFER_TIME", true, false, AVERAGE},
    [VIDEO_CURSOR_UPDATE_TIME] = {"CURSOR_UPDATE_TIME", true, false, AVERAGE},
    [VIDEO_DECODE_SEND_PACKET_TIME] = {"VIDEO_DECODE_SEND_PACKET_TIME", true, false, AVERAGE},
    [VIDEO_DECODE_GET_FRAME_TIME] = {"VIDEO_DECODE_GET_FRAME_TIME", true, false, AVERAGE},
    [VIDEO_FPS_RENDERED] = {"VIDEO_FPS_RENDERED", false, false, AVERAGE_OVER_TIME},
    [VIDEO_PIPELINE_LATENCY] = {"VIDEO_PIPELINE_LATENCY", true, true, AVERAGE},
    [VIDEO_CAPTURE_LATENCY] = {"VIDEO_CAPTURE_LATENCY", true, true, AVERAGE},
    [VIDEO_E2E_LATENCY] = {"VIDEO_END_TO_END_LATENCY", true, true, AVERAGE},
    [VIDEO_RECEIVE_TIME] = {"VIDEO_RECEIVE_TIME", true, false, AVERAGE},
    [VIDEO_RENDER_TIME] = {"VIDEO_RENDER_TIME", true, false, AVERAGE},
    [VIDEO_TIME_BETWEEN_FRAMES] = {"VIDEO_TIME_BETWEEN_FRAMES", true, false, AVERAGE},
    [VIDEO_UPDATE_TIME] = {"VIDEO_UPDATE_TIME", true, false, AVERAGE},
    [NOTIFICATIONS_RECEIVED] = {"NOTIFICATIONS_RECEIVED", false, false, SUM},
    [CLIENT_CPU_USAGE] = {"CLIENT_CPU_USAGE", false, false, AVERAGE},

    [TEST1] = {"TEST1", true, false, AVERAGE},
    [TEST2] = {"TEST2", true, false, AVERAGE},
    [TEST3] = {"TEST3", true, false, AVERAGE},
    [TEST4] = {"TEST4", true, false, AVERAGE},

};

/*
============================
Custom Types
============================
*/

typedef struct StatisticData {
    double sum;
    unsigned count;
    double min;
    double max;
} StatisticData;

typedef struct {
    StatisticData *all_statistics;
    int interval;
} StatisticContext;

static StatisticContext statistic_context;

/*
============================
Private Functions
============================
*/

static void unsafe_print_statistics(void);

/*
============================
Private Function Implementations
============================
*/

static void unsafe_print_statistics(void) {
#ifdef LOG_STATISTICS
    StatisticData *all_statistics = statistic_context.all_statistics;
    // Flush all stored statistics
    for (uint32_t i = 0; i < NUM_METRICS; i++) {
        if (all_statistics[i].count == 0) continue;
        if (statistic_info[i].aggregation_type == SUM) {
            LOG_METRIC("\"%s\" : %.1f", statistic_info[i].key, all_statistics[i].sum);
        } else {
            unsigned current_count;
            if (statistic_info[i].aggregation_type == AVERAGE_OVER_TIME) {
                current_count = statistic_context.interval;
            } else {
                current_count = all_statistics[i].count;
            }
            LOG_METRIC("\"%s\" : %.1f, \"COUNT\": %u", statistic_info[i].key,
                       all_statistics[i].sum / current_count, current_count);
        }

        if (statistic_info[i].is_max_needed)
            LOG_METRIC("\"MAX_%s\" : %.3f", statistic_info[i].key, all_statistics[i].max);
        if (statistic_info[i].is_min_needed)
            LOG_METRIC("\"MIN_%s\" : %.3f", statistic_info[i].key, all_statistics[i].min);

        all_statistics[i].sum = 0;
        all_statistics[i].count = 0;
        all_statistics[i].min = 0;
        all_statistics[i].max = 0;
    }

    start_timer(&print_statistic_clock);
#endif
}

/*
============================
Public Function Implementations
============================
*/

void whist_init_statistic_logger(int interval) {
    log_statistic_mutex = whist_create_mutex();
    statistic_context.interval = interval;
    statistic_context.all_statistics =
        malloc(NUM_METRICS * sizeof(*statistic_context.all_statistics));
    if (statistic_context.all_statistics == NULL) {
        LOG_ERROR("statistic_context.all_statistics malloc failed");
        return;
    }
    start_timer(&print_statistic_clock);
    for (uint32_t i = 0; i < NUM_METRICS; i++) {
        statistic_context.all_statistics[i].sum = 0;
        statistic_context.all_statistics[i].count = 0;
        statistic_context.all_statistics[i].max = 0;
        statistic_context.all_statistics[i].min = 0;
    }
}

void log_double_statistic(uint32_t index, double val) {
    StatisticData *all_statistics = statistic_context.all_statistics;
    if (all_statistics == NULL) {
        LOG_ERROR("all_statistics is NULL");
        return;
    }
    if (index >= NUM_METRICS) {
        LOG_ERROR("index is out of bounds. index = %d, num_metrics = %d", index, NUM_METRICS);
        return;
    }
    whist_lock_mutex(log_statistic_mutex);
    if (all_statistics[index].count == 0) {
        all_statistics[index].min = val;
        all_statistics[index].max = val;
    }

    all_statistics[index].count++;

    if (val > all_statistics[index].max) {
        all_statistics[index].max = val;
    } else if (val < all_statistics[index].min) {
        all_statistics[index].min = val;
    }

    all_statistics[index].sum += val;

    if (get_timer(&print_statistic_clock) > statistic_context.interval) {
        unsafe_print_statistics();
    }
    whist_unlock_mutex(log_statistic_mutex);
}

void destroy_statistic_logger(void) {
    free(statistic_context.all_statistics);
    memset((void *)&statistic_context, 0, sizeof(statistic_context));
    whist_destroy_mutex(log_statistic_mutex);
}
