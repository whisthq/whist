/*
 * General input processing functions for Windows and Linux Ubuntu.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef INPUT_H
#define INPUT_H

#ifdef _WIN32
#include "windowsinput.h"
#else
#include "linuxinput.h"
#endif

input_device_t* CreateInputDevice();

void DestroyInputDevice(input_device_t* input_device);

bool ReplayUserInput(input_device_t* input_device,
                     struct FractalClientMessage* fmsg);

void UpdateKeyboardState(input_device_t* input_device,
                         struct FractalClientMessage* fmsg);

#endif  // INPUT_H
