/**
 * Copyright Fractal Computers, Inc. 2020
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

#include <fractal/core/fractal.h>
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

    strftime(buffer, sizeof buffer, "%FT%T", gmtime(&time_now.tv_sec));
    sprintf(buffer, "%s.%dZ", buffer, time_now.tv_usec);
#endif

    //    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}

int get_utc_offset() {
#if defined(_WIN32)
    char* utc_offset_str;
    runcmd("powershell.exe \"Get-Date -UFormat \\\"%Z\\\"\"", &utc_offset_str);
    if (strlen(utc_offset_str) >= 3) {
        utc_offset_str[3] = '\0';
    }

    int utc_offset = atoi(utc_offset_str);

    free(utc_offset_str);

    return utc_offset;
#else
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);

    return (int)lt.tm_gmtoff / (60 * 60);
#endif
}

int get_dst() {
#if defined(_WIN32)
    // TODO: Implement DST on Windows if it makes sense
    return 0;
#else
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);
    return lt.tm_isdst;
#endif
}

int get_time_data(FractalTimeData* time_data) {
    time_data->use_utc_offset = 1;

    time_data->utc_offset = get_utc_offset();
    time_data->dst_flag = get_dst();
    LOG_INFO("Getting UTC offset %d (DST: %d)", time_data->utc_offset, time_data->dst_flag);

#ifdef _WIN32
    time_data->use_win_name = 1;
    time_data->use_linux_name = 0;

    char* win_tz_name = NULL;
    runcmd("powershell.exe \"$tz = Get-TimeZone; $tz.Id\" ", &win_tz_name);
    LOG_DEBUG("Getting Windows TimeZone %s", win_tz_name);
    if (strlen(win_tz_name) >= 1) {
        win_tz_name[strlen(win_tz_name) - 1] = '\0';
    }
    safe_strncpy(time_data->win_tz_name, win_tz_name,
                 min(sizeof(time_data->win_tz_name), strlen(win_tz_name) + 1));
    free(win_tz_name);

    return 0;
#elif __APPLE__
    time_data->use_win_name = 0;
    time_data->use_linux_name = 1;

    char* linux_tz_name = NULL;
    runcmd(
        "path=$(readlink /etc/localtime); echo "
        "${path#\"/var/db/timezone/zoneinfo/\"}",
        &linux_tz_name);
    safe_strncpy(time_data->linux_tz_name, linux_tz_name,
                 min(sizeof(time_data->linux_tz_name), strlen(linux_tz_name) + 1));
    free(linux_tz_name);

    return 0;
#else
    time_data->use_win_name = 0;
    time_data->use_linux_name = 1;
    time_data->use_utc_offset = 1;

    time_data->utc_offset = get_utc_offset();
    LOG_INFO("Sending UTC offset %d", time_data->utc_offset);
    time_data->dst_flag = get_dst();

    char* linux_tz_name = NULL;
    runcmd("cat /etc/timezone", &linux_tz_name);
    safe_strncpy(time_data->linux_tz_name, linux_tz_name,
                 min(sizeof(time_data->linux_tz_name), strlen(linux_tz_name) + 1));
    free(linux_tz_name);

    return 0;
#endif
}

void set_timezone_from_iana_name(char* linux_tz_name) {
#ifdef _WIN32
    LOG_ERROR("Unimplemented on Windows!");
#else
    char cmd[512];
    int length_used = snprintf(cmd, sizeof(cmd), "timedatectl set-timezone %s", linux_tz_name);

    if (length_used >= (int)sizeof(cmd)) {
        LOG_ERROR("Error: Timezone given is too long! %s", linux_tz_name);
        return;
    }

    runcmd(cmd, NULL);
#endif
}

void set_timezone_from_windows_name(char* win_tz_name) {
#ifdef _WIN32
    char cmd[256];
    //    Timezone name must end with no white space
    for (size_t i = 0; win_tz_name[i] != '\0'; i++) {
        if (win_tz_name[i] == '\n') {
            win_tz_name[i] = '\0';
        }
        if (win_tz_name[i] == '\r') {
            win_tz_name[i] = '\0';
        }
    }
    int length_used =
        snprintf(cmd, sizeof(cmd), "powershell -command \"Set-TimeZone -Id '%s'\"", win_tz_name);

    if (length_used >= (int)sizeof(cmd)) {
        LOG_ERROR("Error: Timezone given is too long! %s", win_tz_name);
        return;
    }

    char* response = NULL;
    runcmd(cmd, &response);
    // LOG_DEBUG("Timezone powershell command: %s -> %s", cmd, response);
    free(response);
#else
    LOG_ERROR("Unimplemented on Linux!");
#endif
}

void set_timezone_from_utc(int utc, int dst_flag) {
    if (dst_flag > 0) {
        LOG_INFO("DST active");
        utc = utc - 1;
    }
#ifdef _WIN32
    char* timezone = "";
    switch (utc) {
        case -12:
            timezone = "Dateline Standard Time";
            break;
        case -11:
            timezone = "UTC-11";
            break;
        case -10:
            timezone = "Hawaiian Standard Time";
            break;
        case -9:
            timezone = "Alaskan Standard Time";
            break;
        case -8:
            timezone = "Pacific Standard Time";
            break;
        case -7:
            timezone = "Mountain Standard Time";
            break;
        case -6:
            timezone = "Central Standard Time";
            break;
        case -5:
            timezone = "US Eastern Standard Time";
            break;
        case -4:
            timezone = "Atlantic Standard Time";
            break;
        case -3:
            timezone = " E. South America Standard Time";
            break;
        case -2:
            timezone = "Mid-Atlantic Standard Time";
            break;
        case -1:
            timezone = "Cape Verde Standard Time";
            break;
        case 0:
            timezone = "GMT Standard Time";
            break;
        case 1:
            timezone = "W. Europe Standard Time";
            break;
        case 2:
            timezone = "E. Europe Standard Time";
            break;
        case 3:
            timezone = "Turkey Standard Time";
            break;
        case 4:
            timezone = "Arabian Standard Time";
            break;
        default:
            LOG_WARNING("Note a valid UTC offset: %d", utc);
            return;
    }
    set_timezone_from_windows_name(timezone);
#else
    char cmd[512];
    if (utc == 0) {
        snprintf(cmd, sizeof(cmd), "timedatectl set-timezone Etc/GMT");
    } else {
        // -utc because of
        // https://www.reddit.com/r/java/comments/5i3zd1/timezoneid_etcgmt2_is_actually_gmt2/
        snprintf(cmd, sizeof(cmd), "timedatectl set-timezone Etc/GMT%s%d", -utc > 0 ? "+" : "",
                 -utc);
    }
    runcmd(cmd, NULL);
#endif
}

void set_time_data(FractalTimeData* time_data) {
#ifdef _WIN32
    if (time_data->use_win_name) {
        LOG_INFO("Setting time from windows time zone %s", time_data->win_tz_name);
        set_timezone_from_windows_name(time_data->win_tz_name);
    } else if (time_data->use_utc_offset) {
        LOG_INFO("Setting time from UTC offset %d", time_data->utc_offset);
        set_timezone_from_utc(time_data->utc_offset, time_data->dst_flag);
    } else {
        LOG_ERROR("Could not set time (%d, %d, %d)", time_data->use_win_name,
                  time_data->use_linux_name, time_data->utc_offset);
    }
#else
    if (time_data->use_linux_name) {
        LOG_INFO("Setting time from IANA time zone %s", time_data->linux_tz_name);
        set_timezone_from_iana_name(time_data->linux_tz_name);
    } else if (time_data->use_utc_offset) {
        LOG_INFO("Setting time from UTC offset %d", time_data->utc_offset);
        set_timezone_from_utc(time_data->utc_offset, time_data->dst_flag);
    } else {
        LOG_ERROR("Could not set time (%d, %d, %d)", time_data->use_win_name,
                  time_data->use_linux_name, time_data->utc_offset);
    }
#endif
}
