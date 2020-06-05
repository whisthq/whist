#ifndef LOGGING_H
#define LOGGING_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file logging.h
 * @brief This fiel contains the logging macros and utils to send Winlogon
 *        status and to send the logs to the webserver.
============================
Usage
============================
We have several levels of logging.
- NO_LOG: self explanatory
- ERROR_LEVEL: only log errors. Errors are conditions that cause the program to
               terminate or lead to an irrecoverable state.
- WARNING_LEVEL: log warnings and above (warnings and errors). Warnings are when
                 things do not work as expected, but we can recover.
- INFO_LEVEL: log info and above. Info is just for logs that provide additional
              information on state. e.g decode time
- DEBUG_LEVEL: log debug and above. For use when actively debugging a problem,
               but for things that don't need to be logged regularly

The log level defaults to DEBUG_LEVEL, but it can also be passed as a compiler
flag, as it is in the root CMakesList.txt, which sets it to DEBUG_LEVEL for
Debug builds and WARNING_LEVEL for release builds.

Note that these macros do not need an additional \n character at the end of your
format strings.

We also have a LOG_IF(condition, format string) Macro which only logs if the
condition is true. This can be used for debugging or if we want to more
aggressively log something when a flag changes. For example in this file you
could #define LOG_AUDIO True and then use LOG_IF(LOG_AUDIO, "my audio logging").
*/

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

/**
 * Defines for conditional logging go here
 * e.g if you are debugging audio and only want to see your debug statements
 * then you would #define LOG_AUDIO True and use LOG_IF(LOG_AUDIO, "your debug
 * statement here")
 */

// Cut-off for which log level is required
#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG_LEVEL
#endif

#define PRINTFUNCTION(format, ...) mprintf(format, __VA_ARGS__)
#define LOG_FMT "%s | %-7s | %-15s | %30s:%-5d | "
#define LOG_ARGS(LOG_TAG) \
    CurrentTimeStr(), LOG_TAG, _FILE, __FUNCTION__, __LINE__

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

#if LOG_LEVEL >= NO_LOGS
#define LOG_IF(condition, message, ...) \
    if (condition)                      \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ##__VA_ARGS__)
#else
#define LOG_IF(condition, message, args...)
#endif

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the logger
 *
 * @param log_directory            The directory to store the log files in. Pass
 *                                 NULL to not store the logs in a log file
 */
void initLogger(char* log_directory);

/**
 * @brief                          Log the given format string
 *
 * @param fmtStr                   The directory to store the log files in
 */
void mprintf(const char* fmtStr, ...);

/**
 * @brief                          Destroy the logger object
 */
void destroyLogger();

/**
 * @brief                          Send the log history to the webserver
 */
bool sendLogHistory();

/**
 * @brief                          Tell the server the WinLogon and connection
 *                                 status
 *
 * @param is_connected             The connection status to send to the server.
 *                                 Pass true if connected to a client and false
 *                                 otherwise
 */
void updateStatus(bool is_connected);

/**
 * @brief                          Get the current server's version number
 *
 * @returns                        A 16-character hexadecimal version number
 */
char* get_version();

#endif  // LOGGING_H
