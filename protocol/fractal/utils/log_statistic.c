/**
 * Copyright Fractal Computers, Inc. 2021
 * @file log_statistic.c
 * @brief This contains logging utils to log repeated statistics, where we
 *        don't want to print every time.
============================
Usage
============================

Call log_double_statistic(key, value) repeatedly within a loop with the same string `key` to store a
double `val`. If it's been PRINTING_FREQUENCY_IN_SEC, the call will also print some statistics about
the stored values for each key (up to MAX_DIFFERENT_STATISTICS keys) and flush the data. This can
also be done manually with print_statistics().
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>
#include "logging.h"
#include "log_statistic.h"

#define MAX_DIFFERENT_STATISTICS 64
#define MAX_KEY_LENGTH 128
#define PRINTING_FREQUENCY_IN_SEC 10

#define LOG_STATISTICS true

static FractalMutex log_statistic_mutex;

static clock print_statistic_clock;

/*
============================
Custom Types
============================
*/

typedef struct StatisticData {
    char key[MAX_KEY_LENGTH];
    double sum;
    unsigned count;
    double min;
    double max;
} StatisticData;

static StatisticData all_statistics[MAX_DIFFERENT_STATISTICS];

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
    if (all_statistics[0].count == 0) {
        // There are no statistics to print currently
        fractal_unlock_mutex(log_statistic_mutex);
        return;
    }

    // Flush all stored statistics
    for (int i = 0; i < MAX_DIFFERENT_STATISTICS && all_statistics[i].count != 0; i++) {
        LOG_INFO("STATISTIC—%s: avg %.2f, min %.2f, max %.2f (%u samples)", all_statistics[i].key,
                 all_statistics[i].sum / all_statistics[i].count, all_statistics[i].min,
                 all_statistics[i].max, all_statistics[i].count);

        all_statistics[i].key[0] = '\0';
        all_statistics[i].sum = 0;
        all_statistics[i].count = 0;
    }

    start_timer(&print_statistic_clock);
#endif
}

/*
============================
Public Function Implementations
============================
*/

void init_statistic_logger() {
    log_statistic_mutex = fractal_create_mutex();
    start_timer(&print_statistic_clock);
    for (int i = 0; i < MAX_DIFFERENT_STATISTICS; i++) {
        all_statistics[i].key[0] = '\0';
        all_statistics[i].sum = 0;
        all_statistics[i].count = 0;
    }
}

void log_double_statistic(const char* key, double val) {
    fractal_lock_mutex(log_statistic_mutex);
    int index = -1;
    for (int i = 0; i < MAX_DIFFERENT_STATISTICS; i++) {
        if (all_statistics[i].count == 0) {
            // This is a new key
            index = i;
            safe_strncpy(all_statistics[index].key, key, MAX_KEY_LENGTH);
            all_statistics[index].min = val;
            all_statistics[index].max = val;
            break;
        } else if (strcmp(all_statistics[i].key, key) == 0) {
            // This is an existing key
            index = i;
            break;
        }
    }

    if (index == -1) {
        LOG_ERROR("Too many different statistics being logged");
        fractal_unlock_mutex(log_statistic_mutex);
        return;
    }

    all_statistics[index].sum += val;
    all_statistics[index].count++;
    if (val > all_statistics[index].max) {
        all_statistics[index].max = val;
    } else if (val < all_statistics[index].min) {
        all_statistics[index].min = val;
    }

    if (get_timer(print_statistic_clock) > PRINTING_FREQUENCY_IN_SEC) {
        unsafe_print_statistics();
    }
    fractal_unlock_mutex(log_statistic_mutex);
}

void print_statistics() {
    fractal_lock_mutex(log_statistic_mutex);
    unsafe_print_statistics();
    fractal_unlock_mutex(log_statistic_mutex);
}
