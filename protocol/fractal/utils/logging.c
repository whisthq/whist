#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "logging.h"

// Multithreaded printf Semaphores and Mutexes
static volatile SDL_sem *multithreadedprintf_semaphore;
static volatile SDL_mutex *multithreadedprintf_mutex;

// Multithreaded printf queue

typedef struct mprintf_queue_item {
    bool log;
    char buf[MPRINTF_BUF_SIZE];
} mprintf_queue_item;
static volatile mprintf_queue_item mprintf_queue[MPRINTF_QUEUE_SIZE];
static volatile mprintf_queue_item mprintf_queue_cache[MPRINTF_QUEUE_SIZE];
static volatile int mprintf_queue_index = 0;
static volatile int mprintf_queue_size = 0;

// Multithreaded printf global variables
SDL_Thread *mprintf_thread = NULL;
static volatile bool run_multithreaded_printf;
int MultiThreadedPrintf(void *opaque);
void real_mprintf(bool log, const char *fmtStr, va_list args);
clock mprintf_timer;
FILE *mprintf_log_file = NULL;
char *log_directory = NULL;

char mprintf_history[1000000];
int mprintf_history_len;

char *get_mprintf_history() { return mprintf_history; }
int get_mprintf_history_len() { return mprintf_history_len; }

void initMultiThreadedPrintf(char *log_dir) {
    mprintf_history_len = 0;

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
    multithreadedprintf_mutex = SDL_CreateMutex();
    multithreadedprintf_semaphore = SDL_CreateSemaphore(0);
    mprintf_thread = SDL_CreateThread((SDL_ThreadFunction)MultiThreadedPrintf,
                                      "MultiThreadedPrintf", NULL);
    StartTimer(&mprintf_timer);
}

void destroyMultiThreadedPrintf() {
    // Wait for any remaining printfs to execute
    SDL_Delay(50);

    run_multithreaded_printf = false;
    SDL_SemPost((SDL_sem *)multithreadedprintf_semaphore);

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
        SDL_SemWait((SDL_sem *)multithreadedprintf_semaphore);

        if (!run_multithreaded_printf) {
            break;
        }

        int cache_size = 0;

        // Clear the queue into the cache,
        // And then let go of the mutex so that printf can continue accumulating
        SDL_LockMutex((SDL_mutex *)multithreadedprintf_mutex);
        cache_size = mprintf_queue_size;
        for (int i = 0; i < mprintf_queue_size; i++) {
            mprintf_queue_cache[i].log = mprintf_queue[mprintf_queue_index].log;
            strcpy((char *)mprintf_queue_cache[i].buf,
                   (const char *)mprintf_queue[mprintf_queue_index].buf);
            mprintf_queue_index++;
            mprintf_queue_index %= MPRINTF_QUEUE_SIZE;
            if (i != 0) {
                SDL_SemWait((SDL_sem *)multithreadedprintf_semaphore);
            }
        }
        mprintf_queue_size = 0;
        SDL_UnlockMutex((SDL_mutex *)multithreadedprintf_mutex);

        // Print all of the data into the cache
        // int last_printf = -1;
        for (int i = 0; i < cache_size; i++) {
            if (mprintf_log_file && mprintf_queue_cache[i].log) {
                fprintf(mprintf_log_file, "%s", mprintf_queue_cache[i].buf);
            }
            // if (i + 6 < cache_size) {
            //	printf("%s%s%s%s%s%s%s", mprintf_queue_cache[i].buf,
            // mprintf_queue_cache[i+1].buf, mprintf_queue_cache[i+2].buf,
            // mprintf_queue_cache[i+3].buf, mprintf_queue_cache[i+4].buf,
            // mprintf_queue_cache[i+5].buf,  mprintf_queue_cache[i+6].buf);
            //    last_printf = i + 6;
            //} else if (i > last_printf) {
            printf("%s", mprintf_queue_cache[i].buf);
            int chars_written = sprintf(&mprintf_history[mprintf_history_len],
                                        "%s", mprintf_queue_cache[i].buf);
            mprintf_history_len += chars_written;

            // Shift buffer over if too large;
            if ((unsigned long)mprintf_history_len >
                sizeof(mprintf_history) - sizeof(mprintf_queue_cache[i].buf) -
                    10) {
                int new_len = sizeof(mprintf_history) / 3;
                for (i = 0; i < new_len; i++) {
                    mprintf_history[i] =
                        mprintf_history[mprintf_history_len - new_len + i];
                }
                mprintf_history_len = new_len;
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

void lprintf(const char *fmtStr, ...) {
    va_list args;
    va_start(args, fmtStr);
    real_mprintf(true, fmtStr, args);
}

void real_mprintf(bool log, const char *fmtStr, va_list args) {
    if (mprintf_thread == NULL) {
        printf("initMultiThreadedPrintf has not been called!\n");
        return;
    }

    SDL_LockMutex((SDL_mutex *)multithreadedprintf_mutex);
    int index = (mprintf_queue_index + mprintf_queue_size) % MPRINTF_QUEUE_SIZE;
    char *buf = NULL;
    if (mprintf_queue_size < MPRINTF_QUEUE_SIZE - 2) {
        mprintf_queue[index].log = log;
        buf = (char *)mprintf_queue[index].buf;
        snprintf(buf, MPRINTF_BUF_SIZE, "%15.4f: ", GetTimer(mprintf_timer));
        int len = (int)strlen(buf);
        vsnprintf(buf + len, MPRINTF_BUF_SIZE - len, fmtStr, args);
        mprintf_queue_size++;
    } else if (mprintf_queue_size == MPRINTF_QUEUE_SIZE - 2) {
        mprintf_queue[index].log = log;
        buf = (char *)mprintf_queue[index].buf;
        strcpy(buf, "Buffer maxed out!!!\n");
        mprintf_queue_size++;
    }

    if (buf != NULL) {
        buf[MPRINTF_BUF_SIZE - 5] = '.';
        buf[MPRINTF_BUF_SIZE - 4] = '.';
        buf[MPRINTF_BUF_SIZE - 3] = '.';
        buf[MPRINTF_BUF_SIZE - 2] = '\n';
        buf[MPRINTF_BUF_SIZE - 1] = '\0';
        SDL_SemPost((SDL_sem *)multithreadedprintf_semaphore);
    }
    SDL_UnlockMutex((SDL_mutex *)multithreadedprintf_mutex);

    va_end(args);
}

#if defined(_WIN32)
LARGE_INTEGER frequency;
bool set_frequency = false;
#endif

void StartTimer(clock *timer) {
#if defined(_WIN32)
    if (!set_frequency) {
        QueryPerformanceFrequency(&frequency);
        set_frequency = true;
    }
    QueryPerformanceCounter(timer);
#else
    // start timer
    gettimeofday(timer, NULL);
#endif
}

double GetTimer(clock timer) {
#if defined(_WIN32)
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    double ret = (double)(end.QuadPart - timer.QuadPart) / frequency.QuadPart;
#else
    // stop timer
    struct timeval t2;
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    double elapsedTime = (t2.tv_sec - timer.tv_sec) * 1000.0;  // sec to ms
    elapsedTime += (t2.tv_usec - timer.tv_usec) / 1000.0;      // us to ms

    // printf("elapsed time in ms is: %f\n", elapsedTime);

    // standard var to return and convert to seconds since it gets converted to
    // ms in function call
    double ret = elapsedTime / 1000.0;
#endif
    return ret;
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