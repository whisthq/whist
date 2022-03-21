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

static StatisticInfo statistic_info[NUM_METRICS] = {
    // {"key", is_max_needed, is_min_needed, average_over_time};
    // Common metrics
    [NETWORK_RTT_UDP] = {"NETWORK_RTT_UDP", true, true, false},

    // Server side metrics
    [AUDIO_ENCODE_TIME] = {"AUDIO_ENCODE_TIME", true, false, false},
    [CLIENT_HANDLE_USERINPUT_TIME] = {"HANDLE_USERINPUT_TIME", true, false, false},
    [NETWORK_THROTTLED_PACKET_DELAY] = {"THROTTLED_PACKET_DELAY", true, false, false},
    [NETWORK_THROTTLED_PACKET_DELAY_RATE] = {"THROTTLED_PACKET_DELAY_RATE", true, false, false},
    [NETWORK_THROTTLED_PACKET_DELAY_LOOPS] = {"THROTTLED_PACKET_DELAY_LOOPS", true, false, false},
    [VIDEO_CAPTURE_CREATE_TIME] = {"VIDEO_CAPTURE_CREATE_TIME", true, false, false},
    [VIDEO_CAPTURE_UPDATE_TIME] = {"VIDEO_CAPTURE_UPDATE_TIME", true, false, false},
    [VIDEO_CAPTURE_SCREEN_TIME] = {"VIDEO_CAPTURE_SCREEN_TIME", true, false, false},
    [VIDEO_CAPTURE_TRANSFER_TIME] = {"VIDEO_CAPTURE_TRANSFER_TIME", true, false, false},
    [VIDEO_ENCODER_UPDATE_TIME] = {"VIDEO_ENCODER_UPDATE_TIME", true, false, false},
    [VIDEO_ENCODE_TIME] = {"VIDEO_ENCODE_TIME", true, false, false},
    [VIDEO_FPS_SENT] = {"VIDEO_FPS_SENT", false, false, true},
    [VIDEO_FPS_SKIPPED_IN_CAPTURE] = {"VIDEO_FPS_SKIPPED_IN_CAPTURE", false, false, true},
    [VIDEO_FRAME_SIZE] = {"VIDEO_FRAME_SIZE", true, false, false},
    [VIDEO_FRAME_PROCESSING_TIME] = {"VIDEO_FRAME_PROCESSING_TIME", true, false, false},
    [VIDEO_GET_CURSOR_TIME] = {"GET_CURSOR_TIME", true, false, false},
    [VIDEO_INTER_FRAME_QP] = {"VIDEO_INTER_FRAME_QP", true, false, false},
    [VIDEO_INTRA_FRAME_QP] = {"VIDEO_INTRA_FRAME_QP", true, false, false},
    [VIDEO_SEND_TIME] = {"VIDEO_SEND_TIME", true, false, false},
    [DBUS_MSGS_RECEIVED] = {"DBUS_MSGS_RECEIVED", false, false, true},
    [SERVER_CPU_USAGE] = {"SERVER_CPU_USAGE", false, false, false},

    // Client side metrics
    [AUDIO_RECEIVE_TIME] = {"AUDIO_RECEIVE_TIME", true, false, false},
    [AUDIO_UPDATE_TIME] = {"AUDIO_UPDATE_TIME", true, false, false},
    [AUDIO_FPS_SKIPPED] = {"AUDIO_FPS_SKIPPED", false, false, true},
    [NETWORK_READ_PACKET_TCP] = {"READ_PACKET_TIME_TCP", true, false, false},
    [NETWORK_READ_PACKET_UDP] = {"READ_PACKET_TIME_UDP", true, false, false},
    [SERVER_HANDLE_MESSAGE_TCP] = {"HANDLE_SERVER_MESSAGE_TIME_TCP", true, false, false},
    [SERVER_HANDLE_MESSAGE_UDP] = {"HANDLE_SERVER_MESSAGE_TIME_UDP", true, false, false},
    [VIDEO_AVCODEC_RECEIVE_TIME] = {"AVCODEC_RECEIVE_TIME", true, false, false},
    [VIDEO_AV_HWFRAME_TRANSFER_TIME] = {"AV_HWFRAME_TRANSFER_TIME", true, false, false},
    [VIDEO_CURSOR_UPDATE_TIME] = {"CURSOR_UPDATE_TIME", true, false, false},
    [VIDEO_DECODE_SEND_PACKET_TIME] = {"VIDEO_DECODE_SEND_PACKET_TIME", true, false, false},
    [VIDEO_DECODE_GET_FRAME_TIME] = {"VIDEO_DECODE_GET_FRAME_TIME", true, false, false},
    [VIDEO_FPS_RENDERED] = {"VIDEO_FPS_RENDERED", false, false, true},
    [VIDEO_E2E_LATENCY] = {"VIDEO_END_TO_END_LATENCY", true, true, false},
    [VIDEO_RECEIVE_TIME] = {"VIDEO_RECEIVE_TIME", true, false, false},
    [VIDEO_RENDER_TIME] = {"VIDEO_RENDER_TIME", true, false, false},
    [VIDEO_TIME_BETWEEN_FRAMES] = {"VIDEO_TIME_BETWEEN_FRAMES", true, false, false},
    [VIDEO_UPDATE_TIME] = {"VIDEO_UPDATE_TIME", true, false, false},
    [NOTIFICATIONS_RECEIVED] = {"NOTIFICATIONS_RECEIVED", false, false, true},
    [CLIENT_CPU_USAGE] = {"CLIENT_CPU_USAGE", false, false, false},

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
        unsigned current_count;
        if (statistic_info[i].average_over_time) {
            if (all_statistics[i].sum == 0) continue;
            current_count = statistic_context.interval;
        } else {
            if (all_statistics[i].count == 0) continue;
            current_count = all_statistics[i].count;
        }

        LOG_METRIC("\"%s\" : %.1f, \"COUNT\": %u", statistic_info[i].key,
                   all_statistics[i].sum / current_count, current_count);

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
