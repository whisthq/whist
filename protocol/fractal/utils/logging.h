#ifndef LOGGING_H
#define LOGGING_H

// *** BEGIN INCLUDES ***

#include <stdio.h>

#ifndef _WIN32
#include <execinfo.h>
#include <signal.h>
#endif

#include "../core/fractal.h"
#include "../network/network.h"
#include <time.h>
#include <string.h>

// *** END INCLUDES ***

// *** BEGIN DEFINES ***
#define LOGGER_QUEUE_SIZE 1000
#define LOGGER_BUF_SIZE 1000
#define _FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#define NO_LOG          0x00
#define ERROR_LEVEL     0x01
#define WARNING_LEVEL   0x02
#define INFO_LEVEL      0x04
#define DEBUG_LEVEL     0x05

// Set logging level here
#ifndef LOG_LEVEL
#define LOG_LEVEL   DEBUG_LEVEL
#endif

#define PRINTFUNCTION(format, ...)      mprintf(format, __VA_ARGS__)
#define LOG_FMT             "%s | %-7s | %-15s | %s:%d | "
#define LOG_ARGS(LOG_TAG)   timenow(), LOG_TAG, _FILE, __FUNCTION__, __LINE__

#define NEWLINE     "\n"
#define ERROR_TAG   "ERROR"
#define WARNING_TAG "WARNING"
#define INFO_TAG    "INFO"
#define DEBUG_TAG   "DEBUG"

#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(message, args...)     PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ## args)
#else
#define LOG_DEBUG(message, args...)
#endif

#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(message, args...)      PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ## args)
#else
#define LOG_INFO(message, args...)
#endif

#if LOG_LEVEL >= WARNING_LEVEL
#define LOG_WARNING(message, args...)     PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(WARNING_TAG), ## args)
#else
#define LOG_WARNING(message, args...)
#endif

#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(message, args...)     PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ## args)
#else
#define LOG_ERROR(message, args...)
#endif

#if defined(_WIN32)
#define clock LARGE_INTEGER
#else
#define clock struct timeval
#endif

// *** END DEFINES ***

// *** BEGIN FUNCTIONS ***

void initLogger(char* log_directory);
void destroyLogger();
void mprintf(const char* fmtStr, ...);
void lprintf(const char* fmtStr, ...);

void StartTimer(clock* timer);
double GetTimer(clock timer);

static inline char* timenow();

void initBacktraceHandler();

char* get_logger_history();
int get_logger_history_len();

bool sendLog();

// Get current time
static inline char* timenow() {
    static char buffer[64];
//    time_t rawtime;
    struct tm *time_str_tm;
//
//    time(&rawtime);
//    timeinfo = localtime(&rawtime);
    struct timeval time_now;
    gettimeofday(&time_now, NULL);
    time_str_tm = gmtime(&time_now.tv_sec);
    sprintf(buffer, "%02i:%02i:%02i:%06i"
            , time_str_tm->tm_hour
            , time_str_tm->tm_min
            , time_str_tm->tm_sec
            , time_now.tv_usec);

//    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}
// *** END FUNCTIONS ***

#endif  // LOGGING_H