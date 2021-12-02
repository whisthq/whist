/**
 * Copyright 2021 Whist Technologies, Inc.
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

#include <fractal/core/whist.h>
#include "clock.h"

#ifdef _WIN32
int get_utc_offset();
#endif

#define US_IN_MS 1000.0

void start_timer(clock* timer) {
#if defined(_WIN32)
    QueryPerformanceCounter(timer);
#else
    // start timer
    gettimeofday(timer, NULL);
#endif
}

double get_timer(clock timer) {
#if defined(_WIN32)
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&end);
    double ret = (double)(end.QuadPart - timer.QuadPart) / frequency.QuadPart;
#else
    // stop timer
    struct timeval t2;
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    double elapsed_time = (t2.tv_sec - timer.tv_sec) * MS_IN_SECOND;  // sec to ms
    elapsed_time += (t2.tv_usec - timer.tv_usec) / US_IN_MS;          // us to ms

    // LOG_INFO("elapsed time in ms is: %f\n", elapsedTime);

    // standard var to return and convert to seconds since it gets converted to
    // ms in function call
    double ret = elapsed_time / MS_IN_SECOND;
#endif
    return ret;
}

clock create_clock(int timeout_ms) {
    clock out;
#if defined(_WIN32)
    out.QuadPart = timeout_ms;
#else
    out.tv_sec = timeout_ms / (double)MS_IN_SECOND;
    out.tv_usec = (timeout_ms % 1000) * 1000;
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
#else
    struct tm* time_str_tm;
    struct timeval time_now;
    gettimeofday(&time_now, NULL);

    time_str_tm = gmtime(&time_now.tv_sec);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i.%06li", time_str_tm->tm_hour,
             time_str_tm->tm_min, time_str_tm->tm_sec, (long)time_now.tv_usec);
#endif

    //    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}
