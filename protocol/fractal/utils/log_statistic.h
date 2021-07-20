#ifndef LOG_STATISTIC_H
#define LOG_STATISTIC_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file log_statistic.h
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
Public Functions
============================
*/

/**
 * @brief                          Initialize the statistic logger.
 */
void init_statistic_logger();

/**
 * @brief                          Print all currently held statistics, and flush the data store.
 */
void print_statistics();

/**
 * @brief                          Note a specific stat value associated with a key. If enough time
 *                                 has passed, also prints all statistics and flushes the data
 *                                 store.
 *
 * @param key                      The string to associate all values of the same statistic with
 *                                 each other. Will also be printed during print_statistics().
 *
 * @param val                      The double value to store.
 */
void log_double_statistic(const char* key, double val);

#endif  // LOG_STATISTIC_H
