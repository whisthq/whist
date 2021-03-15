#ifndef CLOCK_H
#define CLOCK_H
/**
 * Copyright Fractal Computers, Inc. 2020
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

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

/*
============================
Defines
============================
*/

#if defined(_WIN32)
#define clock LARGE_INTEGER
#else
#define clock struct timeval
#endif

/*
============================
Custom Types
============================
*/

#define TZ_NAME_MAXLEN 200

typedef struct FractalTimeData {
    // UTC offset for setting time
    int use_win_name;   /**< Flag if win_tz_name is to be used */
    int use_linux_name; /**< FLag if linux_tz_name is to be used */
    int utc_offset;     /**< UTC offset for osx/linux -> windows */
    int dst_flag;       /**< DST flag, 1 DST, 0 no DST used in conjunction with UTC offset */
    char win_tz_name[TZ_NAME_MAXLEN];   /**< A windows timezone name: e.g Eastern Standard Time */
    char linux_tz_name[TZ_NAME_MAXLEN]; /**< A linux/IANA timezone name: e.g America/New_York  */
} FractalTimeData;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Start the given timer at the current time, as
 *                                 a stopwatch
 *
 * @param timer		               Pointer to the the timer in question
 */
void start_timer(clock* timer);

/**
 * @brief                          Get the amount of elapsed time since the last
 *                                 start_timer
 *
 * @param timer		               The timer in question
 */
double get_timer(clock timer);

/**
 * @brief                          Create a clock that represents the given
 *                                 timeout in milliseconds
 *
 * @param timeout_ms	           The number of milliseconds for the clock
 *
 * @returns						   The desired clock
 */
clock create_clock(int timeout_ms);

/**
 * @brief                          Returns the current time as a string
 *
 * @returns						   The current time as a string
 */
char* current_time_str();

/**
 * @brief                          Gets a FractalTimeData object representing the current timezone
 */
int get_time_data(FractalTimeData* time_data);

/**
 * @brief                          Sets the current timezone based on a FractalTimeData object
 */
void set_time_data(FractalTimeData* time_data);

#endif
