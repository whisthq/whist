#define _CRT_SECURE_NO_WARNINGS  // stupid Windows warnings

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/fractal.h"
#include "../utils/logging.h"
#include "clock.h"

#ifdef _WIN32
int GetUTCOffset();
#endif

#if defined(_WIN32)
LARGE_INTEGER frequency;
bool set_frequency = false;
#endif

void StartTimer(clock* timer) {
#if defined(_WIN32)
    if (!set_frequency) {
        QueryPerformanceFrequency(&frequency);
        set_frequency = true;
    }
    QueryPerformanceCounter(timer);
#else
    // start timer
    gettimeofday(timer, NULL);
#endif
}

double GetTimer(clock timer) {
#if defined(_WIN32)
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    double ret = (double)(end.QuadPart - timer.QuadPart) / frequency.QuadPart;
#else
    // stop timer
    struct timeval t2;
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    double elapsedTime = (t2.tv_sec - timer.tv_sec) * 1000.0;  // sec to ms
    elapsedTime += (t2.tv_usec - timer.tv_usec) / 1000.0;      // us to ms

    // printf("elapsed time in ms is: %f\n", elapsedTime);

    // standard var to return and convert to seconds since it gets converted to
    // ms in function call
    double ret = elapsedTime / 1000.0;
#endif
    return ret;
}

clock CreateClock(int timeout_ms) {
    clock out;
#if defined(_WIN32)
    out.QuadPart = timeout_ms;
#else
    out.tv_sec = timeout_ms / 1000;
    out.tv_usec = (timeout_ms % 1000) * 1000;
#endif
    return out;
}

char* CurrentTimeStr() {
    static char buffer[64];
    //    time_t rawtime;
    //
    //    time(&rawtime);
    //    timeinfo = localtime(&rawtime);
#if defined(_WIN32)
    SYSTEMTIME time_now;
    GetSystemTime(&time_now);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i:%03i", time_now.wHour,
             time_now.wMinute, time_now.wSecond, time_now.wMilliseconds);
#else
    struct tm* time_str_tm;
    struct timeval time_now;
    gettimeofday(&time_now, NULL);

    time_str_tm = gmtime(&time_now.tv_sec);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i:%06li",
             time_str_tm->tm_hour, time_str_tm->tm_min, time_str_tm->tm_sec,
             (long)time_now.tv_usec);
#endif

    //    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}

int GetUTCOffset() {
#if defined(_WIN32)
    return 0;
#else
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);
    printf("dst flag %d \n \n", lt.tm_isdst);

    return (int)lt.tm_gmtoff / (60 * 60);
#endif
}

int GetDST() {
#if defined(_WIN32)
    return 0;
#else
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);
    return lt.tm_isdst;
#endif
}

int GetTimeData(FractalTimeData* time_data) {
#ifdef _WIN32
    time_data->use_win_name = 1;
    time_data->use_linux_name = 0;

    char* win_tz_name = NULL;
    runcmd("powershell.exe \"$tz = Get-TimeZone; $tz.Id\" ", &win_tz_name);
    strncpy(time_data->win_tz_name, win_tz_name,
            sizeof(time_data->win_tz_name));
    time_data->win_tz_name[strlen(time_data->win_tz_name) - 1] = '\0';
    free(win_tz_name);

    LOG_INFO("Sending Windows TimeZone %s", time_data->win_tz_name);

    return 0;
#elif __APPLE__
    time_data->use_win_name = 0;
    time_data->use_linux_name = 1;

    time_data->UTC_Offset = GetUTCOffset();
    LOG_INFO("Sending UTC offset %d", time_data->UTC_Offset);
    time_data->DST_flag = GetDST();

    char* response = NULL;
    runcmd(
        "path=$(readlink /etc/localtime); echo "
        "${path#\"/var/db/timezone/zoneinfo\"}",
        &response);
    strncpy(time_data->linux_tz_name, response,
            sizeof(time_data->linux_tz_name));
    free(response);

    return 0;
#else
    time_data->use_win_name = 0;
    time_data->use_linux_name = 1;

    time_data->UTC_Offset = GetUTCOffset();
    LOG_INFO("Sending UTC offset %d", time_data->UTC_Offset);
    time_data->DST_flag = GetDST();

    char* response = NULL;
    runcmd("cat /etc/timezone", &response);
    strncpy(time_data->linux_tz_name, response,
            sizeof(time_data->linux_tz_name));
    free(response);

    return 0;
#endif
}
