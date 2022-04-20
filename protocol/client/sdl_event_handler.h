#ifndef WHIST_EVENT_HANDLER_H
#define WHIST_EVENT_HANDLER_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file sdl_event_handler.h
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

handleSDLEvent() must be called on any SDL event that occurs. Any action
trigged an SDL event must be triggered in sdl_event_handler.c

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
bool sdl_handle_events(WhistFrontend* frontend);

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
bool sdl_pending_audio_device_update(void);

// (TODO) temporary solution
void set_is_dragging_file(bool is_dragging_file, int x, int y);

void sdl_begin_file_drag(char* filename_list);
void sdl_move_content_drag(int x, int y);
void sdl_complete_content_drag(void);
void sdl_cancel_content_drag(void);

#endif  // WHIST_EVENT_HANDLER_H
