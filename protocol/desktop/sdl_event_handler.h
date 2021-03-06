#ifndef SDL_EVENT_HANDLER_H
#define SDL_EVENT_HANDLER_H
/**
 * Copyright Fractal Computers, Inc. 2020
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
#include "../fractal/core/fractal.h"

/*
============================
Public Functions
============================
*/
/**
 * @brief                          Pops latest client-side SDL event,
 *                                 if there is one, and handles the event, on
 *                                 the client-side.
 *
 * @returns                        Returns -1 if no SDL event is available
 *                                 or an error occurs, 0 on success
 */
int try_handle_sdl_event(void);

/**
 * @brief                          Handles a client-side SDL event, on the
 *                                 client-side.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int handle_sdl_event(SDL_Event *event);

#endif  // SDL_EVENT_HANDLER_H
