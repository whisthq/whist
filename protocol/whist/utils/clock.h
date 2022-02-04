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
 * @brief                          Write a string representing the current time.
 *
 * The format is "yyyy-MM-dd'T'HH:mm:ss.uuuuuu", giving twenty-six bytes including the terminator.
 *
 * @param buffer                   Buffer to write to.
 * @param size                     Size of the buffer.
 * @returns                        Length of the time string, like snprintf().
 */
int current_time_str(char* buffer, size_t size);

/**
 * @brief                          Returns the number of microseconds elapsed since epoch
 *
 * @returns                        Number of microseconds elapsed since epoch
 */
timestamp_us current_time_us(void);

#endif
