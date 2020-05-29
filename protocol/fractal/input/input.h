#ifndef INPUT_H
#define INPUT_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file input.h
 * @brief This file defines general input-processing functions and toggles between Windows and Linux servers.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#ifdef _WIN32
#include "windowsinput.h"
#else
#include "linuxinput.h"
#endif

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Create an input device struct to receive user actions to be replayed on a server
 *
 * @returns                        Initialized input device struct defining mouse and keyboard states
 */
input_device_t* CreateInputDevice();

/**
 * @brief                          Destroy and free the memory of an input device struct
 *
 * @param input_device             The initialized input device struct to destroy and free the memory of
 */
void DestroyInputDevice(input_device_t* input_device);

/**
 * @brief                          Replayed a received user action on a server, by sending it to the OS
 *
 * @param input_device             The initialized input device struct defining mouse and keyboard states for the user
 * @param fmsg                     The Fractal message packet, defining one user action event, to replay on the computer
 *
 * @returns                        True if it replayed the event, False otherwise
 */
bool ReplayUserInput(input_device_t* input_device,
                     struct FractalClientMessage* fmsg);

/**
 * @brief                          Updates the keyboard state on the server to match the client's
 *
 * @param input_device             The initialized input device struct defining mouse and keyboard states for the user
 * @param fmsg                     The Fractal message packet, defining one keyboard event, to update the keyboard state
 */
void UpdateKeyboardState(input_device_t* input_device,
                         struct FractalClientMessage* fmsg);

#endif  // INPUT_H
