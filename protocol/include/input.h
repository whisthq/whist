#ifndef INPUT_H
#define INPUT_H

#ifdef _WIN32
#include "../include/windowsinput.h"
#else
#include "../include/linuxinput.h"
#endif

input_device_t* CreateInputDevice(input_device_t* input_device);

void DestroyInputDevice(input_device_t* input_device);

void ReplayUserInput(input_device_t* input_device,
                     struct FractalClientMessage* fmsg);

void UpdateKeyboardState(input_device_t* input_device,
                         struct FractalClientMessage* fmsg);

void InitCursors();

FractalCursorImage GetCurrentCursor();

#endif  // INPUT_H