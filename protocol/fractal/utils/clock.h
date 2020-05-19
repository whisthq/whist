#ifndef CLOCK_H
#define CLOCK_H

/*
============================
Includes
============================
*/

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

/*
============================
Defines
============================
*/

#if defined(_WIN32)
#define clock LARGE_INTEGER
#else
#define clock struct timeval
#endif

/*
============================
Public Functions
============================
*/

/*
@brief                          Start the given timer at the current time, as a
stopwatch

@param timer		            Pointer to the the timer in question
*/
void StartTimer(clock* timer);

/*
@brief                          Get the amount of elapsed time since the last
StartTimer

@param timer		            The timer in question
*/
double GetTimer(clock timer);

/*
@brief                          Create a clock that represents the given timeout
in milliseconds

@param timeout_ms	            The number of milliseconds for the clock

@returns						The desired clock
*/
clock CreateClock(int timeout_ms);

/*
@brief                          Returns the current time as a string

@returns						The current time as a
string
*/
char* CurrentTimeStr();

#endif
