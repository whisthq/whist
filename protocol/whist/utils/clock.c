/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file clock.c
 * @brief This file contains the helper functions for timing code.
============================
Usage
============================

You can use start_timer and get_timer to time specific pieces of code, or to
relate different events across server and client.
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <whist/core/whist.h>
#include "clock.h"

struct WhistTimerInternal {
#if defined(_WIN32)
    LARGE_INTEGER pc;
#else
    struct timespec ts;
#endif
};

void start_timer(WhistTimer* timer_opaque) {
    struct WhistTimerInternal* timer = (struct WhistTimerInternal*)timer_opaque;
#if defined(_WIN32)
    QueryPerformanceCounter(&timer->pc);
#else
    clock_gettime(CLOCK_MONOTONIC, &timer->ts);
#endif

    // Ensure that compilation fails if the size of the opaque structure
    // is not large enough to fit the real structure.  (This compiles to
    // no code, but needs to occur inside a function.)
    _Static_assert(sizeof(struct WhistTimerInternal) <= sizeof(struct WhistTimerOpaque),
                   "WhistTimerOpaque must be at least as large as WhistTimerInternal "
                   "on all platforms!");
}

double get_timer(const WhistTimer* timer_opaque) {
    const struct WhistTimerInternal* timer = (const struct WhistTimerInternal*)timer_opaque;
#if defined(_WIN32)
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&end);
    double ret = (double)(end.QuadPart - timer->pc.QuadPart) / frequency.QuadPart;
#else
    struct timespec t2;
    // use CLOCK_MONOTONIC for relative time
    clock_gettime(CLOCK_MONOTONIC, &t2);

    double elapsed_time = (t2.tv_sec - timer->ts.tv_sec) * MS_IN_SECOND;
    elapsed_time += (t2.tv_nsec - timer->ts.tv_nsec) / (double)NS_IN_MS;

    double ret = elapsed_time / MS_IN_SECOND;
#endif
    return ret;
}

void adjust_timer(WhistTimer* timer_opaque, int num_seconds) {
    struct WhistTimerInternal* timer = (struct WhistTimerInternal*)timer_opaque;
#if defined(_WIN32)
    timer->pc.QuadPart += (frequency.QuadPart * num_seconds);
#else
    timer->ts.tv_sec += num_seconds;
#endif
}

int current_time_str(char* buffer, size_t size) {
#if defined(_WIN32)
    SYSTEMTIME time_now;
    GetSystemTime(&time_now);
    return snprintf(buffer, size, "%04i-%02i-%02iT%02i:%02i:%02i.%06li", time_now.wYear,
                    time_now.wMonth, time_now.wDay, time_now.wHour, time_now.wMinute,
                    time_now.wSecond, (long)time_now.wMilliseconds * US_IN_MS);
#else
    struct tm* time_str_tm;
    struct timespec time_now;
    // CLOCK_REALTIME gives time since Unix epoch
    clock_gettime(CLOCK_REALTIME, &time_now);
    time_str_tm = gmtime(&time_now.tv_sec);
    return snprintf(buffer, size, "%04i-%02i-%02iT%02i:%02i:%02i.%06li",
                    time_str_tm->tm_year + 1900, time_str_tm->tm_mon + 1, time_str_tm->tm_mday,
                    time_str_tm->tm_hour, time_str_tm->tm_min, time_str_tm->tm_sec,
                    (long)time_now.tv_nsec / NS_IN_US);

#endif
}

timestamp_us current_time_us(void) {
    uint64_t output;
#if defined(_WIN32)
/* Windows epoch starts on 1601-01-01T00:00:00Z. But UNIX/Linux epoch starts on
   1970-01-01T00:00:00Z. To bring Windows time on par Unix epoch, the difference is above start
   times is subtracted */
#define EPOCH_DIFFERENCE 11644473600LL

    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    output = ((uint64_t)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
    output /= 10;  // To bring it to microseconds
    output -= (EPOCH_DIFFERENCE * (uint64_t)1000000);
#else
    struct timespec time_now;
    clock_gettime(CLOCK_REALTIME, &time_now);
    output = ((uint64_t)time_now.tv_sec * US_IN_SECOND) + time_now.tv_nsec / NS_IN_US;
#endif
    // Ensure that this cast is correct, uint64_t -> timestamp_us
    return (timestamp_us)output;
}
