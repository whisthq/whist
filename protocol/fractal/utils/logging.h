#ifndef LOGGING_H
#define LOGGING_H

/*
============================
Includes
============================
*/

#include <string.h>

#include "../core/fractal.h"
#include "clock.h"

/*
============================
Defines
============================
*/

#define LOGGER_QUEUE_SIZE 1000
#define LOGGER_BUF_SIZE 1000
#define _FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#define NO_LOG 0x00
#define ERROR_LEVEL 0x01
#define WARNING_LEVEL 0x02
#define INFO_LEVEL 0x04
#define DEBUG_LEVEL 0x05

// Cut-off for which log level is required
#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG_LEVEL
#endif

#define PRINTFUNCTION(format, ...) mprintf(format, __VA_ARGS__)
#define LOG_FMT "%s | %-7s | %-15s | %s:%d | "
#define LOG_ARGS(LOG_TAG) CurrentTimeStr(), LOG_TAG, _FILE, __FUNCTION__, __LINE__

#define NEWLINE "\n"
#define ERROR_TAG "ERROR"
#define WARNING_TAG "WARNING"
#define INFO_TAG "INFO"
#define DEBUG_TAG "DEBUG"

#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(message, ...) \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ##__VA_ARGS__)
#else
#define LOG_DEBUG(message, ...)
#endif

#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(message, ...) \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ##__VA_ARGS__)
#else
#define LOG_INFO(message, ...)
#endif

#if LOG_LEVEL >= WARNING_LEVEL
#define LOG_WARNING(message, ...) \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(WARNING_TAG), ##__VA_ARGS__)
#else
#define LOG_WARNING(message, ...)
#endif

#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(message, ...) \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ##__VA_ARGS__)
#else
#define LOG_ERROR(message, ...)
#endif

/*
============================
Public Functions
============================
*/

/*
@brief                          Initialize the logger

@param log_directory            The directory to store the log files in. Pass NULL to not store the logs in a log file.
*/
void initLogger(char* log_directory);

/*
@brief                          Log the given format string.

@param fmtStr            The directory to store the log files in
*/
void mprintf(const char* fmtStr, ...);

/*
@brief                          Destroy the logger
*/
void destroyLogger();

/*
@brief                          Send the log history to the webserver
*/
bool sendLogHistory();

#endif  // LOGGING_H
