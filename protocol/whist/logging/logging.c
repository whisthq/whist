/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file logging.c
 * @brief This file contains the logging macros and utils for Whist.
============================
Usage
============================
We have several levels of logging.
- NO_LOG: self explanatory
- ERROR_LEVEL: only log errors. Errors are problems that must be addressed,
               as they indicate a fundamental probem with the protocol,
               but they represent a status that can be recovered from.
- WARNING_LEVEL: log warnings and above (warnings and errors). Warnings are when
                 something did not work as expected, but do not necessarily imply
                 that the protocol is at fault, as it may be an issue with
                 their environment (No audio device found, packet loss during handshake, etc).
- INFO_LEVEL: log info and above. Info is just for logs that provide additional
              information on the state of the protocol. e.g. decode time
- DEBUG_LEVEL: log debug and above. For use when actively debugging a problem,
               but for things that don't need to be logged regularly
The log level defaults to DEBUG_LEVEL, but it can also be passed as a compiler
flag, as it is in the root CMakesList.txt, which sets it to DEBUG_LEVEL for
Debug builds and WARNING_LEVEL for release builds.
Note that these macros do not need an additional \n character at the end of your
format strings.
*/

#ifdef _WIN32
#define _NO_CVCONST_H
#include <Windows.h>
#include <DbgHelp.h>
#include <process.h>
#define strtok_r strtok_s
#else
// Backtrace handling
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
// Signal handling
#include <signal.h>
#endif

#include <stdio.h>
#include <stdbool.h>

#include <whist/core/whist.h>
#include "whist/utils/atomic.h"
#include "whist/utils/command_line.h"
#include "whist/utils/linked_list.h"
#include "logging.h"
#include "error_monitor.h"

#ifndef NDEBUG
// disable logs after some limits are hit, helpful for debug
// if SUPRESS_LOG_AFTER <=0, this feature is disabled
// if this code cause real trouble in future, feel free to remove.
#define SUPRESS_LOG_AFTER 300
#else
#define SUPRESS_LOG_AFTER 0
#endif
// num of logs printed so far, should be okay without mutex only for debug
static int g_log_cnt = 0;

static void init_backtrace_handler(void);

// TAG Strings
static const char* const tag_strings[] = {
    [FATAL_ERROR_LEVEL] = "FATAL_ERROR", [ERROR_LEVEL] = "ERROR", [WARNING_LEVEL] = "WARNING",
    [METRIC_LEVEL] = "METRIC",           [INFO_LEVEL] = "INFO",   [DEBUG_LEVEL] = "DEBUG",
};

/**
 * Type for a single log item in the logger queue.
 */
typedef struct {
    LINKED_LIST_HEADER;
    unsigned int level;
    char buf[LOGGER_BUF_SIZE];
} LoggerQueueItem;

/**
 * List of free LoggerQueueItems to be used for carrying logs.
 *
 * Items are both inserted and removed at the head, to maximise cache
 * locality.
 *
 * Protected by logger_queue_mutex.
 */
static LinkedList logger_freelist;
/**
 * Queue of pending log lines.
 *
 * Log messages are added at the tail.  The logger thread removes them
 * from the head.
 *
 * Protected by logger_queue_mutex.
 */
static LinkedList logger_queue;
/**
 * Active flag for the logger thread.
 *
 * When set to false, the logger thread will exit.
 *
 * This is atomic because it is checked without synchronisation to
 * shortcut log message creation when the logger thread is inactive.
 */
static atomic_int logger_thread_active = ATOMIC_VAR_INIT(0);
/**
 * Pause flag for the logger thread.
 *
 * While true, the logger thread will not process any messages.  This
 * is probably only useful for testing.
 */
static bool logger_thread_pause;
/**
 * Number of threads which are going to write to the queue.
 *
 * We can only destroy the logger once this reaches zero.
 */
static atomic_int logger_thread_writers = ATOMIC_VAR_INIT(0);
/**
 * Mutex protecting the logger queue, freelist and thread-active.
 */
static WhistMutex logger_queue_mutex;
/**
 * Condition variable: set when logging exiting or when the logger queue
 * is nonempty.
 */
static WhistCondition logger_queue_cond;
/**
 * Logger thread handle.
 */
static WhistThread logger_thread = NULL;
/**
 * Mutex protecting crash handling code.
 */
static WhistMutex crash_handler_mutex;
/**
 * True if crash_handler_mutex is initialized
 */
static bool crash_handler_mutex_active;

static void log_single_line(unsigned int level, const char* line) {
    // Log to stdout.
    fputs(line, stdout);

    // Log to the error monitor.
    if (level == WARNING_LEVEL) {
        whist_error_monitor_log_breadcrumb(tag_strings[level], line);
    } else if (level <= ERROR_LEVEL) {
        whist_error_monitor_log_error(line);
    }
}

static void log_flush_output(void) {
    // Only the log to stdout needs an explicit flush.
    fflush(stdout);
}

static int logger_thread_function(void* ignored) {
    // Logger thread: while active, repeatedly fetch a single item from
    // the logger queue and log it.
    whist_lock_mutex(logger_queue_mutex);
    while (1) {
        int thread_active, queue_size;
        while (1) {
            thread_active = atomic_load(&logger_thread_active);
            queue_size = linked_list_size(&logger_queue);
            if (!logger_thread_pause && (!thread_active || queue_size > 0)) break;
            whist_wait_cond(logger_queue_cond, logger_queue_mutex);
        }
        if (!thread_active) break;
        LoggerQueueItem* log = linked_list_extract_head(&logger_queue);
        whist_unlock_mutex(logger_queue_mutex);

        log_single_line(log->level, log->buf);
        if (queue_size > 1) {
            // We will be printing another line immediately, so don't
            // want an explicit flush.
        } else {
            // Flush on the last log line.
            log_flush_output();
        }

        whist_lock_mutex(logger_queue_mutex);
        linked_list_add_head(&logger_freelist, log);
    }
    whist_unlock_mutex(logger_queue_mutex);
    return 0;
}

void flush_logs(void) {
    // Flush the whole log queue.
    bool locked = false;
    if (atomic_load(&logger_thread_active)) {
        // If the logger thread is active then wait until it has flushed
        // the queue itself.  If it looks like we are waiting too long,
        // then assume the logger thread is stuck somehow and do the
        // flush ourselves.  (Trying to write at the same time as the
        // logger thread leads to messages being reordered.)

        WhistTimer timer;
        start_timer(&timer);
        while (1) {
            int queued_messages;

            whist_lock_mutex(logger_queue_mutex);
            queued_messages = linked_list_size(&logger_queue);
            whist_unlock_mutex(logger_queue_mutex);

            if (queued_messages == 0) {
                // Logger thread has flushed all outstanding logs, done.
                return;
            }

            if (get_timer(&timer) > 1.0) {
                // We've waited too long, give up.
                break;
            }
            whist_sleep(10);
        }

        whist_lock_mutex(logger_queue_mutex);
        locked = true;
    } else {
        // If the logger thread isn't active then we can empty the
        // queue without taking the mutex.
    }

    LoggerQueueItem* log;
    while ((log = linked_list_extract_head(&logger_queue))) {
        log_single_line(log->level, log->buf);
        linked_list_add_head(&logger_freelist, log);
    }

    if (locked) {
        whist_unlock_mutex(logger_queue_mutex);
    }

    log_flush_output();
}

void test_set_pause_state_on_logger_thread(bool pause) {
    whist_lock_mutex(logger_queue_mutex);
    logger_thread_pause = pause;
    whist_broadcast_cond(logger_queue_cond);
    whist_unlock_mutex(logger_queue_mutex);
}

static size_t copy_and_escape(char* dst, size_t dst_size, const char* src) {
    size_t d, s;

    // Table of escape codes.
    static const unsigned char escape_list[256] = {
        ['\b'] = 'b',
        ['\f'] = 'f',
        ['\r'] = 'r',
        ['\t'] = 't',
    };

    for (d = s = 0; src[s]; s++) {
        unsigned char esc = escape_list[src[s] & 0xff];
        if (esc) {
            if (d >= dst_size - 2) break;
            dst[d++] = '\\';
            dst[d++] = esc;
        } else {
            if (d >= dst_size - 1) break;
            dst[d++] = src[s];
        }
    }

    dst[d] = '\0';
    return d;
}

static void logger_queue_line(unsigned int level, const char* prefix, const char* line) {
    LoggerQueueItem* log;
    bool overflow = false;

    // Get a free queue item to write to.
    whist_lock_mutex(logger_queue_mutex);
    log = linked_list_extract_head(&logger_freelist);
    if (!log) {
        // Logger queue has overflowed: steal the oldest unwritten
        // message from the queue and replace it.
        log = linked_list_extract_head(&logger_queue);
        if (!log) {
            // Each thread can only hold one queue item at a time, so
            // this implies that at least LOGGER_QUEUE_SIZE threads are
            // running and all of them are acting on queue items right
            // now.  Something has gone very wrong if that is true, so
            // just send our message to stdout and give up.
            whist_unlock_mutex(logger_queue_mutex);
            printf("%s\n", line);
            return;
        }
        overflow = true;
    }
    whist_unlock_mutex(logger_queue_mutex);

    // Write queue item.
    char* buf = log->buf;
    size_t buf_size = sizeof(log->buf);
    size_t pos = 0;
    if (prefix) {
        size_t prefix_size = strlen(prefix);
        strcpy(buf, prefix);
        pos += prefix_size;
    }
    pos += copy_and_escape(buf + pos, buf_size - pos, line);

    // If we overflowed, note after the message that it happened.
    if (overflow) {
        int ret = snprintf(buf + pos, buf_size - pos, "\nLog buffer overflowing!");
        if (ret > 0) pos += ret;
        // Overflowing is always at least a warning.
        if (level > WARNING_LEVEL) {
            level = WARNING_LEVEL;
        }
    }

    // If we truncated, add ellipses to indicate that.
    if (pos >= buf_size - 5) {
        pos = buf_size - 2;
        buf[pos - 3] = '.';
        buf[pos - 2] = '.';
        buf[pos - 1] = '.';
    }
    // Add trailing newline.
    buf[pos++] = '\n';
    buf[pos] = '\0';

    log->level = level;

    // Add the item to the back of the logger queue.
    whist_lock_mutex(logger_queue_mutex);
    linked_list_add_tail(&logger_queue, log);
    whist_broadcast_cond(logger_queue_cond);
    whist_unlock_mutex(logger_queue_mutex);
}

static bool do_not_catch_segfaults;
// Passing the developer mode flag without any parameter will set do_not_catch_segfaults to true by
// default.
COMMAND_LINE_BOOL_OPTION(do_not_catch_segfaults, 'D', "developer-mode",
                         "Run the server in developer mode (don't catch segfaults).")

static void ffmpeg_log_callback(void* av_class, int av_level, const char* fmt, va_list vl);
static int ffmpeg_log_level = AV_LOG_WARNING;
COMMAND_LINE_INT_OPTION(ffmpeg_log_level, 0, "ffmpeg-log", AV_LOG_QUIET, AV_LOG_TRACE,
                        "Include FFmpeg log output of the given level in logs.")

void whist_init_logger(void) {
    if (!do_not_catch_segfaults) init_backtrace_handler();

    linked_list_init(&logger_queue);
    linked_list_init(&logger_freelist);

    // Fill message freelist with our static array of empty items.
    static LoggerQueueItem logger_queue_items[LOGGER_QUEUE_SIZE];
    for (int i = 0; i < LOGGER_QUEUE_SIZE; i++)
        linked_list_add_tail(&logger_freelist, &logger_queue_items[i]);

    logger_queue_mutex = whist_create_mutex();
    logger_queue_cond = whist_create_cond();

    // Enable threaded logging so other threads can use it now.
    atomic_store(&logger_thread_active, 1);

    // Create the logger thread to handle log messages.
    logger_thread = whist_create_thread(&logger_thread_function, "Logger Thread", NULL);

    av_log_set_callback(ffmpeg_log_callback);

    LOG_INFO("Logging initialized!");
}

void destroy_logger(void) {
    // Signal the logger thread to exit, then join it.
    whist_lock_mutex(logger_queue_mutex);
    atomic_store(&logger_thread_active, 0);
    whist_broadcast_cond(logger_queue_cond);
    whist_unlock_mutex(logger_queue_mutex);
    whist_wait_thread(logger_thread, NULL);

    // Ensure that all outstanding writers have completed.
    FATAL_ASSERT(atomic_load(&logger_thread_writers) == 0);

    // Flush any outstanding logs from the queue.
    flush_logs();

    // Destroy mutexes.
    whist_destroy_mutex(logger_queue_mutex);
    whist_destroy_cond(logger_queue_cond);

    crash_handler_mutex_active = false;
    whist_destroy_mutex(crash_handler_mutex);

    // Debug check: if we get here and the logger queue is not empty
    // and the freelist full then something has gone wrong.
    if (linked_list_size(&logger_queue) != 0 ||
        linked_list_size(&logger_freelist) != LOGGER_QUEUE_SIZE) {
        LOG_WARNING("Logger queue was not emptied correctly (%d, %d).",
                    linked_list_size(&logger_queue), linked_list_size(&logger_freelist));
    }
}

static void logger_queue_multiple_lines(unsigned int level, char* message) {
    // use strtok_r over strtok due to thread safety
    char* strtok_context = NULL;  // strtok_r context var

    // Log the first line out of the loop because we log it with
    // the full log formatting time | type | file | log_msg
    // subsequent lines start with | followed by 4 spaces
    char* current_line = strtok_r(message, "\n", &strtok_context);
    logger_queue_line(level, NULL, current_line);

    // Now, log the rest of the lines with the indent of 4 spaces
    current_line = strtok_r(NULL, "\n", &strtok_context);
    while (current_line != NULL) {
        logger_queue_line(level, "|    ", current_line);
        current_line = strtok_r(NULL, "\n", &strtok_context);
    }
}

// This is the common part of the normal and rate-limited logging
// functions below.  The interface matches, so if there were any
// external use for it then it could be made public as well.
static void whist_log_vprintf(unsigned int level, const char* file_name, const char* function,
                              int line_number, const char* fmt_str, va_list args) {
    // if enabled, supress log after some num of logs are printed
    if (SUPRESS_LOG_AFTER > 0) {
        // increase counter of current log printed
        g_log_cnt++;

        // empahse on the log has been supressed
        if (g_log_cnt > SUPRESS_LOG_AFTER && g_log_cnt <= SUPRESS_LOG_AFTER + 5) {
            fprintf(stderr, "normal logs are supressed from now on!!!\n");
        }

        // skip log when we have printed enough
        if (g_log_cnt > SUPRESS_LOG_AFTER) {
            return;
        }
    }

    char stack_buffer[512];
    char* heap_buffer = NULL;
    char* buffer = stack_buffer;
    int position = 0;
    int remaining_size = sizeof(stack_buffer);
    int ret;

    // Attempt to construct the message on the stack, but if it is too
    // long then allocate a heap buffer to use instead.  It is assumed
    // here that the context information will not exceed the default
    // size of the heap buffer - if this is not true then that section
    // will be dropped but the rest should still be logged.

    ret = current_time_str(buffer + position, remaining_size);
    if (ret > 0 && ret <= remaining_size) {
        position += ret;
        remaining_size -= ret;
    }

    const char* tag;
    if (level < (int)ARRAY_LENGTH(tag_strings)) {
        tag = tag_strings[level];
    } else {
        tag = "INVALID";
    }

    ret = snprintf(buffer + position, remaining_size, LOG_CONTEXT_FORMAT, tag, file_name, function,
                   line_number);
    if (ret > 0 && ret <= remaining_size) {
        position += ret;
        remaining_size -= ret;
    }

    va_list args2;
    va_copy(args2, args);

    ret = vsnprintf(buffer + position, remaining_size, fmt_str, args);
    if (ret < 0) {
        // Message is invalid, but we can still try to print the context.
        buffer[position] = '\0';
    } else if (ret >= 0 && ret < remaining_size) {
        // Message fits in stack buffer, yay.
    } else {
        // Message doesn't fit in stack buffer, allocate a new buffer on
        // the heap to use instead.
        remaining_size = ret + 1;
        heap_buffer = safe_malloc(position + remaining_size);
        memcpy(heap_buffer, buffer, position);
        buffer = heap_buffer;
        ret = vsnprintf(buffer + position, remaining_size, fmt_str, args2);
        if (ret < 0) {
            // Something has gone horribly wrong.
            buffer[position] = '\0';
        }
    }

    atomic_fetch_add(&logger_thread_writers, 1);

    if (atomic_load(&logger_thread_active)) {
        logger_queue_multiple_lines(level, buffer);
    } else {
        // Logger is not running, just write to stdout.
        fputs(buffer, stdout);
        fflush(stdout);
    }

    atomic_fetch_sub(&logger_thread_writers, 1);

    va_end(args2);

    free(heap_buffer);
}

// This is the entry point for normal log messages.
void whist_log_printf(unsigned int level, const char* file_name, const char* function,
                      int line_number, const char* fmt_str, ...) {
    va_list args;
    va_start(args, fmt_str);

    whist_log_vprintf(level, file_name, function, line_number, fmt_str, args);

    va_end(args);
}

// This is the entry point for rate-limited log messages.
void whist_log_printf_rate_limited(LogRateLimiter* rate_limiter, unsigned int level,
                                   const char* file_name, const char* function, int line_number,
                                   const char* fmt_str, ...) {
    int suppressions = 0;
    double seconds_since_start = 0.0;
    if (rate_limiter->started) {
        seconds_since_start = get_timer(&rate_limiter->timer);
        if (seconds_since_start > rate_limiter->period_in_seconds) {
            if (rate_limiter->message_count > rate_limiter->max_messages_per_period) {
                suppressions = rate_limiter->message_count - rate_limiter->max_messages_per_period;
            }
            start_timer(&rate_limiter->timer);
            rate_limiter->message_count = 1;
        } else {
            ++rate_limiter->message_count;
            if (rate_limiter->message_count > rate_limiter->max_messages_per_period) {
                return;
            }
        }
    } else {
        start_timer(&rate_limiter->timer);
        rate_limiter->message_count = 1;
        rate_limiter->started = true;
    }

    va_list args;
    va_start(args, fmt_str);

    whist_log_vprintf(level, file_name, function, line_number, fmt_str, args);

    va_end(args);

    if (suppressions > 0) {
        whist_log_printf(level, file_name, function, line_number,
                         "   (%u messages suppressed since %f seconds ago.)", suppressions,
                         seconds_since_start);
    }
}

static void ffmpeg_log_callback(void* class, int av_level, const char* fmt, va_list vl) {
    if (av_level > ffmpeg_log_level) {
        return;
    }

    // Map the AV_LOG_* level to our log levels.
    unsigned int level;
    if (av_level <= AV_LOG_WARNING) {
        level = WARNING_LEVEL;
    } else if (av_level <= AV_LOG_INFO) {
        level = INFO_LEVEL;
    } else {
        level = DEBUG_LEVEL;
    }
    // We don't have the same details as a log line at a known location,
    // but we do have other context information to fill these fields.
    // File name -> "FFmpeg" + instance type (e.g. "AVCodecContext").
    // Function name -> instance name (e.g. "h264") + instance pointer.
    // Line number -> FFmpeg log level.
    AVClass* av_class = class ? *(AVClass**)class : NULL;
    char code_context[36];
    char instance_context[32];
    if (av_class) {
        snprintf(code_context, sizeof(code_context), "FFmpeg_%s", av_class->class_name);
        snprintf(instance_context, sizeof(instance_context), "%s_%#x", av_class->item_name(class),
                 (unsigned int)(intptr_t) class);
    } else {
        snprintf(code_context, sizeof(code_context), "FFmpeg");
        snprintf(instance_context, sizeof(instance_context), "%p", class);
    }
    whist_log_vprintf(level, code_context, instance_context, av_level, fmt, vl);
}

void print_stacktrace(void) {
    /*
        Prints the stacktrace that led to the point at which this function was called.

        NOTE: when updating this function, do NOT add anything that calls `malloc`.
        This function is called when signals are handled, and a SIGABRT during a
        `malloc` can cause `malloc`s called by this function to hang.
    */

    if (crash_handler_mutex_active) {
        whist_lock_mutex(crash_handler_mutex);
        // Flush out all of the logs that occured prior to the stacktrace
        flush_logs();
    } else {
        fprintf(stdout, "ERROR: No crash handler mutex...\n");
    }

#ifdef _WIN32
    unsigned int i;
    void* stack[100];
    unsigned short frames;
    char buf[sizeof(SYMBOL_INFO) + 256 * sizeof(char)];
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)buf;
    HANDLE process;

    process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);

    frames = CaptureStackBackTrace(0, 100, stack, NULL);
    memset(symbol, 0, sizeof(buf));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);

        fprintf(stdout, "%i: %s - 0x%0llx\n", frames - i - 1, symbol->Name, symbol->Address);
    }
#else
#define HANDLER_ARRAY_SIZE 100
    fprintf(stdout, "Printing backtrace...\n");
    void* trace[HANDLER_ARRAY_SIZE];
    size_t trace_size;

    // get void*'s for all entries on the stack
    trace_size = backtrace(trace, HANDLER_ARRAY_SIZE);

    // Get the backtrace symbols
    // Print stacktrace to stdout - use backtrace_symbols_fd instead of
    //     backtrace_symbols because SIGABRT during a `malloc` can cause hangs
    backtrace_symbols_fd(trace, trace_size, STDOUT_FILENO);

    // Print addr2line commands
    for (int i = 1; i < (int)trace_size; i++) {
        // Storage for addr2line command
        char cmd[2048];

        // Generate addr2line command
        void* ptr = trace[i];
        Dl_info info;
        if (dladdr(ptr, &info)) {
            // Update ptr to be an offset from the dl's base
            ptr = (void*)((char*)ptr - (char*)info.dli_fbase);
            snprintf(cmd, sizeof(cmd), "addr2line -fp -e %s -i %p", info.dli_fname, ptr);
            // Can only run on systems with addr2line
            // runcmd(cmd, NULL);
        } else {
            snprintf(cmd, sizeof(cmd), "echo ??");
        }

        // Write addr2line command to logs
        fprintf(stdout, "%s\n", cmd);
    }
#endif
    // Print out the final newlines and flush
    fprintf(stdout, "\n\n");
    fflush(stdout);

    if (crash_handler_mutex_active) {
        whist_unlock_mutex(crash_handler_mutex);
    } else {
        // Exit after printing stacktrace, if a crash happens without a crash handler mutex
        exit(-1);
    }
}

#ifdef _WIN32
static LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS* ExceptionInfo) {  // NOLINT
    fprintf(stdout, "\n");
    switch (ExceptionInfo->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            fprintf(stdout, "Error: EXCEPTION_ACCESS_VIOLATION\n");
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            fprintf(stdout, "Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n");
            break;
        case EXCEPTION_BREAKPOINT:
            fprintf(stdout, "Error: EXCEPTION_BREAKPOINT\n");
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            fprintf(stdout, "Error: EXCEPTION_DATATYPE_MISALIGNMENT\n");
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            fprintf(stdout, "Error: EXCEPTION_FLT_DENORMAL_OPERAND\n");
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            fprintf(stdout, "Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n");
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            fprintf(stdout, "Error: EXCEPTION_FLT_INEXACT_RESULT\n");
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            fprintf(stdout, "Error: EXCEPTION_FLT_INVALID_OPERATION\n");
            break;
        case EXCEPTION_FLT_OVERFLOW:
            fprintf(stdout, "Error: EXCEPTION_FLT_OVERFLOW\n");
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            fprintf(stdout, "Error: EXCEPTION_FLT_STACK_CHECK\n");
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            fprintf(stdout, "Error: EXCEPTION_FLT_UNDERFLOW\n");
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            fprintf(stdout, "Error: EXCEPTION_ILLEGAL_INSTRUCTION\n");
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            fprintf(stdout, "Error: EXCEPTION_IN_PAGE_ERROR\n");
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            fprintf(stdout, "Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n");
            break;
        case EXCEPTION_INT_OVERFLOW:
            fprintf(stdout, "Error: EXCEPTION_INT_OVERFLOW\n");
            break;
        case EXCEPTION_INVALID_DISPOSITION:
            fprintf(stdout, "Error: EXCEPTION_INVALID_DISPOSITION\n");
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            fprintf(stdout, "Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n");
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            fprintf(stdout, "Error: EXCEPTION_PRIV_INSTRUCTION\n");
            break;
        case EXCEPTION_SINGLE_STEP:
            fprintf(stdout, "Error: EXCEPTION_SINGLE_STEP\n");
            break;
        case EXCEPTION_STACK_OVERFLOW:
            fprintf(stdout, "Error: EXCEPTION_STACK_OVERFLOW\n");
            break;
        default:
            fprintf(stdout, "Error: Unrecognized Exception\n");
            break;
    }
    /* If this is a stack overflow then we can't walk the stack, so just show
      where the error happened */
    if (EXCEPTION_STACK_OVERFLOW != ExceptionInfo->ExceptionRecord->ExceptionCode) {
        print_stacktrace();
    } else {
        fprintf(stdout, "Can't show stacktrace when the stack has overflowed!\n");
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#else
static void unix_crash_handler(int sig) {
    fprintf(stdout, "\nError: signal %d:%s\n", sig, strsignal(sig));
    print_stacktrace();
    if (whist_error_monitor_is_initialized()) {
        // We reset the signal handler to default to allow the error monitor
        // to handle the crash without getting stuck in an infinite loop of
        // crash signal handling
        struct sigaction sa = {0};
        sa.sa_handler = SIG_DFL;
        sigaction(sig, &sa, NULL);

        // Then we must re-raise the signal to allow the error monitor to
        // handle it
        raise(sig);
    } else {
        // If the error monitor isn't initialized, we just exit
        exit(1);
    }
}
#endif

static void init_backtrace_handler(void) {
    crash_handler_mutex = whist_create_mutex();
    crash_handler_mutex_active = true;
#ifdef _WIN32
    SetUnhandledExceptionFilter(windows_exception_handler);
#else
    // Try to catch all the signals
    struct sigaction sa = {0};
    sa.sa_handler = &unix_crash_handler;
    for (int i = 1; i < NSIG; i++) {
        // TODO: We should gracefully exit on SIGTERM
        // We do nothing on SIGCHLD, SIGPIPE, and SIGWINCH
        // We crash on anything else
        if (i != SIGTERM && i != SIGCHLD && i != SIGPIPE && i != SIGWINCH) {
            sigaction(i, &sa, NULL);
        }
    }
    // Ignore SIGPIPE, just let the syscall return EPIPE
    sa.sa_handler = SIG_IGN;
    // Without restarting the syscall, it'll forcefully return EINTR
    sa.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sa, NULL);
#endif
}
