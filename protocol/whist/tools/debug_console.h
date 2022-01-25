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
// with this macro, we remove codes at compile, to avoid competitors enable the function in a hacky
// way and get too much info of our product
#define USE_DEBUG_CONSOLE
#endif

typedef struct {
    int force_bitrate;
    int force_burst_bitrate;
    double force_audio_fec_ratio;
    double force_video_fec_ratio;
    int force_no_minimize;
    int verbose_log;
    int simulate_freeze;
} ForcedValues;

/*
============================
Public Functions
============================
*/
ForcedValues* get_forced_values(void);
void enable_debug_console(int port);
int init_debug_console(void);

#endif  // DEBUG_CONSOLE_H
