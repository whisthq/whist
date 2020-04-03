#ifndef X11_INPUT_H
#define X11_INPUT_H

#include "fractal.h"

// void updateKeyboardState( struct FractalClientMessage* fmsg );

void ReplayUserInput(struct FractalClientMessage* fmsg);

#endif  // X11_INPUT_H