/**
 * Copyright Fractal Computers, Inc. 2020
 * @file linuxcursor.c
 * @brief This file defines the cursor types, functions, init and get.
============================
Usage
============================

Use InitCursor to load the appropriate cursor images for a specific OS, and then
GetCurrentCursor to retrieve what the cursor shold be on the OS (drag-window,
arrow, etc.).
*/

/*
============================
Includes
============================
*/

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>
#include <fractal/logging/logging.h>
#include "cursor.h"
#include "string.h"


/*
 * The serial numbers for each cursor, found experiementally
 * Some cursors have alternative forms. These have the same function, but different design
 * These will be indiciated via an "ALT" suffix
 */

#define X11_ARROW 10
#define X11_ARROW_ALT 11
#define X11_IBEAM 12
#define X11_IBEAM_ALT 13
#define X11_HAND_POINT 14
#define X11_HAND_POINT_ALT 15
#define X11_SIZEWE 22
#define X11_SIZENS 19
#define X11_SIZENS_ALT 23
#define X11_SIZENWSE 21
#define NO_MATCH 0


static Display* disp;

/*
============================
Public Function Implementations
============================
*/

void init_cursors() {

    /*
        Initialize all cursors by creating the display
    */

    disp = XOpenDisplay(NULL);
}

FractalCursorID get_cursor_id(int cursor_serial) {
        FractalCursorID id;
        if(cursor_serial == X11_ARROW || cursor_serial == X11_ARROW_ALT) {
                id = FRACTAL_CURSOR_ARROW;
        }
        else if(cursor_serial == X11_IBEAM || cursor_serial == X11_IBEAM_ALT) {
                id = FRACTAL_CURSOR_IBEAM;
        }
        else if(cursor_serial == X11_HAND_POINT || cursor_serial == X11_HAND_POINT_ALT) {
                id = FRACTAL_CURSOR_HAND;
        }
        else if(cursor_serial == X11_SIZEWE) {
                id = FRACTAL_CURSOR_SIZEWE;
        }
        else if(cursor_serial == X11_SIZENS || cursor_serial == X11_SIZENS_ALT) {
                id = FRACTAL_CURSOR_SIZENS;
        }
        else if(cursor_serial == X11_SIZENWSE) {
                id = FRACTAL_CURSOR_SIZENWSE;
        }
        else {
                id = NO_MATCH;
        }

        return id;
}


void get_current_cursor(FractalCursorImage* image) {
    /*
        Returns the current cursor image

        Returns:
            (FractalCursorImage): Current FractalCursorImage
    */

    memset(image, 0, sizeof(FractalCursorImage));
    image->cursor_id = FRACTAL_CURSOR_ARROW;
    image->cursor_state = CURSOR_STATE_VISIBLE;

    if (disp) {
        //XLockDisplay(disp);
        XFixesCursorImage* ci = XFixesGetCursorImage(disp);
        //XUnlockDisplay(disp);

        if (ci->width > MAX_CURSOR_WIDTH || ci->height > MAX_CURSOR_HEIGHT) {
            LOG_WARNING(
                "fractal/cursor/linuxcursor.c::GetCurrentCursor(): cursor width or height exceeds "
                "maximum dimensions. Truncating cursor from %hu by %hu to %hu by %hu.",
                ci->width, ci->height, MAX_CURSOR_WIDTH, MAX_CURSOR_HEIGHT);
        }

        if(NO_MATCH == (image->cursor_id = get_cursor_id(ci->cursor_serial))) {
                image->using_bmp = true;
                image->bmp_width = min(MAX_CURSOR_WIDTH, ci->width);
                image->bmp_height = min(MAX_CURSOR_HEIGHT, ci->height);
                image->bmp_hot_x = ci->xhot;
                image->bmp_hot_y = ci->yhot;

                for (int k = 0; k < image->bmp_width * image->bmp_height; ++k) {
                // we need to do this in case ci->pixels uses 8 bytes per pixel
                        uint32_t argb = (uint32_t)ci->pixels[k];
                        image->bmp[k] = argb;
                }

        }

        XFree(ci);
    }
}