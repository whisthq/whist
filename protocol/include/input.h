#ifndef INPUT_H
#define INPUT_H

#include "fractal.h"

int GetWindowsKeyCode(int sdl_keycode);

void updateKeyboardState( struct FractalClientMessage* fmsg );

void ReplayUserInput(struct FractalClientMessage* fmsg);

void EnterWinString(enum FractalKeycode *keycodes, int len);

void initCursors();

FractalCursorImage GetCurrentCursor();

#endif // INPUT_H