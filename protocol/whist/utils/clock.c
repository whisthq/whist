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

#define _CRT_SECURE_NO_WARNINGS  // stupid Windows warnings

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <whist/core/whist.h>
#include "clock.h"

#ifdef _WIN32
int get_utc_offset();
#endif

void start_timer(clock* timer) {
#if defined(_WIN32)
    QueryPerformanceCounter(timer);
#elif defined(__APPLE__)
    // start timer
    gettimeofday(timer, NULL);
#else
    clock_gettime(CLOCK_MONOTONIC, timer);
#endif
}

double get_timer(clock timer) {
#if defined(_WIN32)
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&end);
    double ret = (double)(end.QuadPart - timer.QuadPart) / frequency.QuadPart;
#elif defined(__APPLE__)
    // Apple doesn't have clock_gettime, so we use gettimeofday
    // stop timer
    struct timeval t2;
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    double elapsed_time = (t2.tv_sec - timer.tv_sec) * MS_IN_SECOND;  // sec to ms
    elapsed_time += (t2.tv_usec - timer.tv_usec) / (double)US_IN_MS;  // us to ms

    // LOG_INFO("elapsed time in ms is: %f\n", elapsedTime);

    // standard var to return and convert to seconds since it gets converted to
    // ms in function call
    double ret = elapsed_time / MS_IN_SECOND;
#else
    struct timespec t2;
    // use CLOCK_MONOTONIC for relative time
    clock_gettime(CLOCK_MONOTONIC, &t2);

    double elapsed_time = (t2.tv_sec - timer.tv_sec) * MS_IN_SECOND;
    elapsed_time += (t2.tv_nsec - timer.tv_nsec) / (double)NS_IN_MS;

    double ret = elapsed_time / MS_IN_SECOND;
#endif
    return ret;
}

timeout_clock create_timeout_clock(int timeout_ms) {
    timeout_clock out;
#if defined(_WIN32)
    out.QuadPart = timeout_ms;
#else
    out.tv_sec = timeout_ms / MS_IN_SECOND;
    out.tv_usec = (timeout_ms % MS_IN_SECOND) * US_IN_MS;
#endif
    return out;
}

char* current_time_str() {
    static char buffer[64];
    //    time_t rawtime;
    //
    //    time(&rawtime);
    //    timeinfo = localtime(&rawtime);
#if defined(_WIN32)
    SYSTEMTIME time_now;
    GetSystemTime(&time_now);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i.%06li", time_now.wHour, time_now.wMinute,
             time_now.wSecond, (long)time_now.wMilliseconds);
#elif defined(__APPLE__)
    struct tm* time_str_tm;
    struct timeval time_now;
    gettimeofday(&time_now, NULL);

    time_str_tm = gmtime(&time_now.tv_sec);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i.%06li", time_str_tm->tm_hour,
             time_str_tm->tm_min, time_str_tm->tm_sec, (long)time_now.tv_usec);
#else
    struct tm* time_str_tm;
    struct timespec time_now;
    // CLOCK_REALTIME gives time since Unix epoch
    clock_gettime(CLOCK_REALTIME, &time_now);
    time_str_tm = gmtime(&time_now.tv_sec);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i.%06li", time_str_tm->tm_hour,
             time_str_tm->tm_min, time_str_tm->tm_sec, (long)time_now.tv_nsec / NS_IN_US);
#endif

    //    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}

timestamp_us current_time_us() {
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
#elif defined(__APPLE__)
    struct timeval time_now;
    gettimeofday(&time_now, NULL);
    // TODO: change to US_IN_SEC and NS_IN_US
    output = ((uint64_t)time_now.tv_sec * US_IN_SECOND) + time_now.tv_usec;
#else
    struct timespec time_now;
    clock_gettime(CLOCK_REALTIME, &time_now);
    output = ((uint64_t)time_now.tv_sec * US_IN_SECOND) + time_now.tv_nsec / NS_IN_US;
#endif
    // Ensure that this cast is correct, uint64_t -> timestamp_us
    return (timestamp_us)output;
}
