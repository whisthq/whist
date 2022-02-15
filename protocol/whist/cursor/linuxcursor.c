/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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
#include <whist/logging/logging.h>
#include <whist/utils/aes.h>
#include <whist/utils/lodepng.h>
#include "cursor.h"
#include "string.h"

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

static Display* disp;
/*
============================
Public Function Implementations
============================
*/

void init_cursors(void) {
    /*
        Initialize all cursors by creating the display
    */

    disp = XOpenDisplay(NULL);
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

WhistCursorInfo* get_current_cursor(void) {
    WhistCursorInfo* image = NULL;
    if (disp) {
        XFixesCursorImage* cursor_image = XFixesGetCursorImage(disp);

        if (cursor_image->width > MAX_CURSOR_WIDTH || cursor_image->height > MAX_CURSOR_HEIGHT) {
            LOG_WARNING("Cursor is too large; rejecting capture: %hux%hu", cursor_image->width,
                        cursor_image->height);
            XFree(cursor_image);
            return NULL;
        }

        WhistCursorID whist_id = get_cursor_id(cursor_image);
        if (whist_id == INVALID) {
            // Use PNG cursor
            unsigned char* png;
            size_t png_size;

            // Convert argb to rgba
            uint32_t* rgba =
                safe_malloc(sizeof(uint32_t) * cursor_image->width * cursor_image->height);
            for (int i = 0; i < cursor_image->width * cursor_image->height; i++) {
                uint32_t argb = (uint32_t)cursor_image->pixels[i];
                rgba[i] = argb << 8 | argb >> 24;
            }

            unsigned int ret = lodepng_encode32(&png, &png_size, (unsigned char*)rgba,
                                                cursor_image->width, cursor_image->height);
            free(rgba);
            if (ret) {
                LOG_WARNING("Failed to encode PNG cursor: %s", lodepng_error_text(ret));
                XFree(cursor_image);
                return NULL;
            }

            image = malloc(sizeof(WhistCursorInfo) + png_size);
            image->using_png = true;
            image->png_width = cursor_image->width;
            image->png_height = cursor_image->height;
            image->png_size = png_size;
            image->png_hot_x = cursor_image->xhot;
            image->png_hot_y = cursor_image->yhot;
            memcpy(image->png, png, png_size);
            image->hash = hash(png, png_size);
            free(png);
        } else {
            // Use system cursor from WhistCursorID
            image = malloc(sizeof(WhistCursorInfo));
            memset(image, 0, sizeof(WhistCursorInfo));
            image->cursor_id = whist_id;
            image->using_png = false;
            image->hash = hash(&whist_id, sizeof(WhistCursorID));
        }
        XFree(cursor_image);
    }

    return image;
}
