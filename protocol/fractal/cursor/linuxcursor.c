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
#include <fractal/logging/logging.h>
#include "cursor.h"
#include "string.h"

static Display* disp;

int get_cursor_id(const char* cursor_name) {
    /*
        Get the corresponding cursor image id for by cursor type

        Arguments:
            cursor_name (const char*): name of cursor icon defined in X11/cursorfont.h
            NOTE: The name is the macro name without the initial "XC_", so "XC_arrow"
            would be "arrow"

        Returns:
            (FractalCursorImage): the corresponding FractalCursorImage id
                for the X11 cursor name
    */

    int fractal_cursor_id = 0;

    if (strcmp(cursor_name, "left_ptr") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_ARROW;
    } else if (strcmp(cursor_name, "crosshair") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_CROSSHAIR;
    } else if (strcmp(cursor_name, "hand1") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_HAND;
    } else if (strcmp(cursor_name, "xterm") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_IBEAM;
    } else if (strcmp(cursor_name, "x_cursor") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_NO;
    } else if (strcmp(cursor_name, "fleur") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_SIZEALL;
    } else if (strcmp(cursor_name, "bottom_left_corner") == 0 || strcmp(cursor_name, "top_right_corner") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_SIZENESW;
    } else if (strcmp(cursor_name, "sb_v_double_arrow") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_SIZENS;
    } else if (strcmp(cursor_name, "sizing") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_SIZENWSE;
    } else if (strcmp(cursor_name, "sb_h_double_arrow") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_SIZEWE;
    } else if (strcmp(cursor_name, "watch") == 0) {
        fractal_cursor_id = FRACTAL_CURSOR_WAITARROW;
    } else {
        fractal_cursor_id = FRACTAL_CURSOR_ARROW;
    }

    return fractal_cursor_id;
}



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
        XLockDisplay(disp);
        XFixesCursorImage* ci = XFixesGetCursorImage(disp);
        XUnlockDisplay(disp);

        if (ci->width > MAX_CURSOR_WIDTH || ci->height > MAX_CURSOR_HEIGHT) {
            LOG_WARNING(
                "fractal/cursor/linuxcursor.c::GetCurrentCursor(): cursor width or height exceeds "
                "maximum dimensions. Truncating cursor from %hu by %hu to %hu by %hu.",
                ci->width, ci->height, MAX_CURSOR_WIDTH, MAX_CURSOR_HEIGHT);
        }

        image->using_bmp = false;
        image->bmp_width = min(MAX_CURSOR_WIDTH, ci->width);
        image->bmp_height = min(MAX_CURSOR_HEIGHT, ci->height);
        image->bmp_hot_x = ci->xhot;
        image->bmp_hot_y = ci->yhot;
        image->cursor_id = get_cursor_id(ci->name);

        XFree(ci);
    }
}
