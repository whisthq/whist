#ifndef CURSOR_H
#define CURSOR_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file cursor.h
 * @brief This file defines the cursor types, functions, init and get.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include <stdbool.h>

#include "../../include/SDL2/SDL.h"

/*
============================
Defines
============================
*/

#define MAX_CURSOR_WIDTH 32
#define MAX_CURSOR_HEIGHT 32

/*
============================
Custom Types
============================
*/

typedef enum FractalCursorState {
    CURSOR_STATE_HIDDEN = 0,
    CURSOR_STATE_VISIBLE = 1
} FractalCursorState;

typedef struct FractalCursorImage {
    SDL_SystemCursor cursor_id;
    FractalCursorState cursor_state;
    bool cursor_use_bmp;
    unsigned short cursor_bmp_width;
    unsigned short cursor_bmp_height;
    unsigned short cursor_bmp_hot_x;
    unsigned short cursor_bmp_hot_y;
    uint32_t cursor_bmp[MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT];
} FractalCursorImage;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize all cursors
 */
void InitCursors();

/**
 * @brief                          Returns the current cursor image
 *
 * @returns                        Current FractalCursorImage
 */
FractalCursorImage GetCurrentCursor();

int InitPeerCursors(void);

int DestroyPeerCursors(void);

int drawPeerCursor(SDL_Renderer *renderer, int x, int y, int r, int g, int b);

#endif  // CURSOR_H
