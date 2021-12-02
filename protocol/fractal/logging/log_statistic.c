/**
 * Copyright 2021 Whist Technologies, Inc.
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

#include <fractal/core/whist.h>
#include "logging.h"
#include "log_statistic.h"

#define LOG_STATISTICS true

static WhistMutex log_statistic_mutex;

static clock print_statistic_clock;

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

void unsafe_print_statistics();

/*
============================
Private Function Implementations
============================
*/

void unsafe_print_statistics() {
#ifdef LOG_STATISTICS
    StatisticData *all_statistics = statistic_context.all_statistics;
    StatisticInfo *statistic_info = statistic_context.statistic_info;
    // Flush all stored statistics
    for (uint32_t i = 0; i < statistic_context.num_metrics; i++) {
        float avg;
        if (statistic_info[i].average_over_time) {
            avg = all_statistics[i].sum / statistic_context.interval;
        } else {
            if (all_statistics[i].count == 0) continue;
            avg = all_statistics[i].sum / all_statistics[i].count;
        }
        LOG_METRIC("\"%s\" : %.2f", statistic_info[i].key, avg);
        if (statistic_info[i].is_max_needed)
            LOG_METRIC("\"MAX_%s\" : %.2f", statistic_info[i].key, all_statistics[i].max);
        if (statistic_info[i].is_min_needed)
            LOG_METRIC("\"MIN_%s\" : %.2f", statistic_info[i].key, all_statistics[i].min);

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
    }

    all_statistics[index].sum += val;
    all_statistics[index].count++;
    if (val > all_statistics[index].max) {
        all_statistics[index].max = val;
    } else if (val < all_statistics[index].min) {
        all_statistics[index].min = val;
    }

    if (get_timer(print_statistic_clock) > statistic_context.interval) {
        unsafe_print_statistics();
    }
    whist_unlock_mutex(log_statistic_mutex);
}

void destroy_statistic_logger() {
    memset((void *)&statistic_context, 0, sizeof(statistic_context));
    whist_destroy_mutex(log_statistic_mutex);
}
