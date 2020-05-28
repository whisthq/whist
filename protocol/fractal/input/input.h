#ifndef INPUT_H
#define INPUT_H
/**
Copyright Fractal Computers, Inc. 2020
@file input.h
@brief 

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
@brief                          Initialize all cursors

@returns                        Initialized input device struct defining mouse and keyboard states
*/
input_device_t* CreateInputDevice();

/**
@brief                          Returns the current cursor image

@param input_device             The initialized input device struct to destroy and free the memory of
*/
void DestroyInputDevice(input_device_t* input_device);

/**
@brief                          Returns the current cursor image

@param input_device             The initialized input device struct defining mouse and keyboard states for the user
@param fmsg                     The Fractal message packet, defining one user action event, to replay on the computer

@returns                        True if it replayed the event, False otherwise
*/
bool ReplayUserInput(input_device_t* input_device,
                     struct FractalClientMessage* fmsg);

/**
@brief                          Returns the current cursor image

@param input_device             The initialized input device struct defining mouse and keyboard states for the user
@param fmsg                     The Fractal message packet, defining one keyboard event, to update the keyboard state
*/
void UpdateKeyboardState(input_device_t* input_device,
                         struct FractalClientMessage* fmsg);

#endif  // INPUT_H
