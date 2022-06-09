/**
 * @copyright Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file clock.h
 * @brief This file contains the helper functions for timing code.
 */
#ifndef WHIST_UTILS_CLOCK_H
#define WHIST_UTILS_CLOCK_H
/*
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
#if OS_IS(OS_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

/**
 * @defgroup timers Timers
 *
 * Whist API for timers.
 *
 * This provides a way to measure how long a section of code takes or
 * how much real time has elapsed since some event.
 *
 * Example:
 * @code{.c}
 * WhistTimer timer;
 * start_timer(&timer);
 * foo();
 * printf("foo() took %f seconds to run.", get_timer(&timer));
 * @endcode
 *
 * @{
 */

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

/**
 * Timestamp type.
 *
 * This represents a number of microseconds since the epoch.  Returned
 * by current_time_us().
 */
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
 *
 * @return                         Seconds since the timer was started.
 */
double get_timer(const WhistTimer* timer);

/**
 * @brief                          Get the difference in seconds between two timer values
 *
 * @param start_timer              Pointer to the start timer.
 *
 * @param end_timer                Pointer to the end timer.
 *
 * @return                         Value of (end_timer - start_timer) in seconds
 */
double diff_timer(const WhistTimer* start_timer, const WhistTimer* end_timer);

/**
 * @brief                          Add the value of the timer by a configurable number of seconds
 *
 * @param timer                    Pointer to the timer to query.
 *
 * @param num_seconds              Number of seconds to be added to the timer value. This parameter
 *                                 can be negative as well.
 */
void adjust_timer(WhistTimer* timer, int num_seconds);

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
 * @note                           This value returned by this function is based on wallclock time,
 *                                 it's not for comparing how much time has past
 */
timestamp_us current_time_us(void);

/**
 * @brief                          Get a comparable timestamp in sec.
 *
 * @returns                        Timestamp in sec.
 * @note                           The point of this function is to get a comparable timestamp, the
 *                                 absolute value is not important. But for a well-defined behavior:
 *                                 The first call of this function returns zero, following calls run
 *                                 the time past in sec since first call.
 *                                 This function is thread-safe after the first call finishes. The
 *                                 first call is expected to be done inside whist_init_subsystems().
 */
double get_timestamp_sec(void);

/** @} */

#endif /* WHIST_UTILS_CLOCK_H */
