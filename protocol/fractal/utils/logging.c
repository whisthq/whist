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
#include "../network/network.h"
#include "logging.h"

// Include Sentry
#include <sentry.h>
#define SENTRY_DSN "https://8e1447e8ea2f4ac6ac64e0150fa3e5d0@o400459.ingest.sentry.io/5373388"

char* get_logger_history();
int get_logger_history_len();
void init_backtrace_handler();

// TAG Strings
const char* debug_tag = "DEBUG";
const char* info_tag = "INFO";
const char* warning_tag = "WARNING";
const char* error_tag = "FATAL_ERROR";
const char* fatal_error_tag = "FATAL_ERROR";

extern int connection_id;
extern char sentry_environment[FRACTAL_ARGS_MAXLEN + 1];
extern bool using_sentry;

// logger Semaphores and Mutexes
static volatile FractalSemaphore logger_semaphore;
static volatile FractalMutex logger_mutex;
static volatile FractalMutex log_file_mutex;
static volatile FractalMutex logger_queue_mutex;

// logger queue

typedef struct LoggerQueueItem {
    int id;
    bool log;
    const char* tag;
    char buf[LOGGER_BUF_SIZE];
} LoggerQueueItem;
static volatile LoggerQueueItem logger_queue[LOGGER_QUEUE_SIZE];
static volatile LoggerQueueItem logger_queue_cache[LOGGER_QUEUE_SIZE];
static volatile int logger_queue_index = 0;
static volatile int logger_queue_size = 0;
static volatile int logger_global_id = 0;

// logger global variables
FractalThread mprintf_thread = NULL;
static volatile bool run_multithreaded_printf;
int multi_threaded_printf(void* opaque);
void mprintf(bool log, const char* tag, const char* fmt_str, va_list args);
clock mprintf_timer;
FILE* mprintf_log_file = NULL;
FILE* mprintf_log_connection_file = NULL;
int log_connection_log_id;
char* log_directory = NULL;
size_t log_directory_length;
char* log_file_name = NULL;
size_t log_file_length = 100;
char log_env[FRACTAL_ARGS_MAXLEN];

// This is written to in MultiThreaderPrintf
#define LOG_CACHE_SIZE 1000000
char logger_history[LOG_CACHE_SIZE];
int logger_history_len;

char* get_logger_history() { return logger_history; }
int get_logger_history_len() { return logger_history_len; }

void start_connection_log();

void init_logger(char* log_dir) {
    /*
        Initializes the Fractal logger and starts writing to the log file

        Arguments:
            log_dir (char*): pointer to character array that indicates the directory in which the
       log file should be stored

        Returns:
            None
    */
    init_backtrace_handler();

    logger_history_len = 0;
    if (log_dir) {
        log_directory_length = strlen(log_dir);
        log_directory = (char*)safe_malloc(log_directory_length + 2);
        log_file_name = (char*)safe_malloc(log_directory_length + log_file_length +
                                           2);  // assuming the longest file name is
                                                // {log_directory}log-staging_prev.txt
        safe_strncpy(log_directory, log_dir, log_directory_length + 1);
#if defined(_WIN32)
        log_directory[log_directory_length] = '\\';
#else
        log_directory[log_directory_length] = '/';
#endif
        log_directory[log_directory_length + 1] = '\0';

        // name the initial log file before we know what environment we're in
        safe_strncpy(log_env, "-init", sizeof(log_env));
        snprintf(log_file_name, log_directory_length + log_file_length + 2, "%s%s%s%s",
                 log_directory, "log", log_env, ".txt");

#if defined(_WIN32)
        CreateDirectoryA(log_directory, 0);
#else
        mkdir(log_directory, 0755);
#endif
        printf("Trying to open up %s \n", log_file_name);
        mprintf_log_file = fopen(log_file_name, "ab");
        if (mprintf_log_file == NULL) {
            printf("Couldn't open up logfile\n");
        }
    }

    run_multithreaded_printf = true;
    logger_mutex = fractal_create_mutex();
    logger_semaphore = fractal_create_semaphore(0);
    log_file_mutex = fractal_create_mutex();
    logger_queue_mutex = fractal_create_mutex();
    mprintf_thread = fractal_create_thread((FractalThreadFunction)multi_threaded_printf,
                                           "MultiThreadedPrintf", NULL);
    LOG_INFO("Writing logs to %s", log_file_name);
    //    start_timer(&mprintf_timer);
}

bool init_sentry(char* environment, const char* runner_type) {
    /*
        Initializes sentry based on the environment passed in as an arg

        Arguments:
            environment (char*): production/staging/development
            runner_type (const char*): client or server

        Returns:
            bool: whether sentry was initialized
    */

    // only log "production" and "staging" env sentry events
    if (strcmp(environment, "production") == 0 || strcmp(environment, "staging") == 0) {
        if (!safe_strncpy(sentry_environment, environment, sizeof(sentry_environment))) {
            printf("Sentry environment is too long: %s\n", environment);
            return -1;
        }
        sentry_set_tag("protocol-type", runner_type);

        sentry_options_t* options = sentry_options_new();
        // sentry_options_set_debug(options, true);  // if sentry is playing up uncomment this
        sentry_options_set_dsn(options, SENTRY_DSN);
        // These are used by sentry to classify events and so we can keep track of version specific
        // issues.
        char release[200];
        sprintf(release, "fractal-protocol@%s", fractal_git_revision());
        sentry_options_set_release(options, release);
        sentry_options_set_environment(options, sentry_environment);
        sentry_init(options);

        return true;
    }

    return false;
}

void rename_log_file() {
    /*
        Renames the log file to a name based on the current environment. Should be called after
       init_logger (so the initial log file is created) and after parse_args (so any environment
       variables have been passed in and parsed)

        Arguments:
            None

        Returns:
            None
    */
    fractal_lock_mutex((FractalMutex)log_file_mutex);
    // close the log file before renaming
    if (mprintf_log_file) {
        fclose(mprintf_log_file);
    }

    // use sentry_environment to set new log file name
    char new_log_file_name[1000] = "";
    if (strcmp(sentry_environment, "production") == 0) {
        safe_strncpy(log_env, "", sizeof(log_env));
    } else if (strcmp(sentry_environment, "staging") == 0) {
        safe_strncpy(log_env, "-staging", sizeof(log_env));
    } else {
        safe_strncpy(log_env, "-dev", sizeof(log_env));
    }
    snprintf(new_log_file_name, sizeof(new_log_file_name), "%s%s%s%s", log_directory, "log",
             log_env, ".txt");

    printf("Trying to rename %s to %s\n", log_file_name, new_log_file_name);

    if (rename(log_file_name, new_log_file_name) != 0) {
        printf("Couldn't rename logfile\n");
    }

    // replace old log name with new name
    safe_strncpy(log_file_name, new_log_file_name, log_directory_length + log_file_length + 1);

    // reopen log file to write to
    mprintf_log_file = fopen(log_file_name, "ab");
    if (mprintf_log_file == NULL) {
        printf("Couldn't open up logfile\n");
    }
    fractal_unlock_mutex((FractalMutex)log_file_mutex);
}

// Sets up logs for a new connection, overwriting previous
void start_connection_log() {
    fractal_lock_mutex((FractalMutex)log_file_mutex);

    if (mprintf_log_connection_file) {
        fclose(mprintf_log_connection_file);
    }
    char log_connection_directory[1000] = "";
    strcat(log_connection_directory, log_directory);
    strcat(log_connection_directory, "log_connection.txt");
    mprintf_log_connection_file = fopen(log_connection_directory, "w+b");
    log_connection_log_id = logger_global_id;

    fractal_unlock_mutex((FractalMutex)log_file_mutex);

    LOG_INFO("Beginning connection log");
}

void destroy_logger() {
    // Flush out any remaining logs
    flush_logs();
    if (using_sentry) {
        sentry_shutdown();
    }
    run_multithreaded_printf = false;
    fractal_post_semaphore((FractalSemaphore)logger_semaphore);

    fractal_wait_thread(mprintf_thread, NULL);
    mprintf_thread = NULL;

    if (mprintf_log_file) {
        fclose(mprintf_log_file);
    }
    mprintf_log_file = NULL;

    logger_history[0] = '\0';
    logger_history_len = 0;
    if (log_directory) {
        free(log_directory);
    }
    if (log_file_name) {
        free(log_file_name);
    }
}

void sentry_send_bread_crumb(const char* tag, const char* sentry_str) {
    if (!using_sentry) return;
        // in the current sentry-native beta version, breadcrumbs prevent sentry
        //  from sending Windows events to the dashboard, so we only log
        //  breadcrumbs for OSX and Linux
#ifndef _WIN32
    sentry_value_t crumb = sentry_value_new_breadcrumb("default", sentry_str);
    sentry_value_set_by_key(crumb, "category", sentry_value_new_string("protocol-logs"));
    sentry_value_set_by_key(crumb, "level", sentry_value_new_string(tag));
    sentry_add_breadcrumb(crumb);
#endif
}

void sentry_send_event(const char* sentry_str) {
    if (!using_sentry) return;

    sentry_value_t event = sentry_value_new_message_event(
        /*   level */ SENTRY_LEVEL_ERROR,
        /*  logger */ "client-logs",
        /* message */ sentry_str);
    sentry_capture_event(event);
}

int multi_threaded_printf(void* opaque) {
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
    fractal_lock_mutex((FractalMutex)logger_queue_mutex);
    // Clear the queue into the cache,
    // And then let go of the mutex so that printf can continue accumulating
    fractal_lock_mutex((FractalMutex)logger_mutex);
    int cache_size = 0;
    cache_size = logger_queue_size;
    for (int i = 0; i < logger_queue_size; i++) {
        safe_strncpy((char*)logger_queue_cache[i].buf,
                     (const char*)logger_queue[logger_queue_index].buf, LOGGER_BUF_SIZE);
        logger_queue_cache[i].log = logger_queue[logger_queue_index].log;
        logger_queue_cache[i].tag = logger_queue[logger_queue_index].tag;
        logger_queue_cache[i].id = logger_queue[logger_queue_index].id;
        logger_queue[logger_queue_index].buf[0] = '\0';
        logger_queue_index++;
        logger_queue_index %= LOGGER_QUEUE_SIZE;
        if (i != 0) {
            fractal_wait_semaphore((FractalSemaphore)logger_semaphore);
        }
    }
    logger_queue_size = 0;
    fractal_unlock_mutex((FractalMutex)logger_mutex);

    fractal_lock_mutex((FractalMutex)log_file_mutex);
    // Print all of the data into the cache
    for (int i = 0; i < cache_size; i++) {
        logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 5] = '.';
        logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 4] = '.';
        logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 3] = '.';
        logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 2] = '\n';
        logger_queue_cache[i].buf[LOGGER_BUF_SIZE - 1] = '\0';
        // Log to the log files
        if (logger_queue_cache[i].log) {
            if (mprintf_log_file) {
                fprintf(mprintf_log_file, "%s", logger_queue_cache[i].buf);
            }
            if (mprintf_log_connection_file && logger_queue_cache[i].id >= log_connection_log_id) {
                fprintf(mprintf_log_connection_file, "%s", logger_queue_cache[i].buf);
            }
        }
        // Log to stdout
        fprintf(stdout, "%s", logger_queue_cache[i].buf);
        // Log to sentry
        const char* tag = logger_queue_cache[i].tag;
        if (tag == WARNING_TAG) {
            sentry_send_bread_crumb(tag, (const char*)logger_queue[i].buf);
        } else if (tag == ERROR_TAG || tag == FATAL_ERROR_TAG) {
            sentry_send_event((const char*)logger_queue[i].buf);
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
        //}
    }

    // Flush the logs
    fflush(stdout);
    if (mprintf_log_file) {
        fflush(mprintf_log_file);
    }
    if (mprintf_log_connection_file) {
        fflush(mprintf_log_connection_file);
    }

#define MAX_LOG_FILE_SIZE (5 * BYTES_IN_KILOBYTE * BYTES_IN_KILOBYTE)

    // If the log file is large enough, cache it
    if (mprintf_log_file) {
        fseek(mprintf_log_file, 0L, SEEK_END);
        int sz = ftell(mprintf_log_file);

        // If it's larger than 5MB, start a new file and store the old one
        if (sz > MAX_LOG_FILE_SIZE) {
            fclose(mprintf_log_file);

            char f[1000] = "";
            snprintf(f, sizeof(f), "%s%s%s%s", log_directory, "log", log_env, ".txt");

            char fp[1000] = "";
            snprintf(fp, sizeof(fp), "%s%s%s%s", log_directory, "log", log_env, "_prev.txt");

#if defined(_WIN32)
            WCHAR wf[1000];
            WCHAR wfp[1000];
            mbstowcs(wf, f, sizeof(wf));
            mbstowcs(wfp, fp, sizeof(wfp));
            DeleteFileW(wfp);
            MoveFileW(wf, wfp);
            DeleteFileW(wf);
#endif
            mprintf_log_file = fopen(f, "ab");
        }
    }

    // If the log file is large enough, cache it
    if (mprintf_log_connection_file) {
        fseek(mprintf_log_connection_file, 0L, SEEK_END);
        long sz = ftell(mprintf_log_connection_file);

        // If it's larger than 5MB, start a new file and store the old one
        if (sz > MAX_LOG_FILE_SIZE) {
            long buf_len = MAX_LOG_FILE_SIZE / 2;

            char* original_buf = safe_malloc(buf_len);
            char* buf = original_buf;
            fseek(mprintf_log_connection_file, -buf_len, SEEK_END);
            fread(buf, buf_len, 1, mprintf_log_connection_file);

            while (buf_len > 0 && buf[0] != '\n') {
                buf++;
                buf_len--;
            }

            char f[1000] = "";
            strcat(f, log_directory);
            strcat(f, "log_connection.txt");
            mprintf_log_connection_file = freopen(f, "wb", mprintf_log_connection_file);
            fwrite(buf, buf_len, 1, mprintf_log_connection_file);
            fflush(mprintf_log_connection_file);

            free(original_buf);
        }
    }
    fractal_unlock_mutex((FractalMutex)log_file_mutex);
    fractal_unlock_mutex((FractalMutex)logger_queue_mutex);
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
char* escape_string(char* old_string, bool escape_all) {
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

    // Map to mprintf
    mprintf(WRITE_MPRINTF_TO_LOG, tag, fmt_str, args);
    va_end(args);
}

// Core multithreaded printf function, that accepts va_list and log boolean
void mprintf(bool log, const char* tag, const char* fmt_str, va_list args) {
    if (mprintf_thread == NULL) {
        printf("initLogger has not been called! Printing below...\n");
        vprintf(fmt_str, args);
        return;
    }

    fractal_lock_mutex((FractalMutex)logger_mutex);

    int index = (logger_queue_index + logger_queue_size) % LOGGER_QUEUE_SIZE;
    char* buf = NULL;
    if (logger_queue_size < LOGGER_QUEUE_SIZE - 2) {
        logger_queue[index].log = log;
        logger_queue[index].tag = tag;
        logger_queue[index].id = logger_global_id++;
        buf = (char*)logger_queue[index].buf;

        if (buf[0] != '\0') {
            char old_msg[LOGGER_BUF_SIZE];
            memcpy(old_msg, buf, LOGGER_BUF_SIZE);
            int chars_written =
                snprintf(buf, LOGGER_BUF_SIZE, "OLD MESSAGE: %s\nTRYING TO OVERWRITE WITH: %s\n",
                         old_msg, logger_queue[index].buf);
            if (!(chars_written > 0 && chars_written <= LOGGER_BUF_SIZE)) {
                buf[0] = '\0';
            }
            logger_queue_size++;
            fractal_post_semaphore((FractalSemaphore)logger_semaphore);
        } else {
            // After calls to function which invoke VA args, the args are
            // undefined so we copy
            va_list args_copy;
            va_copy(args_copy, args);

            // Get the length of the formatted string with args replaced.
            int len = vsnprintf(NULL, 0, fmt_str, args) + 1;

            // print to a temp buf so we can split on \n
            char* temp_buf = safe_malloc(sizeof(char) * (len + 1));
            vsnprintf(temp_buf, len, fmt_str, args_copy);
            // use strtok_r over strtok due to thread safety
            char* strtok_context = NULL;  // strtok_r context var
            // Log the first line out of the loop because we log it with
            // the full log formatting time | type | file | log_msg
            // subsequent lines start with | followed by 4 spaces
            char* line = strtok_r(temp_buf, "\n", &strtok_context);
            char* san_line = escape_string(line, false);
            snprintf(buf, LOGGER_BUF_SIZE, "%s \n", san_line);
            free(san_line);
            logger_queue_size++;
            fractal_post_semaphore((FractalSemaphore)logger_semaphore);

            index = (logger_queue_index + logger_queue_size) % LOGGER_QUEUE_SIZE;
            buf = (char*)logger_queue[index].buf;
            line = strtok_r(NULL, "\n", &strtok_context);
            while (line != NULL) {
                san_line = escape_string(line, false);
                snprintf(buf, LOGGER_BUF_SIZE, "|    %s \n", san_line);
                free(san_line);
                logger_queue[index].log = log;
                logger_queue[index].tag = tag;
                logger_queue[index].id = logger_global_id++;
                logger_queue_size++;
                fractal_post_semaphore((FractalSemaphore)logger_semaphore);
                index = (logger_queue_index + logger_queue_size) % LOGGER_QUEUE_SIZE;
                buf = (char*)logger_queue[index].buf;
                line = strtok_r(NULL, "\n", &strtok_context);
            }

            va_end(args_copy);
        }

    } else if (logger_queue_size == LOGGER_QUEUE_SIZE - 2) {
        buf = (char*)logger_queue[index].buf;
        safe_strncpy(buf, "Buffer maxed out!!!\n", LOGGER_BUF_SIZE);
        logger_queue[index].log = log;
        logger_queue[index].tag = tag;
        logger_queue[index].id = logger_global_id++;
        logger_queue_size++;
        fractal_post_semaphore((FractalSemaphore)logger_semaphore);
    }

    fractal_unlock_mutex((FractalMutex)logger_mutex);
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

    fractal_lock_mutex((FractalMutex)log_file_mutex);
    for (i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);

        fprintf(stderr, "%i: %s - 0x%0llx\n", frames - i - 1, symbol->Name, symbol->Address);
        fprintf(mprintf_log_file, "%i: %s - 0x%0llx\n", frames - i - 1, symbol->Name,
                symbol->Address);
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

    // Print stacktrace to stderr and logfile
    fractal_lock_mutex((FractalMutex)log_file_mutex);
    // Print backtrace messages
    for (int i = 1; i < (int)trace_size; i++) {
        fprintf(stderr, "[backtrace #%02d] %s\n", i, messages[i]);
        fprintf(mprintf_log_file, "[backtrace #%02d] %s\n", i, messages[i]);
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
        fprintf(stderr, "%s\n", cmd);
        fprintf(mprintf_log_file, "%s\n", cmd);
    }
#endif
    // Print out the final newlines, flush, then unlock the log file
    fprintf(stderr, "\n\n");
    fprintf(mprintf_log_file, "\n\n");
    fflush(stderr);
    fflush(mprintf_log_file);
    fractal_unlock_mutex((FractalMutex)log_file_mutex);

    fractal_unlock_mutex(crash_handler_mutex);
}

#ifdef _WIN32
LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS* ExceptionInfo) {  // NOLINT
    fprintf(stderr, "\n");
    switch (ExceptionInfo->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            fprintf(stderr, "Error: EXCEPTION_ACCESS_VIOLATION\n");
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            fprintf(stderr, "Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n");
            break;
        case EXCEPTION_BREAKPOINT:
            fprintf(stderr, "Error: EXCEPTION_BREAKPOINT\n");
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            fprintf(stderr, "Error: EXCEPTION_DATATYPE_MISALIGNMENT\n");
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            fprintf(stderr, "Error: EXCEPTION_FLT_DENORMAL_OPERAND\n");
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            fprintf(stderr, "Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n");
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            fprintf(stderr, "Error: EXCEPTION_FLT_INEXACT_RESULT\n");
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            fprintf(stderr, "Error: EXCEPTION_FLT_INVALID_OPERATION\n");
            break;
        case EXCEPTION_FLT_OVERFLOW:
            fprintf(stderr, "Error: EXCEPTION_FLT_OVERFLOW\n");
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            fprintf(stderr, "Error: EXCEPTION_FLT_STACK_CHECK\n");
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            fprintf(stderr, "Error: EXCEPTION_FLT_UNDERFLOW\n");
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            fprintf(stderr, "Error: EXCEPTION_ILLEGAL_INSTRUCTION\n");
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            fprintf(stderr, "Error: EXCEPTION_IN_PAGE_ERROR\n");
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            fprintf(stderr, "Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n");
            break;
        case EXCEPTION_INT_OVERFLOW:
            fprintf(stderr, "Error: EXCEPTION_INT_OVERFLOW\n");
            break;
        case EXCEPTION_INVALID_DISPOSITION:
            fprintf(stderr, "Error: EXCEPTION_INVALID_DISPOSITION\n");
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            fprintf(stderr, "Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n");
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            fprintf(stderr, "Error: EXCEPTION_PRIV_INSTRUCTION\n");
            break;
        case EXCEPTION_SINGLE_STEP:
            fprintf(stderr, "Error: EXCEPTION_SINGLE_STEP\n");
            break;
        case EXCEPTION_STACK_OVERFLOW:
            fprintf(stderr, "Error: EXCEPTION_STACK_OVERFLOW\n");
            break;
        default:
            fprintf(stderr, "Error: Unrecognized Exception\n");
            break;
    }
    /* If this is a stack overflow then we can't walk the stack, so just show
      where the error happened */
    if (EXCEPTION_STACK_OVERFLOW != ExceptionInfo->ExceptionRecord->ExceptionCode) {
        print_stacktrace();
    } else {
        fprintf(stderr, "Can't show stacktrace when the stack has overflowed!\n");
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void unix_crash_handler(int sig) {
    fprintf(stderr, "\nError: signal %d:%s\n", sig, strsignal(sig));
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
        // We do nothing on SIGCHLD
        // We ignore SIGPIPE
        // We crash on anything else
        if (i != SIGTERM && i != SIGCHLD && i != SIGPIPE) {
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

char* get_version() {
    static char* version = NULL;

    if (version) {
        return version;
    }

#ifdef _WIN32
    char* version_filepath = "C:\\Program Files\\Fractal\\version";
#else
    char* version_filepath = "./version";
#endif

    size_t length;
    FILE* f = fopen(version_filepath, "r");

    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        static char buf[200];
        version = buf;
        int bytes = (int)fread(version, 1, min(length, (long)sizeof(buf) - 1),
                               f);  // cast for compiler warning
        for (int i = 0; i < bytes; i++) {
            if (version[i] == '\n') {
                version[i] = '\0';
                break;
            }
        }
        version[bytes] = '\0';
        fclose(f);
    } else {
        version = "NONE";
    }

    return version;
}

void save_connection_id(int connection_id_int) {
    char connection_id_filename[1000] = "";
    strcat(connection_id_filename, log_directory);
    strcat(connection_id_filename, "connection_id.txt");
    FILE* connection_id_file = fopen(connection_id_filename, "wb");
    fprintf(connection_id_file, "%d", connection_id_int);
    fclose(connection_id_file);

    // send connection id to sentry as a tag, client also does this
    if (using_sentry) {
        char str_connection_id[100];
        sprintf(str_connection_id, "%d", connection_id_int);
        sentry_set_tag("connection_id", str_connection_id);
    }
}

typedef struct UpdateStatusData {
    bool is_connected;
    char* host;
    char* identifier;
    char* hex_aes_private_key;
} UpdateStatusData;

int32_t multithreaded_update_server_status(void* data) {
    UpdateStatusData* d = data;

    char json[1000];
    snprintf(json, sizeof(json),
             "{\n"
             "  \"available\" : %s,\n"
             "  \"identifier\" : %s,\n"
             "  \"private_key\" : \"%s\"\n"
             "}",
             d->is_connected ? "false" : "true", d->identifier, d->hex_aes_private_key);
    send_post_request(d->host, "/container/ping", json, NULL, 0);

    free(d);
    return 0;
}

void update_server_status(bool is_connected, char* host, char* identifier,
                          char* hex_aes_private_key) {
    LOG_INFO("Update Status: %s", is_connected ? "Connected" : "Disconnected");
    UpdateStatusData* d = safe_malloc(sizeof(UpdateStatusData));
    d->is_connected = is_connected;
    d->host = host;
    d->identifier = identifier;
    d->hex_aes_private_key = hex_aes_private_key;
    FractalThread update_status =
        fractal_create_thread(multithreaded_update_server_status, "update_server_status", d);
    fractal_detach_thread(update_status);
}
