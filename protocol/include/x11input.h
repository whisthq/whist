#ifndef X11_INPUT_H
#define X11_INPUT_H

#include "fractal.h"

// void updateKeyboardState( struct FractalClientMessage* fmsg );

void ReplayUserInput(struct FractalClientMessage* fmsg);

void initInputPlayback();

void stopInputPlayback();

#endif  // X11_INPUT_H