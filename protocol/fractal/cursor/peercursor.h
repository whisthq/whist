#ifndef PEERCURSOR_H
#define PEERCURSOR_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file peercursor.h
 * @brief This file contains code for rendering peer cursors.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "../../include/SDL2/SDL.h"

/*
============================
Public Functions
============================
*/

int InitPeerCursors(void);

int DestroyPeerCursors(void);

int drawPeerCursor(SDL_Renderer *renderer, int x, int y, int r, int g, int b);

#endif  // PEERCURSOR_H
