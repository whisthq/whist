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
#include <assert.h>

/*
============================
Public Constants
============================
*/
#define STATISTICS_FREQUENCY_IN_SEC 10

/*
============================
Public Structures
============================
*/
typedef struct {
    const char *key;
    bool is_max_needed;
    bool is_min_needed;
    bool average_over_time;
} StatisticInfo;

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
void whist_init_statistic_logger(uint32_t num_metrics, StatisticInfo *statistic_info, int interval);

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
