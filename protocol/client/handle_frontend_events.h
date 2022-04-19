#ifndef WHIST_CLIENT_HANDLE_FRONTEND_EVENTS_H
#define WHIST_CLIENT_HANDLE_FRONTEND_EVENTS_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file handle_frontend_events.c
 * @brief This file exposes the functions to handle frontend events in the main loop.
============================
Usage
============================

handle_frontend_events() should be periodically called in the main loop to poll and handle
frontend events, including input, mouse motion, and resize events.

TODO: Combine this with sdl_utils.h in some way
*/

/*
============================
Includes
============================
*/
#include <whist/core/whist.h>
#include "frontend/frontend.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Flushes the SDL event queue, and
 *                                 handles all of the pending events.
 *                                 Calling this function repeatedly is necessary,
 *                                 otherwise "Application not responding" or a Mac beachball
 *                                 may appear.
 *
 * @returns                        True on success,
 *                                 False on failure
 *
 * @note                           This function call MAY occasionally hang for an indefinitely long
 *                                 time. To see an example of this, simply drag the window on
 *                                 MSWindows, or hold down the minimize button on Mac.
 *                                 To see more information on this issue, and my related rant,
 *                                 go to https://github.com/libsdl-org/SDL/issues/1059
 */
bool handle_frontend_events(WhistFrontend* frontend);

/**
 * @brief                          The function will let you know if an audio device has
 *                                 been plugged in or unplugged since the last time you
 *                                 called this function.
 *
 * @returns                        True if there's been an audio device update since
 *                                 the last time you called this function,
 *                                 False otherwise.
 *
 * @note                           This function is thread-safe with any other sdl function call.
 */
bool pending_audio_device_update(void);

#endif  // WHIST_CLIENT_HANDLE_FRONTEND_EVENTS_H
