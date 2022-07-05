/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file linux_cursor_capture.c
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
#include <whist/logging/logging.h>
#include <whist/utils/aes.h>
#include <string.h>
#include <fcntl.h>

#include "cursor_internal.h"

static Display* disp = NULL;

#define NUM_GTK_CURSORS 60
static const WhistCursorType gtk_index_to_cursor_type[NUM_GTK_CURSORS] = {
    WHIST_CURSOR_ALL_SCROLL,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_CELL,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_NO_DROP,
    WHIST_CURSOR_GRABBING,
    WHIST_CURSOR_RESIZE_COL,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_CONTEXT_MENU,
    WHIST_CURSOR_COPY,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_CROSSHAIR,
    WHIST_CURSOR_HELP,
    WHIST_CURSOR_COPY,
    WHIST_CURSOR_ALIAS,
    WHIST_CURSOR_MOVE,
    WHIST_CURSOR_NO_DROP,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_RESIZE_E,
    WHIST_CURSOR_RESIZE_EW,
    WHIST_CURSOR_HELP,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_ARROW,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_ALIAS,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_MOVE,
    WHIST_CURSOR_RESIZE_N,
    WHIST_CURSOR_RESIZE_NE,
    WHIST_CURSOR_RESIZE_NESW,
    WHIST_CURSOR_NOT_ALLOWED,
    WHIST_CURSOR_RESIZE_NS,
    WHIST_CURSOR_RESIZE_NW,
    WHIST_CURSOR_RESIZE_NWSE,
    WHIST_CURSOR_GRAB,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_HAND,
    WHIST_CURSOR_PROGRESS,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_RESIZE_ROW,
    WHIST_CURSOR_RESIZE_S,
    WHIST_CURSOR_RESIZE_SE,
    WHIST_CURSOR_RESIZE_SW,
    WHIST_CURSOR_IBEAM,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_IBEAM_VERTICAL,
    WHIST_CURSOR_RESIZE_W,
    WHIST_CURSOR_WAIT,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_ZOOM_IN,
    WHIST_CURSOR_ZOOM_OUT,
};

/*
============================
Public Function Implementations
============================
*/

void whist_cursor_capture_init(void) {
    if (disp == NULL) {
        disp = XOpenDisplay(NULL);
    } else {
        LOG_ERROR("Cursor capture already initialized");
    }
}

void whist_cursor_capture_destroy(void) {
    if (disp != NULL) {
        XCloseDisplay(disp);
        disp = NULL;
    }
}

/**
 * @brief                   Get a WhistCursorType corresponding to the captured
 *                          cursor image
 *
 * @param cursor_image      The X11 cursor image retrieved from XFixesGetCursorImage
 *
 * @returns                 The appropriate WhistCursorType, or WHIST_CURSOR_PNG if
 *                          the image is not a recognized cursor type.
 */
static WhistCursorType get_cursor_type(XFixesCursorImage* cursor_image) {
    // A lot of this logic is hardcoded, either in X11, or in the cursor theme provided in
    // mandelboxes/base/display/theme/cursor-builder. Comments here will explain what is
    // going on, but for ultimate clarity look at the theme generation files.

    // If the cursor is a 1x1 transparent cursor, the cursor is actually hidden, as per GTK.
    if (cursor_image->width == 1 && cursor_image->height == 1 &&
        (cursor_image->pixels[0] & 0xff000000) == 0) {
        return WHIST_CURSOR_NONE;
    }

    // Our GTK cursor theme guarantees that all actual system cursors are 32x32 squares whose
    // quadrants are distinctive colors. So we first check cursor size. Then (rather than
    // checking every pixel), we check the four corners for whether they match their colors.

    // Size check
    if (cursor_image->width != 32 || cursor_image->height != 32) {
        // Not a system cursor
        return WHIST_CURSOR_PNG;
    }

    // Color check
    uint32_t northwest = cursor_image->pixels[0] & 0x00ffffff;
    uint32_t northeast = cursor_image->pixels[31] & 0x00ffffff;
    uint32_t southwest = cursor_image->pixels[32 * 31] & 0x00ffffff;
    uint32_t southeast = cursor_image->pixels[32 * 31 + 31] & 0x00ffffff;

    const uint32_t northwest_color = 0x42f58a;
    const uint32_t northeast_color = 0xe742f5;
    const uint32_t southwest_color = 0x2f7aeb;
    const uint32_t southeast_color = 0xff7e20;

    if (northwest != northwest_color || northeast != northeast_color ||
        southwest != southwest_color || southeast != southeast_color) {
        // Not a system cursor
        return WHIST_CURSOR_PNG;
    }

    // Now we're guaranteed to have a system cursor. Our cursor theme encodes cursor type in
    // the hot coordinates of each cursor, so that we can avoid processing the entire image.

    // We alphebatize the GTK cursor types, and associate each type to its index in that list.
    // See the list and mappings here:
    // https://docs.google.com/spreadsheets/d/1rfaAAePjQrz_19CTkpN4zmnNJ_FOSrjOfWbyGLgE4zE/edit?usp=sharing

    // In our encoding, xhot and yhot are both multiples of three, and the index is calculated
    // as (x/3) + 7 * (y/3).

    // As a sanity check, let's warn if they're not multiples of three.
    if (cursor_image->xhot % 3 != 0 || cursor_image->yhot % 3 != 0) {
        LOG_WARNING_RATE_LIMITED(5, 1,
                                 "Cursor hot-spot is supposed to be a multiple of 3: (%d, %d)",
                                 cursor_image->xhot, cursor_image->yhot);
    }

    int index = (cursor_image->xhot / 3) + 7 * (cursor_image->yhot / 3);

    if (index < 0 || index >= NUM_GTK_CURSORS) {
        LOG_WARNING("Received system cursor with weird hot dimensions: %d x %d", cursor_image->xhot,
                    cursor_image->yhot);
        return WHIST_CURSOR_PNG;
    }

    return gtk_index_to_cursor_type[index];
}

static WhistMouseMode get_latest_mouse_mode(void) {
#define POINTER_LOCK_UPDATE_TRIGGER_FILE "/home/whist/.teleport/pointer-lock-update"
    static WhistMouseMode mode = MOUSE_MODE_NORMAL;
    static WhistTimer timer;
    static bool first_run = true;
    if (first_run) {
        start_timer(&timer);
        first_run = false;
    }

    // This happens in the video capture loop, but takes negligible time
    if (get_timer(&timer) > 20.0 / MS_IN_SECOND) {
        if (!access(POINTER_LOCK_UPDATE_TRIGGER_FILE, R_OK)) {
            int fd = open(POINTER_LOCK_UPDATE_TRIGGER_FILE, O_RDONLY);
            char pointer_locked[2] = {0};
            read(fd, pointer_locked, 1);
            close(fd);
            if (pointer_locked[0] == '1') {
                mode = MOUSE_MODE_RELATIVE;
            } else if (pointer_locked[0] == '0') {
                mode = MOUSE_MODE_NORMAL;
            } else {
                LOG_ERROR("Invalid pointer lock state");
            }
            remove(POINTER_LOCK_UPDATE_TRIGGER_FILE);
        }
        start_timer(&timer);
    }

    return mode;
}

WhistCursorInfo* whist_cursor_capture(void) {
    WhistCursorInfo* image = NULL;
    if (disp) {
        XFixesCursorImage* cursor_image = XFixesGetCursorImage(disp);

        if (cursor_image->width > MAX_CURSOR_WIDTH || cursor_image->height > MAX_CURSOR_HEIGHT) {
            LOG_WARNING("Cursor is too large; rejecting capture: %hux%hu", cursor_image->width,
                        cursor_image->height);
            XFree(cursor_image);
            return NULL;
        }

        WhistMouseMode mode = get_latest_mouse_mode();

        WhistCursorType cursor_type = get_cursor_type(cursor_image);
        if (cursor_type == WHIST_CURSOR_PNG) {
            // Use PNG cursor

            // Convert argb to rgba
            uint32_t rgba[MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT];
            for (int i = 0; i < cursor_image->width * cursor_image->height; i++) {
                const uint32_t argb_pix = (uint32_t)cursor_image->pixels[i];
                rgba[i] = argb_pix << 8 | argb_pix >> 24;
            }
            image = whist_cursor_info_from_rgba(rgba, cursor_image->width, cursor_image->height,
                                                cursor_image->xhot, cursor_image->yhot, mode);
        } else {
            // Use system cursor
            image = whist_cursor_info_from_type(cursor_type, mode);
        }
        XFree(cursor_image);
    } else {
        LOG_ERROR("Cursor capture not initialized");
    }

    return image;
}
