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
- NO_LOG: only log fatal errors. Fatal errors are irrecoverable and cause the protocol to
immediately exit.
- ERROR_LEVEL: only log errors. Errors are conditions that necessarily
               imply that the protocol has done something wrong and must be fixed.
               However, Errors remain recoverable can the protocol can ignore them.
- WARNING_LEVEL: log warnings and above (warnings and errors). Warnings might indicate
                 that something is wrong, but it not necessarily the fault of the protocol,
                 it may be the fault of the environment (e.g. No audio device detected,
                 packet loss during handshake, etc).
- INFO_LEVEL: log info and above. Info is for logs that provide additional
              information on the state of the protocol. e.g. decode time
- DEBUG_LEVEL: log debug and above. For use when actively debugging a problem,
               but for things that don't need to be logged regularly

The log level defaults to DEBUG_LEVEL, but it can also be passed as a compiler
flag, as it is in the root CMakesList.txt, which sets it to DEBUG_LEVEL for
Debug builds and WARNING_LEVEL for release builds.

Note that these macros do not need an additional \n character at the end of your
format strings.
*/

/*
============================
Includes
============================
*/

#include <string.h>
#include <fractal/core/fractal.h>
#include <fractal/utils/clock.h>

/*
============================
Defines
============================
*/

#define LOGGER_QUEUE_SIZE 1000
#define LOGGER_BUF_SIZE 1000
#define _FILE ((sizeof(__ROOT_FILE__) < sizeof(__FILE__)) ? (&__FILE__[sizeof(__ROOT_FILE__)]) : "")
#define NO_LOG 0x00
#define ERROR_LEVEL 0x01
#define WARNING_LEVEL 0x02
#define INFO_LEVEL 0x04
#define DEBUG_LEVEL 0x05

// Cut-off for which log level is required
#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG_LEVEL
#endif

#define LOG_FMT "%s | %-7s | %-35s | %-30s:%-5d | "
#define LOG_ARGS(LOG_TAG) current_time_str(), LOG_TAG, _FILE, __FUNCTION__, __LINE__

#define NEWLINE "\n"
// Cast to const chars so that comparison against XYZ_TAG is defined
extern const char *debug_tag, *info_tag, *warning_tag, *error_tag, *fatal_error_tag;
#define DEBUG_TAG debug_tag
#define INFO_TAG info_tag
#define WARNING_TAG warning_tag
#define ERROR_TAG error_tag
#define FATAL_ERROR_TAG fatal_error_tag

#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(message, ...) \
    internal_logging_printf(DEBUG_TAG, LOG_FMT message NEWLINE, LOG_ARGS(DEBUG_TAG), ##__VA_ARGS__)
#else
#define LOG_DEBUG(message, ...)
#endif

// LOG_INFO refers to something that can happen, and does not imply that anything went wrong
#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(message, ...) \
    internal_logging_printf(INFO_TAG, LOG_FMT message NEWLINE, LOG_ARGS(INFO_TAG), ##__VA_ARGS__)
#else
#define LOG_INFO(message, ...)
#endif

// LOG_WARNING refers to something going wrong, but it's unknown whether or not it's the code or the
// host's configuration (i.e. no audio device etc)
#if LOG_LEVEL >= WARNING_LEVEL
#define LOG_WARNING(message, ...)                                                        \
    internal_logging_printf(WARNING_TAG, LOG_FMT message NEWLINE, LOG_ARGS(WARNING_TAG), \
                            ##__VA_ARGS__);
#else
#define LOG_WARNING(message, ...)
#endif

// LOG_ERROR must imply that something is fundamentally wrong with our code, but it is a recoverable
// error
#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(message, ...) \
    internal_logging_printf(ERROR_TAG, LOG_FMT message NEWLINE, LOG_ARGS(ERROR_TAG), ##__VA_ARGS__);
#else
#define LOG_ERROR(message, ...)
#endif

// LOG_FATAL implies that the protocol cannot recover from this error, something might be
// wrong with our code (Or e.g. the host is out of RAM etc)
#define LOG_FATAL(message, ...)                                                                  \
    internal_logging_printf(FATAL_ERROR_TAG, LOG_FMT message NEWLINE, LOG_ARGS(FATAL_ERROR_TAG), \
                            ##__VA_ARGS__);                                                      \
    terminate_protocol(FRACTAL_EXIT_FAILURE)

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Print the stacktrace of the calling function to stdout
 *                                 This will be registered as if it was logged as well
 */
void print_stacktrace();

/**
 * @brief                          Initialize the logger.
 *
 */
void init_logger();

/**
 * @brief                          Log the given format string
 *                                 This is an internal function that shouldn't be used,
 *                                 please use the macros LOG_INFO, LOG_WARNING, etc
 *
 * @param tag                      The tag-level to log with
 *
 * @param fmt_str                  The format string to printf with
 */
void internal_logging_printf(const char* tag, const char* fmt_str, ...);

/**
 * @brief                          This function will immediately flush all of the logs,
 *                                 rather than slowly pushing the logs through
 *                                 from another thread
 */
void flush_logs();

/**
 * @brief                          Destroy the logger object
 */
void destroy_logger();

#endif  // LOGGING_H
