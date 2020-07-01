#ifndef CLOCK_H
#define CLOCK_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clock.h
 * @brief Helper functions for timing
============================
Usage
============================
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

typedef struct FractalTimeData {
    // UTC offset for setting time
    int use_win_name;   /**< Flag if win_tz_name is to be used */
    int use_linux_name; /**< FLag if linux_tz_name is to be used */
    int UTC_Offset;     /**< UTC offset for osx/linux -> windows */
    int DST_flag; /**< DST flag, 1 DST, 0 no DST used in conjunction with UTC
                     offset */
    char win_tz_name[200];   /**< A windows timezone name: e.g Eastern Standard
                                Time */
    char linux_tz_name[200]; /**< A linux/IANA timezone name: e.g
                                America/New_York  */
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
void StartTimer(clock* timer);

/**
 * @brief                          Get the amount of elapsed time since the last
 *                                 StartTimer
 *
 * @param timer		               The timer in question
 */
double GetTimer(clock timer);

/**
 * @brief                          Create a clock that represents the given
 *                                 timeout in milliseconds
 *
 * @param timeout_ms	           The number of milliseconds for the clock
 *
 * @returns						   The desired clock
 */
clock CreateClock(int timeout_ms);

/**
 * @brief                          Returns the current time as a string
 *
 * @returns						   The current time as a string
 */
char* CurrentTimeStr();

/**
 * @brief                          Returns the current UTC offset as a string
 * @return                         int of current utc offset
 */
int GetUTCOffset();

/**
 * @brief                          Returns a flag for whether DST is on or off
 * @return                         Positive int for on 0 for off.
 */
int GetDST();

int GetTimeData(FractalTimeData* time_data);

#endif
