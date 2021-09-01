#ifndef INPUT_H
#define INPUT_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file input.h
 * @brief This file defines general input-processing functions and toggles
 *        between Windows and Linux servers.
============================
Usage
============================

Toggles dynamically between input receiving on Windows or Linux Ubuntu
computers. You can create an input device to receive input (keystrokes, mouse
clicks, etc.) via CreateInputDevice. You can then send input to the Windows OS
via ReplayUserInput, and use UpdateKeyboardState to sync keyboard state between
local and remote computers (say, sync them to both have CapsLock activated).
Lastly, you can input an array of keycodes using `InputKeycodes`.
*/

/*
============================
Includes
============================
*/

#include "input_driver.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the input system. Should be used prior to every new
 *                                 client connection that will send inputs
 *
 * @param os_type                  The OS type to use for keyboard mapping
 */
void reset_input(FractalOSType os_type);

/**
 * @brief                          Replayed a received user action on a server,
 *                                 by sending it to the OS
 *
 * @param input_device             The initialized input device struct defining
 *                                 mouse and keyboard states for the user
 *
 * @param fmsg                     The Fractal message packet, defining one user
 *                                 action event, to replay on the computer
 *
 * @returns                        True if it replayed the event, False
 *                                 otherwise
 */
bool replay_user_input(InputDevice* input_device, FractalClientMessage* fmsg);

/**
 * @brief                          Updates the keyboard state on the server to
 *                                 match the client's
 *
 * @param input_device             The initialized input device struct defining
 *                                 mouse and keyboard states for the user
 *
 * @param fmsg                     The Fractal message packet, defining one
 *                                 keyboard event, to update the keyboard state
 */
void update_keyboard_state(InputDevice* input_device, FractalClientMessage* fmsg);

/**
 * @brief                          Updates the keyboard state on the server to
 *                                 match the client's
 *
 * @param input_device             The initialized input device struct defining
 *                                 mouse and keyboard states for the user
 *
 * @param keycodes                 A pointer to an array of keycodes to sequentially
 *                                 input
 *
 * @param count                    The number of keycodes to read from `keycodes`
 *
 * @returns                        The number of keycodes successfully inputted from
 *                                 the array
 */
size_t input_keycodes(InputDevice* input_device, FractalKeycode* keycodes, size_t count);

#endif  // INPUT_H
