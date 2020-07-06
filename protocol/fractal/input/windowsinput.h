#ifndef WINDOWS_INPUT_H
#define WINDOWS_INPUT_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file windowsinput.h
 * @brief This file defines the Windows input device type and function to enter
 *        string on Winlogon screen.
============================
Usage
============================

Use EnterWinString to enter a password to Windows Winlogon screen, to log a user
into the Windows desktop environment. You can create an input device to receive
input (keystrokes, mouse clicks, etc.) via CreateInputDevice (defined in input.h).
You can then send input to the Windows OS via ReplayUserInput, and use 
UpdateKeyboardState to sync keyboard state between local and remote computers 
(say, sync them to both have CapsLock activated).
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

typedef char input_device_t;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Inputs a provided series of character to
 *                                 Winlogon, to log in to the computer
 *
 * @param keycodes                 Enumeration of every character to enter,
 *                                 sequentially
 * @param len                      Number of characters to enter
 */
void EnterWinString(FractalKeycode* keycodes, int len);

#endif  // WINDOWS_INPUT_H
