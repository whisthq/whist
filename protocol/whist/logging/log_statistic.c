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
    double offset;
    double sum_of_squares;
    double sum_with_offset;
} StatisticData;

typedef struct {
    StatisticInfo *statistic_info;
    StatisticData *all_statistics;
    uint32_t num_metrics;
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
    StatisticInfo *statistic_info = statistic_context.statistic_info;
    // Flush all stored statistics
    for (uint32_t i = 0; i < statistic_context.num_metrics; i++) {
        unsigned current_count;
        if (statistic_info[i].average_over_time) {
            current_count = statistic_context.interval;
        } else {
            if (all_statistics[i].count == 0) continue;
            current_count = all_statistics[i].count;
        }

        LOG_METRIC("\"%s\" : %u,%.2f,%.2f,%.2f", statistic_info[i].key, current_count,
                   all_statistics[i].sum, all_statistics[i].sum_of_squares,
                   all_statistics[i].sum_with_offset);
        assert(current_count >= 0);
        assert(all_statistics[i].sum >= 0);
        assert(all_statistics[i].sum_of_squares >= 0);

        if (statistic_info[i].is_max_needed)
            LOG_METRIC("\"MAX_%s\" : %.2f", statistic_info[i].key, all_statistics[i].max);
        if (statistic_info[i].is_min_needed)
            LOG_METRIC("\"MIN_%s\" : %.2f", statistic_info[i].key, all_statistics[i].min);

        all_statistics[i].sum = 0;
        all_statistics[i].count = 0;
        all_statistics[i].min = 0;
        all_statistics[i].max = 0;
        all_statistics[i].sum_of_squares = 0;
        all_statistics[i].sum_with_offset = 0;
    }

    start_timer(&print_statistic_clock);
#endif
}

/*
============================
Public Function Implementations
============================
*/

void whist_init_statistic_logger(uint32_t num_metrics, StatisticInfo *statistic_info,
                                 int interval) {
    if (statistic_info == NULL) {
        LOG_ERROR("StatisticInfo is NULL");
        return;
    }
    log_statistic_mutex = whist_create_mutex();
    statistic_context.statistic_info = statistic_info;
    statistic_context.num_metrics = num_metrics;
    statistic_context.interval = interval;
    statistic_context.all_statistics =
        malloc(num_metrics * sizeof(*statistic_context.all_statistics));
    if (statistic_context.all_statistics == NULL) {
        LOG_ERROR("statistic_context.all_statistics malloc failed");
        return;
    }
    start_timer(&print_statistic_clock);
    for (uint32_t i = 0; i < num_metrics; i++) {
        statistic_context.all_statistics[i].sum = 0;
        statistic_context.all_statistics[i].count = 0;
        statistic_context.all_statistics[i].max = 0;
        statistic_context.all_statistics[i].min = 0;
        statistic_context.all_statistics[i].offset = 0;
        statistic_context.all_statistics[i].sum_of_squares = 0;
        statistic_context.all_statistics[i].sum_with_offset = 0;
    }
}

void log_double_statistic(uint32_t index, double val) {
    StatisticData *all_statistics = statistic_context.all_statistics;
    if (all_statistics == NULL) {
        LOG_ERROR("all_statistics is NULL");
        return;
    }
    if (index >= statistic_context.num_metrics) {
        LOG_ERROR("index is out of bounds. index = %d, num_metrics = %d", index,
                  statistic_context.num_metrics);
        return;
    }
    whist_lock_mutex(log_statistic_mutex);
    if (all_statistics[index].count == 0) {
        all_statistics[index].min = val;
        all_statistics[index].max = val;
        all_statistics[index].offset = val;
    }

    all_statistics[index].count++;

    if (val > all_statistics[index].max) {
        all_statistics[index].max = val;
    } else if (val < all_statistics[index].min) {
        all_statistics[index].min = val;
    }

    all_statistics[index].sum += val;
    all_statistics[index].sum_of_squares +=
        (val - all_statistics[index].offset) * (val - all_statistics[index].offset);
    all_statistics[index].sum_with_offset += (val - all_statistics[index].offset);

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
