/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file debug_console.h
 * @brief TODO
============================
Usage
============================

Call these functions from anywhere within client where they're
needed.
*/

#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

/*
============================
Includes
============================
*/

/*
============================
Defines
============================
*/

#ifndef NDEBUG
// make sure debug console is only enabled for DEBUG build
// with this macro, we remove codes at compile, to avoid competitors enable the function in a hacky
// way and get too much info of our product
#define USE_DEBUG_CONSOLE
#endif

typedef struct {
    int bitrate;
    int burst_bitrate;
    double audio_fec_ratio;
    double video_fec_ratio;
    int no_minimize;
    int verbose_log;
    int verbose_log_audio;  // you can add more log flags like this
    int simulate_freeze;    // this is an expample rather than a out-of-box option
} DebugConsoleOverrideValues;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Ger overrided values from debug console
 *
 * @returns                        A pointer to the struct of all override values
 */
DebugConsoleOverrideValues* get_debug_console_override_values(void);

/**
 * @brief                          Enable debug console on the specific UDP port
 *
 * @param port                     The UDP port to let debug_console listen on
 */
void enable_debug_console(int port);

/**
 * @brief                          Init the debug console, if enabled
 *
 * @note                           should be called after, enable_debug_console()
 *                                 debug_console can be only enabled for a debug build
 */
int init_debug_console(void);

#endif  // DEBUG_CONSOLE_H
