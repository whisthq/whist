/**
 * Copyright Fractal Computers, Inc. 2020
 * @file logging.c
 * @brief This file contains the logging macros and utils to send Winlogon
 *        status and to send the logs to the webserver.
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

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

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

#include <fractal/core/fractal.h>
#include <fractal/network/network.h>
#include "logging.h"
#include "error_monitor.h"

char* get_logger_history();
int get_logger_history_len();
void init_backtrace_handler();

// TAG Strings
const char* debug_tag = "DEBUG";
const char* info_tag = "INFO";
const char* warning_tag = "WARNING";
const char* error_tag = "ERROR";
const char* fatal_error_tag = "FATAL_ERROR";

// logger Semaphores and Mutexes
static volatile FractalSemaphore logger_semaphore;
static volatile FractalMutex logger_queue_mutex;

// logger queue
typedef struct LoggerQueueItem {
    const char* tag;
    char buf[LOGGER_BUF_SIZE];
} LoggerQueueItem;
static volatile LoggerQueueItem logger_queue[LOGGER_QUEUE_SIZE];
static volatile LoggerQueueItem logger_queue_cache[LOGGER_QUEUE_SIZE];
static volatile int logger_queue_index = 0;
static volatile int logger_queue_size = 0;

// logger global variables
FractalThread mprintf_thread = NULL;
static volatile bool run_multithreaded_printf;
int multithreaded_printf(void* opaque);
void mprintf(const char* tag, const char* fmt_str, va_list args);

// This is written to in MultiThreaderPrintf
#define LOG_CACHE_SIZE 1000000
char logger_history[LOG_CACHE_SIZE];
int logger_history_len;

char* get_logger_history() { return logger_history; }
int get_logger_history_len() { return logger_history_len; }

void init_logger() {
    /*
        Initializes the Fractal logger.

        Arguments:
            None

        Returns:
            None
    */

    init_backtrace_handler();
    logger_history_len = 0;

    run_multithreaded_printf = true;
    logger_queue_mutex = fractal_create_mutex();
    logger_semaphore = fractal_create_semaphore(0);
    logger_queue_mutex = fractal_create_mutex();
    mprintf_thread = fractal_create_thread((FractalThreadFunction)multithreaded_printf,
                                           "MultiThreadedPrintf", NULL);
    LOG_INFO("Logging initialized!");
}

void destroy_logger() {
    // Flush out any remaining logs
    flush_logs();

    run_multithreaded_printf = false;
    fractal_post_semaphore((FractalSemaphore)logger_semaphore);

    fractal_wait_thread(mprintf_thread, NULL);
    mprintf_thread = NULL;

    logger_history[0] = '\0';
    logger_history_len = 0;
}

int multithreaded_printf(void* opaque) {
    UNUSED(opaque);

    while (true) {
        // Wait until signaled by printf to begin running
        fractal_wait_semaphore((FractalSemaphore)logger_semaphore);

        if (!run_multithreaded_printf) {
            break;
        }

        flush_logs();
    }

    return 0;
}

void flush_logs() {
    if (run_multithreaded_printf) {
        // Clear the queue into the cache,
        // And then let go of the mutex so that printf can continue accumulating
        fractal_lock_mutex((FractalMutex)logger_queue_mutex);
        int cache_size = 0;
        cache_size = logger_queue_size;
        for (int i = 0; i < logger_queue_size; i++) {
            safe_strncpy((char*)logger_queue_cache[i].buf,
                         (const char*)logger_queue[logger_queue_index].buf, LOGGER_BUF_SIZE);
            logger_queue_cache[i].tag = logger_queue[logger_queue_index].tag;
            logger_queue[logger_queue_index].buf[0] = '\0';
            logger_queue_index++;
            logger_queue_index %= LOGGER_QUEUE_SIZE;
            if (i != 0) {
                fractal_wait_semaphore((FractalSemaphore)logger_semaphore);
            }
        }
        logger_queue_size = 0;
        fractal_unlock_mutex((FractalMutex)logger_queue_mutex);

        // Print all of the data into the cache
        for (int i = 0; i < cache_size; i++) {
            logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 5] = '.';
            logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 4] = '.';
            logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 3] = '.';
            logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 2] = '\n';
            logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 1] = '\0';

            // Log to stdout
            fprintf(stdout, "%s", logger_queue_cache[i].buf);

            // Log to the error monitor
            const char* tag = logger_queue_cache[i].tag;
            if (tag == WARNING_TAG) {
                error_monitor_log_breadcrumb(tag, (const char*)logger_queue_cache[i].buf);
            } else if (tag == ERROR_TAG || tag == FATAL_ERROR_TAG) {
                error_monitor_log_error((const char*)logger_queue_cache[i].buf);
            }

            // Log to the logger history buffer
            int chars_written =
                sprintf(&logger_history[logger_history_len], "%s", logger_queue_cache[i].buf);
            logger_history_len += chars_written;

            // Shift buffer over if too large;
            if ((unsigned long)logger_history_len >
                sizeof(logger_history) - sizeof(logger_queue_cache[i].buf) - 10) {
                int new_len = sizeof(logger_history) / 3;
                for (i = 0; i < new_len; i++) {
                    logger_history[i] = logger_history[logger_history_len - new_len + i];
                }
                logger_history_len = new_len;
            }
        }
    }

    // Flush the logs
    fflush(stdout);
}

/**
 * This function escapes certain escape sequences in a log. It allocates heap
 * memory that must be later freed. Specifically, it by-default escapes "\b", "\f", "\r", "\t"
 * (Note: This comment is finicky with doxygen when it comes to escape sequences,
 * so double-check the doxygen docs if you're changing this documentation comment)
 *
 * @param old_string               A string with sequences we want escaped,
 *                                 e.g "some\tstring\r\n\r\n"
 * @param escape_all               If true, also escapes ", \, and newlines
 * @return                         The same string, escaped using only basic ASCII,
 *                                 "some\\\tstuff\\\r\n\\\r\n"
 *                                 "some\\\tstuff\\\r\\\n\\\r\\\n" <- escape_all=true
 */
char* escape_string(const char* old_string, bool escape_all) {
    size_t old_string_len = strlen(old_string);
    char* new_string = safe_malloc(2 * (old_string_len + 1));
    int new_str_len = 0;
    for (size_t i = 0; i < old_string_len; i++) {
        switch (old_string[i]) {
            case '\b':
                new_string[new_str_len++] = '\\';
                new_string[new_str_len++] = 'b';
                break;
            case '\f':
                new_string[new_str_len++] = '\\';
                new_string[new_str_len++] = 'f';
                break;
            case '\r':
                new_string[new_str_len++] = '\\';
                new_string[new_str_len++] = 'r';
                break;
            case '\t':
                new_string[new_str_len++] = '\\';
                new_string[new_str_len++] = 't';
                break;
            case '\"':
                if (escape_all) {
                    new_string[new_str_len++] = '\\';
                    new_string[new_str_len++] = '\"';
                    break;
                }
                // fall through
            case '\\':
                if (escape_all) {
                    new_string[new_str_len++] = '\\';
                    new_string[new_str_len++] = '\\';
                    break;
                }
                // fall through
            case '\n':
                if (escape_all) {
                    new_string[new_str_len++] = '\\';
                    new_string[new_str_len++] = 'n';
                    break;
                }
                // fall through
            default:
                new_string[new_str_len++] = old_string[i];
                break;
        }
    }
    new_string[new_str_len++] = '\0';
    return new_string;
}

// Our vararg function that gets called from LOG_INFO, LOG_WARNING, etc macros
void internal_logging_printf(const char* tag, const char* fmt_str, ...) {
    va_list args;
    va_start(args, fmt_str);

    if (mprintf_thread == NULL) {
        // If the logger isn't initialized yet, just write to stdout
        vprintf(fmt_str, args);
        fflush(stdout);
    } else {
        // Otherwise, use mprintf
        mprintf(tag, fmt_str, args);
    }

    va_end(args);
}

void mprintf_queue_line(const char* line_fmt, const char* tag, const char* line) {
    /*
        Queues a line of a log message to be printed by the logger thread. Warns the
        user if this queued line is overwriting an existing entry on the queue or if
        the queue is full.

        Note:
            This should be called from `mprintf`, with a lock on `logger_queue_mutex` held.

        Arguments:
            line_fmt (const char*): How to format the line (for the first line of a
                message, this can just be "%s\n". For subsequent lines, we need to
                indent the line by a few spaces for clarity.)
            tag (const char*): The tag to use for the log message
            line (const char*): The (unsanitized) log message itself
    */

    if (logger_queue_size >= LOGGER_QUEUE_SIZE - 1 || tag == NULL || line == NULL || line_fmt == NULL) {
        // If the queue is full, we just drop the log
        return;
    }
    int index = (logger_queue_index + logger_queue_size) % LOGGER_QUEUE_SIZE;
    char* dest = (char*)logger_queue[index].buf;

    char* sanitized_line = escape_string(line, false);

    if (logger_queue_size == LOGGER_QUEUE_SIZE - 2) {
        // If the queue is becoming full, warn the user
        // Technically we should check if we are overwriting an existing message,
        // but the only really important thing is that the queue is full.
        snprintf(dest, LOGGER_QUEUE_SIZE, "%s\nLog buffer maxed out!\n", sanitized_line);

        // Automatically make queue fills a warning
        logger_queue[index].tag = WARNING_TAG;
    } else if (logger_queue[index].buf[0] != '\0') {
        // If we are overwriting an existing message, warn the user
        // We ignore `line_fmt` here because indenting make it hard to read
        char old_message[LOGGER_BUF_SIZE];
        memcpy(old_message, dest, LOGGER_BUF_SIZE);
        int written = snprintf(dest, LOGGER_BUF_SIZE, "Log overwrite!\nOLD | %s\nNEW | %s\n",
                               old_message, sanitized_line);
        if (written < 0 || written >= LOGGER_BUF_SIZE) {
            // This should never happen, but just in case...
            dest[LOGGER_BUF_SIZE - 1] = '\0';
        }
        // Automatically make overwrites a warning
        logger_queue[index].tag = WARNING_TAG;
    } else {
        // Normally, just copy the message according to the line format
        snprintf(dest, LOGGER_BUF_SIZE, line_fmt, sanitized_line);
        logger_queue[index].tag = tag;
    }

    free(sanitized_line);

    ++logger_queue_size;
    fractal_post_semaphore((FractalSemaphore)logger_semaphore);
}

// Core multithreaded printf function, that accepts va_list and log boolean
void mprintf(const char* tag, const char* fmt_str, va_list args) {
    fractal_lock_mutex((FractalMutex)logger_queue_mutex);

    // After calls to function which invoke VA args, the args are
    // undefined so we copy
    va_list args_copy;
    va_copy(args_copy, args);

    // Get the length of the formatted string with args replaced.
    int len = vsnprintf(NULL, 0, fmt_str, args) + 1;

    // Print to a temp buf so we can split on \n
    char* full_message = safe_malloc(sizeof(char) * (len + 1));
    vsnprintf(full_message, len, fmt_str, args_copy);

    // Make sure to close the copied args
    va_end(args_copy);

    // use strtok_r over strtok due to thread safety
    char* strtok_context = NULL;  // strtok_r context var

    // Log the first line out of the loop because we log it with
    // the full log formatting time | type | file | log_msg
    // subsequent lines start with | followed by 4 spaces
    char* current_line = strtok_r(full_message, "\n", &strtok_context);
    mprintf_queue_line("%s\n", tag, current_line);

    // Now, log the rest of the lines with the indent of 4 spaces
    current_line = strtok_r(NULL, "\n", &strtok_context);
    while (current_line != NULL) {
        mprintf_queue_line("|    %s\n", tag, current_line);
        current_line = strtok_r(NULL, "\n", &strtok_context);
    }

    // Free the temp buf, making sure to do it after we're done with `strtok_r`
    free(full_message);

    fractal_unlock_mutex((FractalMutex)logger_queue_mutex);
}

FractalMutex crash_handler_mutex;

void print_stacktrace() {
    fractal_lock_mutex(crash_handler_mutex);

    // Flush out all of the logs that occured prior to the stacktrace
    flush_logs();

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

    void* trace[HANDLER_ARRAY_SIZE];
    size_t trace_size;

    // get void*'s for all entries on the stack
    trace_size = backtrace(trace, HANDLER_ARRAY_SIZE);

    // Get the backtrace symbols
    char** messages;
    messages = backtrace_symbols(trace, trace_size);

    // Print stacktrace to stdout
    // Print backtrace messages
    for (int i = 1; i < (int)trace_size; i++) {
        fprintf(stdout, "[backtrace #%02d] %s\n", i, messages[i]);
    }
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
    fractal_unlock_mutex(crash_handler_mutex);
}

#ifdef _WIN32
LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS* ExceptionInfo) {  // NOLINT
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
void unix_crash_handler(int sig) {
    fprintf(stdout, "\nError: signal %d:%s\n", sig, strsignal(sig));
    print_stacktrace();
    _exit(-1);
}
#endif

void init_backtrace_handler() {
    crash_handler_mutex = fractal_create_mutex();
#ifdef _WIN32
    SetUnhandledExceptionFilter(windows_exception_handler);
#else
    // Try to catch all the signals
    struct sigaction sa = {0};
    sa.sa_handler = &unix_crash_handler;
    sa.sa_flags = SA_SIGINFO;
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
