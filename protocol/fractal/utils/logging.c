#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

#ifdef _WIN32
#define _NO_CVCONST_H
#include <Windows.h>
#include <process.h>
#include <DbgHelp.h>
#else
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
    va_end(args);
}

void real_mprintf(bool log, const char *fmtStr, va_list args) {
    if (mprintf_thread == NULL) {
        printf("initLogger has not been called! Printing below...\n");
        vprintf(fmtStr, args);
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
}

SDL_mutex* crash_handler_mutex;

void PrintStacktrace()
{
    SDL_LockMutex( crash_handler_mutex );

#ifdef _WIN32
    unsigned int   i;
    void* stack[100];
    unsigned short frames;
    char buf[sizeof( SYMBOL_INFO ) + 256*sizeof( char )];
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)buf;
    HANDLE         process;

    process = GetCurrentProcess();

    SymInitialize( process, NULL, TRUE );

    frames = CaptureStackBackTrace( 0, 100, stack, NULL );
    memset( symbol, 0, sizeof( buf ) );
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

    for( i = 0; i < frames; i++ )
    {
        SymFromAddr( process, (DWORD64)(stack[i]), 0, symbol );

        fprintf( stderr, "%i: %s - 0x%0llx\n", frames - i - 1, symbol->Name, symbol->Address );
    }
#else
#define HANDLER_ARRAY_SIZE 100

    void* array[HANDLER_ARRAY_SIZE];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace( array, HANDLER_ARRAY_SIZE );

    // print out all the frames to stderr
    backtrace_symbols_fd( array, size, STDERR_FILENO );

    // and to the log
    int fd = fileno( mprintf_log_file );
    backtrace_symbols_fd( array, size, fd );

    fprintf( stderr, "addr2line -e build64/FractalServer" );
    for( size_t i = 0; i < size; i++ )
    {
        fprintf( stderr, " %p", array[i] );
    }
    fprintf( stderr, "\n\n" );
#endif

    SDL_UnlockMutex( crash_handler_mutex );
}

#ifdef _WIN32
LONG WINAPI windows_exception_handler( EXCEPTION_POINTERS* ExceptionInfo )
{
    SDL_Delay( 250 );
    fprintf( stderr, "\n" );
    switch( ExceptionInfo->ExceptionRecord->ExceptionCode )
    {
    case EXCEPTION_ACCESS_VIOLATION:
        fprintf( stderr, "Error: EXCEPTION_ACCESS_VIOLATION\n" );
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        fprintf( stderr, "Error: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n" );
        break;
    case EXCEPTION_BREAKPOINT:
        fprintf( stderr, "Error: EXCEPTION_BREAKPOINT\n" );
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        fprintf( stderr, "Error: EXCEPTION_DATATYPE_MISALIGNMENT\n" );
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        fprintf( stderr, "Error: EXCEPTION_FLT_DENORMAL_OPERAND\n" );
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        fprintf( stderr, "Error: EXCEPTION_FLT_DIVIDE_BY_ZERO\n" );
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        fprintf( stderr, "Error: EXCEPTION_FLT_INEXACT_RESULT\n" );
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        fprintf( stderr, "Error: EXCEPTION_FLT_INVALID_OPERATION\n" );
        break;
    case EXCEPTION_FLT_OVERFLOW:
        fprintf( stderr, "Error: EXCEPTION_FLT_OVERFLOW\n" );
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        fprintf( stderr, "Error: EXCEPTION_FLT_STACK_CHECK\n" );
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        fprintf( stderr, "Error: EXCEPTION_FLT_UNDERFLOW\n" );
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        fprintf( stderr, "Error: EXCEPTION_ILLEGAL_INSTRUCTION\n" );
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        fprintf( stderr, "Error: EXCEPTION_IN_PAGE_ERROR\n" );
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        fprintf( stderr, "Error: EXCEPTION_INT_DIVIDE_BY_ZERO\n" );
        break;
    case EXCEPTION_INT_OVERFLOW:
        fprintf( stderr, "Error: EXCEPTION_INT_OVERFLOW\n" );
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        fprintf( stderr, "Error: EXCEPTION_INVALID_DISPOSITION\n" );
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        fprintf( stderr, "Error: EXCEPTION_NONCONTINUABLE_EXCEPTION\n" );
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        fprintf( stderr, "Error: EXCEPTION_PRIV_INSTRUCTION\n" );
        break;
    case EXCEPTION_SINGLE_STEP:
        fprintf( stderr, "Error: EXCEPTION_SINGLE_STEP\n" );
        break;
    case EXCEPTION_STACK_OVERFLOW:
        fprintf( stderr, "Error: EXCEPTION_STACK_OVERFLOW\n" );
        break;
    default:
        fprintf( stderr, "Error: Unrecognized Exception\n" );
        break;
    }
    fflush( stderr );
    /* If this is a stack overflow then we can't walk the stack, so just show
      where the error happened */
    if( EXCEPTION_STACK_OVERFLOW != ExceptionInfo->ExceptionRecord->ExceptionCode )
    {
        PrintStacktrace();
    } else
    {
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void crash_handler(int sig) {
    fprintf( stderr, "\nError: signal %d:\n", sig );
    PrintStacktrace();
    SDL_Delay(100);
    exit(-1);
}
#endif

void initBacktraceHandler() {
    crash_handler_mutex = SDL_CreateMutex();
#ifdef _WIN32
    SetUnhandledExceptionFilter( windows_exception_handler );
#else
    signal(SIGSEGV, crash_handler);
#endif
}

char *get_version() {
    static char *version = NULL;

    if (version) {
        return version;
    }

#ifdef _WIN32
    char *version_filepath = "C:\\Program Files\\Fractal\\version";
#else
    char *version_filepath = "./version";
#endif

    size_t length;
    FILE *f = fopen(version_filepath, "r");

    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        static char buf[17];
        version = buf;
        fread(version, 1, min(length, (long)sizeof(buf)),
              f);  // cast for compiler warning
        version[16] = '\0';
        fclose(f);
    } else {
        version = "NONE";
    }

    return version;
}

bool sendLogHistory() {
    char *host = is_dev_vm() ? STAGING_HOST : PRODUCTION_HOST;
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
            \"version\" : \"%s\",\
            \"logs\" : \"%s\",\
            \"sender\" : \"server\"\
    }",
            connection_id, get_version(), logs);

    LOG_INFO("Sending logs to webserver...");
    SendJSONPost(host, path, json);
    free(logs);
    free(json);

    return true;
}

typedef struct update_status_data {
    bool is_connected;
} update_status_data_t;

int32_t MultithreadedUpdateStatus(void *data) {
    update_status_data_t *d = data;

    char json[1000];

    snprintf(json, sizeof(json),
             "{\
            \"ready\" : true\
    }");

    char *host = is_dev_vm() ? STAGING_HOST : PRODUCTION_HOST;

    SendJSONPost(host, "/vm/winlogonStatus", json);

    snprintf(json, sizeof(json),
             "{\
            \"version\" : \"%s\",\
            \"available\" : %s\
    }",
             get_version(), d->is_connected ? "false" : "true");
    SendJSONPost(host, "/vm/connectionStatus", json);

    free(d);
    return 0;
}

void updateStatus(bool is_connected) {
    update_status_data_t *d = malloc(sizeof(update_status_data_t));
    d->is_connected = is_connected;
    SDL_Thread *update_status =
        SDL_CreateThread(MultithreadedUpdateStatus, "UpdateStatus", d);
    SDL_DetachThread(update_status);
}
