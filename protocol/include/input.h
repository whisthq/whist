#ifndef INPUT_H
#define INPUT_H

#ifdef _WIN32
#include "windowsinput.h"
#else
#include "linuxinput.h"
#endif

input_device_t* CreateInputDevice(input_device_t* input_device);

void DestroyInputDevice(input_device_t* input_device);

void ReplayUserInput(input_device_t* input_device,
                     struct FractalClientMessage* fmsg);

void UpdateKeyboardState(input_device_t* input_device,
                         struct FractalClientMessage* fmsg);

#endif  // INPUT_H