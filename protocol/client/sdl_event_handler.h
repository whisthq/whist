#ifndef SDL_EVENT_HANDLER_H
#define SDL_EVENT_HANDLER_H
/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file sdl_event_handler.h
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

handleSDLEvent() must be called on any SDL event that occurs. Any action
trigged an SDL event must be triggered in sdl_event_handler.c
*/

/*
============================
Includes
============================
*/
#include <whist/core/whist.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Flushes the SDL event queue,
 *                                 and handles all of the events
 *
 * @returns                        True on success,
 *                                 False on failure
 *
 * @note                           This function call MAY occasionally hang for an indefinitely long
 * time. To see an example of this, simply drag the window on MSWindows, or hold down the minimize
 * button on Mac. To see more information on this issue, and my related rant, go to
 * https://github.com/libsdl-org/SDL/issues/1059
 */
bool sdl_handle_events();

#endif  // SDL_EVENT_HANDLER_H
