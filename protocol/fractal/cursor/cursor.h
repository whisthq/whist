/*
 * General cursor file.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef CURSOR_H
#define CURSOR_H

#ifdef _WIN32
#include "windowscursor.h"
#else
#include "linuxcursor.h"
#endif

void InitCursors();

FractalCursorImage GetCurrentCursor();

#endif  // CURSOR_H