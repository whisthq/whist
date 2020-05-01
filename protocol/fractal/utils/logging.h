#ifndef LOGGING_H
#define LOGGING_H

// *** BEGIN INCLUDES ***

#include <stdio.h>

#ifndef _WIN32
#include <execinfo.h>
#include <signal.h>
#endif

#include "../core/fractal.h"
#include "../network/network.h"

// *** END INCLUDES ***

// *** BEGIN DEFINES ***
#define MPRINTF_QUEUE_SIZE 1000
#define MPRINTF_BUF_SIZE 1000

#if defined(_WIN32)
#define clock LARGE_INTEGER
#else
#define clock struct timeval
#endif

// *** END DEFINES ***

// *** BEGIN FUNCTIONS ***

void initMultiThreadedPrintf(char* log_directory);
void destroyMultiThreadedPrintf();
void mprintf(const char* fmtStr, ...);
void lprintf(const char* fmtStr, ...);

void StartTimer(clock* timer);
double GetTimer(clock timer);

void initBacktraceHandler();

char* get_mprintf_history();
int get_mprintf_history_len();

bool sendLog();
// *** END FUNCTIONS ***

#endif  // LOGGING_H