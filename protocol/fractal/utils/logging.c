#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

#ifndef _WIN32
#include <execinfo.h>
#include <signal.h>
#endif

#include "../core/fractal.h"
#include "../network/network.h"
#include "logging.h"

char *get_logger_history();
int get_logger_history_len();
void initBacktraceHandler();

extern int connection_id;

// logger Semaphores and Mutexes
static volatile SDL_sem *logger_semaphore;
static volatile SDL_mutex *logger_mutex;

// logger queue

typedef struct logger_queue_item {
    bool log;
    char buf[LOGGER_BUF_SIZE];
} logger_queue_item;
static volatile logger_queue_item logger_queue[LOGGER_QUEUE_SIZE];
static volatile logger_queue_item logger_queue_cache[LOGGER_QUEUE_SIZE];
static volatile int logger_queue_index = 0;
static volatile int logger_queue_size = 0;

// logger global variables
SDL_Thread *mprintf_thread = NULL;
static volatile bool run_multithreaded_printf;
int MultiThreadedPrintf(void *opaque);
void real_mprintf(bool log, const char *fmtStr, va_list args);
clock mprintf_timer;
FILE *mprintf_log_file = NULL;
char *log_directory = NULL;

char logger_history[1000000];
int logger_history_len;

char *get_logger_history() { return logger_history; }
int get_logger_history_len() { return logger_history_len; }

void initLogger(char *log_dir) {
    initBacktraceHandler();

    logger_history_len = 0;

    if (log_dir) {
        log_directory = log_dir;
        char f[1000] = "";
        strcat(f, log_directory);
        strcat(f, "/log.txt");
#if defined(_WIN32)
        CreateDirectoryA(log_directory, 0);
#else
        mkdir(log_directory, 0755);
#endif
        mprintf_log_file = fopen(f, "ab");
    }

    run_multithreaded_printf = true;
    logger_mutex = SDL_CreateMutex();
    logger_semaphore = SDL_CreateSemaphore(0);
    mprintf_thread = SDL_CreateThread((SDL_ThreadFunction)MultiThreadedPrintf,
                                      "MultiThreadedPrintf", NULL);
    //    StartTimer(&mprintf_timer);
}

void destroyLogger() {
    // Wait for any remaining printfs to execute
    SDL_Delay(50);

    run_multithreaded_printf = false;
    SDL_SemPost((SDL_sem *)logger_semaphore);

    SDL_WaitThread(mprintf_thread, NULL);
    mprintf_thread = NULL;

    if (mprintf_log_file) {
        fclose(mprintf_log_file);
    }
    mprintf_log_file = NULL;
}

int MultiThreadedPrintf(void *opaque) {
    opaque;

    while (true) {
        // Wait until signaled by printf to begin running
        SDL_SemWait((SDL_sem *)logger_semaphore);

        if (!run_multithreaded_printf) {
            break;
        }

        int cache_size = 0;

        // Clear the queue into the cache,
        // And then let go of the mutex so that printf can continue accumulating
        SDL_LockMutex((SDL_mutex *)logger_mutex);
        cache_size = logger_queue_size;
        for (int i = 0; i < logger_queue_size; i++) {
            logger_queue_cache[i].log = logger_queue[logger_queue_index].log;
            strcpy((char *)logger_queue_cache[i].buf,
                   (const char *)logger_queue[logger_queue_index].buf);
            logger_queue[logger_queue_index].buf[0] = '\0';
            logger_queue_index++;
            logger_queue_index %= LOGGER_QUEUE_SIZE;
            if (i != 0) {
                SDL_SemWait((SDL_sem *)logger_semaphore);
            }
        }
        logger_queue_size = 0;
        SDL_UnlockMutex((SDL_mutex *)logger_mutex);

        // Print all of the data into the cache
        // int last_printf = -1;
        for (int i = 0; i < cache_size; i++) {
            if (mprintf_log_file && logger_queue_cache[i].log) {
                fprintf(mprintf_log_file, "%s", logger_queue_cache[i].buf);
            }
            // if (i + 6 < cache_size) {
            //	printf("%s%s%s%s%s%s%s", logger_queue_cache[i].buf,
            // logger_queue_cache[i+1].buf, logger_queue_cache[i+2].buf,
            // logger_queue_cache[i+3].buf, logger_queue_cache[i+4].buf,
            // logger_queue_cache[i+5].buf,  logger_queue_cache[i+6].buf);
            //    last_printf = i + 6;
            //} else if (i > last_printf) {
            printf("%s", logger_queue_cache[i].buf);
            int chars_written = sprintf(&logger_history[logger_history_len],
                                        "%s", logger_queue_cache[i].buf);
            logger_history_len += chars_written;

            // Shift buffer over if too large;
            if ((unsigned long)logger_history_len >
                sizeof(logger_history) - sizeof(logger_queue_cache[i].buf) -
                    10) {
                int new_len = sizeof(logger_history) / 3;
                for (i = 0; i < new_len; i++) {
                    logger_history[i] =
                        logger_history[logger_history_len - new_len + i];
                }
                logger_history_len = new_len;
            }
            //}
        }
        if (mprintf_log_file) {
            fflush(mprintf_log_file);
        }

        // If the log file is large enough, cache it
        if (mprintf_log_file) {
            fseek(mprintf_log_file, 0L, SEEK_END);
            int sz = ftell(mprintf_log_file);

            // If it's larger than 5MB, start a new file and store the old one
            if (sz > 5 * 1024 * 1024) {
                fclose(mprintf_log_file);

                char f[1000] = "";
                strcat(f, log_directory);
                strcat(f, "/log.txt");
                char fp[1000] = "";
                strcat(fp, log_directory);
                strcat(fp, "/log_prev.txt");
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
    }
    return 0;
}

void mprintf(const char *fmtStr, ...) {
    va_list args;
    va_start(args, fmtStr);

    real_mprintf(WRITE_MPRINTF_TO_LOG, fmtStr, args);
}

void real_mprintf(bool log, const char *fmtStr, va_list args) {
    if (mprintf_thread == NULL) {
        printf("initLogger has not been called!\n");
        return;
    }

    SDL_LockMutex((SDL_mutex *)logger_mutex);
    int index = (logger_queue_index + logger_queue_size) % LOGGER_QUEUE_SIZE;
    char *buf = NULL;
    if (logger_queue_size < LOGGER_QUEUE_SIZE - 2) {
        logger_queue[index].log = log;
        buf = (char *)logger_queue[index].buf;
        //        snprintf(buf, LOGGER_BUF_SIZE, "%15.4f: ",
        //        GetTimer(mprintf_timer));
        if (buf[0] != '\0') {
            char old_msg[LOGGER_BUF_SIZE];
            memcpy(old_msg, buf, LOGGER_BUF_SIZE);
            int chars_written =
                snprintf(buf, LOGGER_BUF_SIZE,
                         "OLD MESSAGE: %s\nTRYING TO OVERWRITE WITH: %s\n",
                         old_msg, logger_queue[index].buf);
            if (!(chars_written > 0 && chars_written <= LOGGER_BUF_SIZE)) {
                buf[0] = '\0';
            }
        } else {
            vsnprintf(buf, LOGGER_BUF_SIZE, fmtStr, args);
        }
        logger_queue_size++;
    } else if (logger_queue_size == LOGGER_QUEUE_SIZE - 2) {
        logger_queue[index].log = log;
        buf = (char *)logger_queue[index].buf;
        strcpy(buf, "Buffer maxed out!!!\n");
        logger_queue_size++;
    }

    if (buf != NULL) {
        buf[LOGGER_BUF_SIZE - 5] = '.';
        buf[LOGGER_BUF_SIZE - 4] = '.';
        buf[LOGGER_BUF_SIZE - 3] = '.';
        buf[LOGGER_BUF_SIZE - 2] = '\n';
        buf[LOGGER_BUF_SIZE - 1] = '\0';
        SDL_SemPost((SDL_sem *)logger_semaphore);
    }
    SDL_UnlockMutex((SDL_mutex *)logger_mutex);

    va_end(args);
}

#ifndef _WIN32
SDL_mutex *crash_handler_mutex;

void crash_handler(int sig) {
    SDL_LockMutex(crash_handler_mutex);

#define HANDLER_ARRAY_SIZE 100

    void *array[HANDLER_ARRAY_SIZE];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, HANDLER_ARRAY_SIZE);

    // print out all the frames to stderr
    fprintf(stderr, "\nError: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    fprintf(stderr, "addr2line -e build64/server");
    for (size_t i = 0; i < size; i++) {
        fprintf(stderr, " %p", array[i]);
    }
    fprintf(stderr, "\n\n");

    SDL_UnlockMutex(crash_handler_mutex);

    SDL_Delay(100);
    exit(-1);
}
#endif

void initBacktraceHandler() {
#ifndef _WIN32
    crash_handler_mutex = SDL_CreateMutex();
    signal(SIGSEGV, crash_handler);
#endif
}

bool sendLogHistory() {
    char *host = "cube-celery-staging.herokuapp.com";
    char *path = "/logs";

    char *logs_raw = get_logger_history();
    int raw_log_len = get_logger_history_len();

    char *logs = malloc(1000 + 2 * raw_log_len);
    int log_len = 0;
    for (int i = 0; i < raw_log_len; i++) {
        switch (logs_raw[i]) {
            case '\b':
                logs[log_len++] = '\\';
                logs[log_len++] = 'b';
                break;
            case '\f':
                logs[log_len++] = '\\';
                logs[log_len++] = 'f';
                break;
            case '\n':
                logs[log_len++] = '\\';
                logs[log_len++] = 'n';
                break;
            case '\r':
                logs[log_len++] = '\\';
                logs[log_len++] = 'r';
                break;
            case '\t':
                logs[log_len++] = '\\';
                logs[log_len++] = 't';
                break;
            case '"':
                logs[log_len++] = '\\';
                logs[log_len++] = '"';
                break;
            case '\\':
                logs[log_len++] = '\\';
                logs[log_len++] = '\\';
                break;
            default:
                logs[log_len++] = logs_raw[i];
                break;
        }
    }

    logs[log_len++] = '\0';

    char *json = malloc(1000 + log_len);
    sprintf(json,
            "{\
            \"connection_id\" : \"%d\",\
            \"logs\" : \"%s\",\
            \"sender\" : \"server\"\
    }",
            connection_id, logs);
    SendJSONPost(host, path, json);
    free(logs);
    free(json);

    return true;
}
