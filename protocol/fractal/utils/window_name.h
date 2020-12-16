#ifndef WINDOW_NAME_H
#define WINDOW_NAME_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file window_name.h
 * @brief This file contains all the code for getting the name of a window.
============================
Usage
============================

TODO
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Defines
============================
*/

#define WINDOW_NAME_MAXLEN 64

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the window name listener.
 *
 */
void init_window_name_listener();

/**
 * @brief                          Get the name of the focused window.
 *
 * @param name                     Address to write name. Should have at least WINDOW_NAME_MAXLEN
 *                                 bytes available.
 *
 */
void get_window_name(char* name);

#endif  // WINDOW_NAME_H
