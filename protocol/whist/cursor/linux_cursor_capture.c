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

/*
        These are the cursor hashes of corresponding Whist Cursors
        (from cursor.h) returned from ::hash from ../utils/aes.c found
        experimentally
*/
typedef enum ChromeCursorHash {
    ARROW_CURSOR_HASH = 3933283985,
    IBEAM_CURSOR_HASH = 2687203118,
    WAIT_PROGRESS_CURSOR_HASH = 635125873,

    /*
        The wait cursor in Ubuntu is a spinning wheel. This is
        just a combination of a bunch of different wheel images.

        All these macros define all the hash various wait wheel cursor images
*/
    WAIT_CURSOR_HASH1 = 4281051011,
    WAIT_CURSOR_HASH2 = 1219385211,
    WAIT_CURSOR_HASH3 = 2110653072,
    WAIT_CURSOR_HASH4 = 2645617132,
    WAIT_CURSOR_HASH5 = 4109023132,
    WAIT_CURSOR_HASH6 = 3564201703,
    WAIT_CURSOR_HASH7 = 3062356816,
    WAIT_CURSOR_HASH8 = 162297790,
    WAIT_CURSOR_HASH9 = 1891884989,
    WAIT_CURSOR_HASH10 = 3681403656,
    WAIT_CURSOR_HASH11 = 3510490915,
    WAIT_CURSOR_HASH12 = 980730422,
    WAIT_CURSOR_HASH13 = 3351284218,
    WAIT_CURSOR_HASH14 = 453843329,
    WAIT_CURSOR_HASH15 = 1330002778,
    WAIT_CURSOR_HASH16 = 2194145305,
    WAIT_CURSOR_HASH17 = 3827791507,
    WAIT_CURSOR_HASH18 = 627807385,
    WAIT_CURSOR_HASH19 = 3578467491,
    WAIT_CURSOR_HASH20 = 2358572147,
    WAIT_CURSOR_HASH21 = 2200949727,
    WAIT_CURSOR_HASH22 = 66480096,
    WAIT_CURSOR_HASH23 = 3167854604,

    NWSE_CURSOR_HASH = 2133544106,
    NW_CURSOR_HASH = 1977751514,
    SE_CURSOR_HASH = 3001669061,
    NESW_CURSOR_HASH = 303720310,
    SW_CURSOR_HASH = 3760849629,
    NE_CURSOR_HASH = 3504429407,
    EW_CURSOR_HASH = 1098442634,
    NS_CURSOR_HASH = 1522636070,
    NOT_ALLOWED_CURSOR_HASH = 1482285723,
    HAND_POINT_CURSOR_HASH = 2478081084,
    HAND_GRAB_CURSOR_HASH = 3452761364,
    HAND_GRABBING_CURSOR_HASH = 3674173946,
    CROSSHAIR_CURSOR_HASH = 1236176635
} ChromeCursorHash;

static Display* disp = NULL;

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
 * @brief Matches the cursor image from the screen to a WhistCursorID
 *
 * TODO: (abecohen) X11 cursor fonts are overridden by the running application.
 * In this case, chrome will override the cursor library and use its own. If
 * we can find where that cursor file is located, get X11 to use that as its theme,
 * we may be able to just match the cursor via the name parameter in cursor_image.
 *
 * @param cursor_image the cursor image from XFixesGetCursorImage
 * @return WhistCursorID, INVALID when no matching WhistCursorID found
 */
static WhistCursorID get_cursor_id(XFixesCursorImage* cursor_image) {
    WhistCursorID id = INVALID;

    // Need to multiply the size by 4, as the width*height describes
    // number of pixels, which are 32 bit, so 4 bytes each.
    uint32_t cursor_hash =
        hash(cursor_image->pixels, 4 * cursor_image->width * cursor_image->height);
    switch (cursor_hash) {
        case ARROW_CURSOR_HASH:
            id = WHIST_CURSOR_ARROW;
            break;
        case IBEAM_CURSOR_HASH:
            id = WHIST_CURSOR_IBEAM;
            break;
        case WAIT_PROGRESS_CURSOR_HASH:
            id = WHIST_CURSOR_WAITARROW;
            break;
        case WAIT_CURSOR_HASH1:
        case WAIT_CURSOR_HASH2:
        case WAIT_CURSOR_HASH3:
        case WAIT_CURSOR_HASH4:
        case WAIT_CURSOR_HASH5:
        case WAIT_CURSOR_HASH6:
        case WAIT_CURSOR_HASH7:
        case WAIT_CURSOR_HASH8:
        case WAIT_CURSOR_HASH9:
        case WAIT_CURSOR_HASH10:
        case WAIT_CURSOR_HASH11:
        case WAIT_CURSOR_HASH12:
        case WAIT_CURSOR_HASH13:
        case WAIT_CURSOR_HASH14:
        case WAIT_CURSOR_HASH15:
        case WAIT_CURSOR_HASH16:
        case WAIT_CURSOR_HASH17:
        case WAIT_CURSOR_HASH18:
        case WAIT_CURSOR_HASH19:
        case WAIT_CURSOR_HASH20:
        case WAIT_CURSOR_HASH21:
        case WAIT_CURSOR_HASH22:
        case WAIT_CURSOR_HASH23:
            id = WHIST_CURSOR_WAIT;
            break;
        case NWSE_CURSOR_HASH:
        case NW_CURSOR_HASH:
        case SE_CURSOR_HASH:
            id = WHIST_CURSOR_SIZENWSE;
            break;
        case NESW_CURSOR_HASH:
        case NE_CURSOR_HASH:
        case SW_CURSOR_HASH:
            id = WHIST_CURSOR_SIZENESW;
            break;
        case EW_CURSOR_HASH:
            id = WHIST_CURSOR_SIZEWE;
            break;
        case NS_CURSOR_HASH:
            id = WHIST_CURSOR_SIZENS;
            break;
        case NOT_ALLOWED_CURSOR_HASH:
            id = WHIST_CURSOR_NO;
            break;
        case CROSSHAIR_CURSOR_HASH:
            id = WHIST_CURSOR_CROSSHAIR;
            break;
        case HAND_POINT_CURSOR_HASH:
        case HAND_GRAB_CURSOR_HASH:
        case HAND_GRABBING_CURSOR_HASH:
            id = WHIST_CURSOR_HAND;
            break;
    }

    return id;
}

static WhistCursorState get_latest_cursor_state(void) {
#define POINTER_LOCK_UPDATE_TRIGGER_FILE "/home/whist/.teleport/pointer-lock-update"
    static WhistCursorState state = CURSOR_STATE_VISIBLE;
    static WhistTimer timer;
    static bool first_run = true;
    if (first_run) {
        start_timer(&timer);
        first_run = false;
    }

    // TODO: Is it too costly to do this in the video capture loop?
    if (get_timer(&timer) > 20.0 / MS_IN_SECOND) {
        if (!access(POINTER_LOCK_UPDATE_TRIGGER_FILE, R_OK)) {
            int fd = open(POINTER_LOCK_UPDATE_TRIGGER_FILE, O_RDONLY);
            char pointer_locked[2] = {0};
            read(fd, pointer_locked, 1);
            close(fd);
            if (pointer_locked[0] == '1') {
                state = CURSOR_STATE_HIDDEN;
            } else if (pointer_locked[0] == '0') {
                state = CURSOR_STATE_VISIBLE;
            } else {
                LOG_ERROR("Invalid pointer lock state");
            }
            remove(POINTER_LOCK_UPDATE_TRIGGER_FILE);
        }
        start_timer(&timer);
    }

    return state;
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

        WhistCursorState state = get_latest_cursor_state();

        WhistCursorID whist_id = get_cursor_id(cursor_image);
        if (whist_id == INVALID) {
            // Use PNG cursor

            // Convert argb to rgba
            uint32_t rgba[MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT];
            for (int i = 0; i < cursor_image->width * cursor_image->height; i++) {
                const uint32_t argb_pix = (uint32_t)cursor_image->pixels[i];
                rgba[i] = argb_pix << 8 | argb_pix >> 24;
            }
            image = whist_cursor_info_from_rgba(rgba, cursor_image->width, cursor_image->height,
                                                cursor_image->xhot, cursor_image->yhot, state);
        } else {
            // Use system cursor from WhistCursorID
            image = whist_cursor_info_from_id(whist_id, state);
        }
        XFree(cursor_image);
    } else {
        LOG_ERROR("Cursor capture not initialized");
    }

    return image;
}
