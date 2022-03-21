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

static StatisticInfo statistic_info[NUM_METRICS];

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

static void init_statistics_info(void) {
    // (StatisticInfo){"key", is_max_needed, is_min_needed, average_over_time};
    // Common metrics
    statistic_info[NETWORK_RTT_UDP] = (StatisticInfo){"NETWORK_RTT_UDP", true, true, false};

    // Server side metrics
    statistic_info[AUDIO_ENCODE_TIME] = (StatisticInfo){"AUDIO_ENCODE_TIME", true, false, false};
    statistic_info[CLIENT_HANDLE_USERINPUT_TIME] =
        (StatisticInfo){"HANDLE_USERINPUT_TIME", true, false, false};
    statistic_info[NETWORK_THROTTLED_PACKET_DELAY] =
        (StatisticInfo){"THROTTLED_PACKET_DELAY", true, false, false};
    statistic_info[NETWORK_THROTTLED_PACKET_DELAY_RATE] =
        (StatisticInfo){"THROTTLED_PACKET_DELAY_RATE", true, false, false};
    statistic_info[NETWORK_THROTTLED_PACKET_DELAY_LOOPS] =
        (StatisticInfo){"THROTTLED_PACKET_DELAY_LOOPS", true, false, false};
    statistic_info[VIDEO_CAPTURE_CREATE_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_CREATE_TIME", true, false, false};
    statistic_info[VIDEO_CAPTURE_UPDATE_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_UPDATE_TIME", true, false, false};
    statistic_info[VIDEO_CAPTURE_SCREEN_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_SCREEN_TIME", true, false, false};
    statistic_info[VIDEO_CAPTURE_TRANSFER_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_TRANSFER_TIME", true, false, false};
    statistic_info[VIDEO_ENCODER_UPDATE_TIME] =
        (StatisticInfo){"VIDEO_ENCODER_UPDATE_TIME", true, false, false};
    statistic_info[VIDEO_ENCODE_TIME] = (StatisticInfo){"VIDEO_ENCODE_TIME", true, false, false};
    statistic_info[VIDEO_FPS_SENT] = (StatisticInfo){"VIDEO_FPS_SENT", false, false, true};
    statistic_info[VIDEO_FPS_SKIPPED_IN_CAPTURE] =
        (StatisticInfo){"VIDEO_FPS_SKIPPED_IN_CAPTURE", false, false, true};
    statistic_info[VIDEO_FRAME_SIZE] = (StatisticInfo){"VIDEO_FRAME_SIZE", true, false, false};
    statistic_info[VIDEO_FRAME_PROCESSING_TIME] =
        (StatisticInfo){"VIDEO_FRAME_PROCESSING_TIME", true, false, false};
    statistic_info[VIDEO_GET_CURSOR_TIME] = (StatisticInfo){"GET_CURSOR_TIME", true, false, false};
    statistic_info[VIDEO_INTER_FRAME_QP] =
        (StatisticInfo){"VIDEO_INTER_FRAME_QP", true, false, false};
    statistic_info[VIDEO_INTRA_FRAME_QP] =
        (StatisticInfo){"VIDEO_INTRA_FRAME_QP", true, false, false};
    statistic_info[VIDEO_SEND_TIME] = (StatisticInfo){"VIDEO_SEND_TIME", true, false, false};
    statistic_info[DBUS_MSGS_RECEIVED] = (StatisticInfo){"DBUS_MSGS_RECEIVED", false, false, true};
    statistic_info[SERVER_CPU_USAGE] = (StatisticInfo){"SERVER_CPU_USAGE", false, false, false};

    // Client side metrics
    statistic_info[AUDIO_RECEIVE_TIME] = (StatisticInfo){"AUDIO_RECEIVE_TIME", true, false, false};
    statistic_info[AUDIO_UPDATE_TIME] = (StatisticInfo){"AUDIO_UPDATE_TIME", true, false, false};
    statistic_info[AUDIO_FPS_SKIPPED] = (StatisticInfo){"AUDIO_FPS_SKIPPED", false, false, true};
    statistic_info[NETWORK_READ_PACKET_TCP] =
        (StatisticInfo){"READ_PACKET_TIME_TCP", true, false, false};
    statistic_info[NETWORK_READ_PACKET_UDP] =
        (StatisticInfo){"READ_PACKET_TIME_UDP", true, false, false};
    statistic_info[SERVER_HANDLE_MESSAGE_TCP] =
        (StatisticInfo){"HANDLE_SERVER_MESSAGE_TIME_TCP", true, false, false};
    statistic_info[SERVER_HANDLE_MESSAGE_UDP] =
        (StatisticInfo){"HANDLE_SERVER_MESSAGE_TIME_UDP", true, false, false};
    statistic_info[VIDEO_AVCODEC_RECEIVE_TIME] =
        (StatisticInfo){"AVCODEC_RECEIVE_TIME", true, false, false};
    statistic_info[VIDEO_AV_HWFRAME_TRANSFER_TIME] =
        (StatisticInfo){"AV_HWFRAME_TRANSFER_TIME", true, false, false};
    statistic_info[VIDEO_CURSOR_UPDATE_TIME] =
        (StatisticInfo){"CURSOR_UPDATE_TIME", true, false, false};
    statistic_info[VIDEO_DECODE_SEND_PACKET_TIME] =
        (StatisticInfo){"VIDEO_DECODE_SEND_PACKET_TIME", true, false, false};
    statistic_info[VIDEO_DECODE_GET_FRAME_TIME] =
        (StatisticInfo){"VIDEO_DECODE_GET_FRAME_TIME", true, false, false};
    statistic_info[VIDEO_FPS_RENDERED] = (StatisticInfo){"VIDEO_FPS_RENDERED", false, false, true};
    statistic_info[VIDEO_E2E_LATENCY] =
        (StatisticInfo){"VIDEO_END_TO_END_LATENCY", true, true, false};
    statistic_info[VIDEO_RECEIVE_TIME] = (StatisticInfo){"VIDEO_RECEIVE_TIME", true, false, false};
    statistic_info[VIDEO_RENDER_TIME] = (StatisticInfo){"VIDEO_RENDER_TIME", true, false, false};
    statistic_info[VIDEO_TIME_BETWEEN_FRAMES] =
        (StatisticInfo){"VIDEO_TIME_BETWEEN_FRAMES", true, false, false};
    statistic_info[VIDEO_UPDATE_TIME] = (StatisticInfo){"VIDEO_UPDATE_TIME", true, false, false};
    statistic_info[NOTIFICATIONS_RECEIVED] =
        (StatisticInfo){"NOTIFICATIONS_RECEIVED", false, false, true};
    statistic_info[CLIENT_CPU_USAGE] = (StatisticInfo){"CLIENT_CPU_USAGE", false, false, false};
}
/*
============================
Public Function Implementations
============================
*/

void whist_init_statistic_logger(int interval) {
    init_statistics_info();
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
