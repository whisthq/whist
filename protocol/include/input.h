#ifndef INPUT_H
#define INPUT_H

#include "fractal.h"

void updateKeyboardState( struct FractalClientMessage* fmsg );

void ReplayUserInput(struct FractalClientMessage* fmsg);

void initCursors();

FractalCursorImage GetCurrentCursor();

#if defined(_WIN32)
void EnterWinString( enum FractalKeycode* keycodes, int len );
#endif

#endif // INPUT_H