#ifndef CURSOR_H
#define CURSOR_H

#ifdef _WIN32
#include "../include/windowscursor.h"
#else
#include "../include/linuxcursor.h"
#endif

void InitCursors();

FractalCursorImage GetCurrentCursor();

#endif  // CURSOR_H