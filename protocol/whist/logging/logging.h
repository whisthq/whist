#ifndef LOGGING_H
#define LOGGING_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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
#include <whist/core/whist.h>
#include <whist/utils/clock.h>

/*
============================
Defines
============================
*/

#define LOGGER_QUEUE_SIZE 1000
#define LOGGER_BUF_SIZE 1000
#ifndef __ROOT_FILE__
#define __ROOT_FILE__ ""
#endif
#define LOG_FILE_NAME \
    ((sizeof(__ROOT_FILE__) < sizeof(__FILE__)) ? (&__FILE__[sizeof(__ROOT_FILE__)]) : "")
#define NO_LOG 0x00
#define ERROR_LEVEL 0x01
#define WARNING_LEVEL 0x02
#define METRIC_LEVEL 0x03
#define INFO_LEVEL 0x04
#define DEBUG_LEVEL 0x05

// Cut-off for which log level is required
#ifndef LOG_LEVEL
#define LOG_LEVEL DEBUG_LEVEL
#endif

//                     Timestamp    File name   Line number
//                         |   Tag      |  Function  |
//                         |    |       |       |    |
#define LOG_CONTEXT_FORMAT " | %-7s | %-35s | %-30s:%-5d | "

// a hacky way to disable logs after some limited are hit, so that we can look at audio logs more
// clearly
#define SUPRESS_LOG_AFTER 200

// We use do/while(0) to force the user to ";" the end of the LOG,
// While still keeping the statement inside a contiguous single block,
// so that `if(cond) LOG;` works as expected
#define LOG_MESSAGE(tag, message, ...)                                                \
    do {                                                                              \
        extern int g_log_cnt;                                                         \
        g_log_cnt++;                                                                  \
        if (g_log_cnt < SUPRESS_LOG_AFTER)                                            \
            internal_logging_printf(tag##_TAG, LOG_FILE_NAME, __FUNCTION__, __LINE__, \
                                    message "\n", ##__VA_ARGS__);                     \
        if (g_log_cnt >= SUPRESS_LOG_AFTER && g_log_cnt <= SUPRESS_LOG_AFTER + 10)    \
            fprintf(stderr, "non-audio logs are supressed from now on!!!\n");         \
    } while (0)

#define NEWLINE "\n"
// Cast to const chars so that comparison against XYZ_TAG is defined
extern const char *const debug_tag,
    *const info_tag, *const metric_tag, *const warning_tag, *const error_tag,
                                                                *const fatal_error_tag;
#define DEBUG_TAG debug_tag
#define INFO_TAG info_tag
#define METRIC_TAG metric_tag
#define WARNING_TAG warning_tag
#define ERROR_TAG error_tag
#define FATAL_ERROR_TAG fatal_error_tag

#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(message, ...) LOG_MESSAGE(DEBUG, message, ##__VA_ARGS__)
#else
#define LOG_DEBUG(message, ...)
#endif

// LOG_INFO refers to something that can happen, and does not imply that anything went wrong
#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(message, ...) LOG_MESSAGE(INFO, message, ##__VA_ARGS__)
#else
#define LOG_INFO(message, ...)
#endif

// LOG_METRIC refers to one or more key-value pairs in JSON format. Useful for monitoring
// performance metrics. Note that keys should be within double quotes, for JSON parsing to work.
// For example :
// LOG_METRIC("\"Latency\" : %d", latency);
#if LOG_LEVEL >= METRIC_LEVEL
#define LOG_METRIC(message, ...) LOG_MESSAGE(METRIC, message, ##__VA_ARGS__)
#else
#define LOG_METRIC(message, ...)
#endif

// LOG_WARNING refers to something going wrong,
// but it's unknown whether or not it's the code's fault or
// simply the host's situation (i.e. no audio device etc)
// TLDR ~ Something dubious happened, but not sure if it's an issue or not though.
#if LOG_LEVEL >= WARNING_LEVEL
#define LOG_WARNING(message, ...) LOG_MESSAGE(WARNING, message, ##__VA_ARGS__)
#else
#define LOG_WARNING(message, ...)
#endif

// LOG_ERROR *must* directly imply that something is fundamentally wrong with our code,
// but refers to something that is recoverable nonetheless.
// For OS errors, use LOG_WARNING when we want to try to recover, and LOG_FATAL when we do not.
// TLDR ~ Problems with *our code* that we can still and want to, recover from.
#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(message, ...) LOG_MESSAGE(ERROR, message, ##__VA_ARGS__)
#else
#define LOG_ERROR(message, ...)
#endif

// LOG_FATAL implies that the protocol cannot recover from this (usually OS-related) error,
// This is generally used for OS errors (the host being out of RAM, SDL failing to initialize),
// If an OS-related error code is recoverable, but exceedingly unlikely or unheard of thusfar
// (e.g. mutex creation failing), LOG_FATAL is highly preferred over attempting a recovery.
// TLDR ~ Problems with *the OS* that are unexpected and not easily recoverable from.
#define LOG_FATAL(message, ...)                           \
    do {                                                  \
        LOG_MESSAGE(FATAL_ERROR, message, ##__VA_ARGS__); \
        terminate_protocol(WHIST_EXIT_FAILURE);           \
    } while (0)

// FATAL_ASSERT will fatally exist when something absurd happens,
// such as an index being negative, or otherwise out-of-bounds.
// Even if we "could" recover, fatally asserting is preferred for absurdity checks.
// TLDR ~ Problems with *our code* that are absurd or unexpected.
#define FATAL_ASSERT(condition)                              \
    do {                                                     \
        if (!(condition)) {                                  \
            LOG_FATAL("Assertion [%s] Failed!", #condition); \
        }                                                    \
    } while (0)

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Print the stacktrace of the calling function to stdout
 *                                 This will be registered as if it was logged as well
 */
void print_stacktrace(void);

/**
 * @brief                          Initialize the logger.
 * @param catch_segfaults          Shall we install a segfault/backtrace handler
 *
 */
void whist_init_logger(bool catch_segfaults);

/**
 * @brief                          Log the given format string
 *                                 This is an internal function that shouldn't be used,
 *                                 please use the macros LOG_INFO, LOG_WARNING, etc
 *
 * @param tag                      The tag-level to log with
 * @param file_name                Name of the file this is being called from.
 * @param function                 Name of the function this is being called from.
 * @param line_number              Line number this is being called from.
 * @param fmt_str                  The format string to printf with
 */
#ifdef __GNUC__
__attribute__((format(printf, 5, 6)))
#endif
void internal_logging_printf(const char* tag, const char *file_name, const char *function,
                             int line_number, const char* fmt_str, ...);

/**
 * @brief                          This function will immediately flush all of the logs,
 *                                 rather than slowly pushing the logs through
 *                                 from another thread
 */
void flush_logs(void);

/**
 * @brief                          Destroy the logger object
 */
void destroy_logger(void);

/**
 * Set pause state on logging thread.
 *
 * While pause is set, the logging thread will not process any messages.
 * This should only be used for test purposes.
 *
 * @param pause  Pause state to set.
 */
void test_set_pause_state_on_logger_thread(bool pause);

#endif  // LOGGING_H
