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

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <utils/logging.h>

#include "cursor.h"
#include "string.h"

static Display* disp;

static uint32_t last_cursor[MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT] = {0};

void InitCursors() { disp = XOpenDisplay(NULL); }

FractalCursorImage GetCurrentCursor() {
    FractalCursorImage image = {0};
    image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
    image.cursor_state = CURSOR_STATE_VISIBLE;
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

        image.cursor_bmp_width = min(MAX_CURSOR_WIDTH, ci->width);
        image.cursor_bmp_height = min(MAX_CURSOR_HEIGHT, ci->height);
        image.cursor_bmp_hot_x = ci->xhot;
        image.cursor_bmp_hot_y = ci->yhot;

        for (int k = 0; k < image.cursor_bmp_width * image.cursor_bmp_height; ++k) {
            // we need to do this in case ci->pixels uses 8 bytes per pixel
            uint32_t argb = (uint32_t)ci->pixels[k];
            image.cursor_bmp[k] = argb;
        }

        if (memcmp(image.cursor_bmp, last_cursor,
                   sizeof(uint32_t) * MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT)) {
            image.cursor_use_bmp = true;
            memcpy(last_cursor, image.cursor_bmp,
                   sizeof(uint32_t) * MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT);
        } else {
            image.cursor_use_bmp = false;
        }

        XFree(ci);
    }

    return image;
}
