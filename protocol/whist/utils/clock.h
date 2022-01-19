#ifndef CLOCK_H
#define CLOCK_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file clock.h
 * @brief This file contains the helper functions for timing code.
============================
Usage
============================

You can use start_timer and get_timer to time specific pieces of code, or to
relate different events across server and client.
*/

/*
============================
Includes
============================
*/

#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

/*
============================
Custom Types
============================
*/

/**
 * @brief Structure representing a timer.
 *
 * Start the timer with start_timer(), get the time elapsed since it was
 * started with get_timer().
 */
typedef struct WhistTimerOpaque {
    // This structure is opaque, but needs to be defined so that instances
    // of it can be made on the stack.
    uint64_t opaque[2];
} WhistTimer;

typedef uint64_t timestamp_us;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Start the given timer at the current time, as
 *                                 a stopwatch
 *
 * @param timer		           Pointer to the timer to start.
 */
void start_timer(WhistTimer* timer);

/**
 * @brief                          Get the amount of elapsed time in seconds since
 *                                 the last start_timer
 *
 * @param timer		           Pointer to the timer to query.
 */
double get_timer(const WhistTimer* timer);

/**
 * @brief                          Returns the current time as a string
 *
 * @returns						   The current time as a string
 */
char* current_time_str(void);

/**
 * @brief                          Returns the number of microseconds elapsed since epoch
 *
 * @returns                        Number of microseconds elapsed since epoch
 */
timestamp_us current_time_us(void);

#endif
