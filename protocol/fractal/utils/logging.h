#ifndef LOGGING_H
#define LOGGING_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file logging.h
 * @brief This file contains the logging macros and utils to send Winlogon
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
#include "sentry.h"
#include "../core/fractal.h"
#include "clock.h"

/*
============================
Defines
============================
*/

#define SENTRY_DSN "https://8e1447e8ea2f4ac6ac64e0150fa3e5d0@o400459.ingest.sentry.io/5373388"

#define LOGGER_QUEUE_SIZE 1000
#define LOGGER_BUF_SIZE 1000
#define _FILE ((sizeof(__ROOT_FILE__) < sizeof(__FILE__)) ? (&__FILE__[sizeof(__ROOT_FILE__)]) : "")
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

#ifndef __ANDROID_API__
#define PRINTFUNCTION(format, ...) mprintf(format, __VA_ARGS__)
#else
#include <android/log.h>
#define PRINTFUNCTION(format, ...) \
    __android_log_print(ANDROID_LOG_DEBUG, "ANDROID_DEBUG", format, __VA_ARGS__);
#endif

#define SENTRYBREADCRUMB(tag, format, ...) sentry_send_bread_crumb(tag, format, ##__VA_ARGS__)
#define SENTRYEVENT(format, ...) sentry_send_event(format, ##__VA_ARGS__)
#define LOG_FMT "%s | %-7s | %-35s | %-30s:%-5d | "
#define LOG_ARGS(LOG_TAG) current_time_str(), LOG_TAG, _FILE, __FUNCTION__, __LINE__

#define NEWLINE "\n"
#define FATAL_ERROR_TAG "FATAL_ERROR"
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

// LOG_INFO refers to something that can happen, and does not imply that anything went wrong
#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(message, ...) \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ##__VA_ARGS__)
#else
#define LOG_INFO(message, ...)
#endif

// LOG_WARNING refers to something going wrong, but it's unknown whether or not it's the code or the
// host's configuration (i.e. no audio device etc)
#if LOG_LEVEL >= WARNING_LEVEL
#define LOG_WARNING(message, ...)                                                 \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(WARNING_TAG), ##__VA_ARGS__); \
    SENTRYBREADCRUMB(WARNING_TAG, message, ##__VA_ARGS__)
#else
#define LOG_WARNING(message, ...)
#endif

// LOG_ERROR must imply that something is fundamentally wrong with our code, but it is a recoverable
// error
#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(message, ...)                                                 \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ##__VA_ARGS__); \
    SENTRYEVENT(message, ##__VA_ARGS__)
#else
#define LOG_ERROR(message, ...)
#endif

// LOG_FATAL implies that the protocol cannot recover from this error, something might be
// wrong with our code (Or e.g. the host is out of RAM etc)
#define LOG_FATAL(message, ...)                                                       \
    PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(FATAL_ERROR_TAG), ##__VA_ARGS__); \
    SENTRYEVENT(message, ##__VA_ARGS__);                                              \
    terminate_protocol()

#if LOG_LEVEL >= NO_LOGS
#define LOG_IF(condition, message, ...) \
    if (condition) PRINTFUNCTION(LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ##__VA_ARGS__)
#else
#define LOG_IF(condition, message, args...)
#endif

/*
============================
Public Functions
============================
*/

void sentry_send_bread_crumb(char* tag, const char* fmt_str, ...);

void sentry_send_event(const char* fmt_str, ...);

/**
 * @brief                          Print the stacktrace of the calling function to stdout
 *                                 This will be registered as if it was logged as well
 */
void print_stacktrace();

/**
 * @brief                          Initialize the logger
 *
 * @param log_directory            The directory to store the log files in. Pass
 *                                 NULL to not store the logs in a log file
 *
 */
void init_logger(char* log_directory);

/**
 * @brief                          Log the given format string
 *
 * @param fmt_str                  The directory to store the log files in
 */
void mprintf(const char* fmt_str, ...);

/**
 * @brief                          Destroy the logger object
 */
void destroy_logger();

/**
 * @brief                          Set the logger to categorize all logs from now
 *                                  on as a new connection. Only these will be sent
 *                                  on a sendConnectionHistory call.
 */
void start_connection_log();

/**
 * @brief                          Save the current connection id into the log history
 *
 * @param connection_id            The connection id to use
 */
void save_connection_id(int connection_id);

/**
 * @brief                          Tell the server the WinLogon and connection
 *                                 status
 *
 * @param is_connected             The connection status to send to the server.
 *                                 Pass true if connected to a client and false
 *                                 otherwise.
 * @param host                     The webserver host to send the message to.
 * @param identifier               The string that uniquely identifies this
 *                                 instance of the protocol to the webserver.
 * @param hex_aes_private_key      The private key, as a hex string.
 */
void update_server_status(bool is_connected, char* host, char* identifier,
                          char* hex_aes_private_key);

/**
 * @brief                          Get the current server's version number
 *
 * @returns                        A 16-character hexadecimal version number
 */
char* get_version();

#endif  // LOGGING_H
