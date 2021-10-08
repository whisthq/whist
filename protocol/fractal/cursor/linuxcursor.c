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
#include "../utils/aes.h"
#include "cursor.h"
#include "string.h"

/*
        These are the cursor hashes of corresponding Fractal Cursors
        (from cursor.h) returned from ::hash from ../utils/aes.c found
        expirementally
*/
#define ARROW_CURSOR_HASH 3933283985
#define IBEAM_CURSOR_HASH 2687203118
#define WAIT_PROGRESS_CURSOR_HASH 635125873

/*
        The wait cursor in Ubuntu is a spinning wheel. This is 
        just a combination of a bunch of different wheel images.

        All these macros define all the hash various wait wheel cursor images
*/
#define WAIT_CURSOR_HASH1 4281051011
#define WAIT_CURSOR_HASH2 1219385211
#define WAIT_CURSOR_HASH3 2110653072
#define WAIT_CURSOR_HASH4 2645617132
#define WAIT_CURSOR_HASH5 4109023132
#define WAIT_CURSOR_HASH6 3564201703
#define WAIT_CURSOR_HASH7 3062356816
#define WAIT_CURSOR_HASH8 162297790
#define WAIT_CURSOR_HASH9 1891884989
#define WAIT_CURSOR_HASH10 3681403656
#define WAIT_CURSOR_HASH11 3510490915
#define WAIT_CURSOR_HASH12 980730422
#define WAIT_CURSOR_HASH13 3351284218
#define WAIT_CURSOR_HASH14 453843329
#define WAIT_CURSOR_HASH15 3351284218
#define WAIT_CURSOR_HASH16 1330002778
#define WAIT_CURSOR_HASH17 2194145305
#define WAIT_CURSOR_HASH18 3827791507
#define WAIT_CURSOR_HASH19 627807385
#define WAIT_CURSOR_HASH20 3578467491
#define WAIT_CURSOR_HASH21 2358572147
#define WAIT_CURSOR_HASH22 2200949727
#define WAIT_CURSOR_HASH23 66480096
#define WAIT_CURSOR_HASH24 3167854604

#define NWSE_CURSOR_HASH 2133544106
#define NW_CURSOR_HASH 1977751514
#define SE_CURSOR_HASH 3001669061
#define NESW_CURSOR_HASH 303720310
#define SW_CURSOR_HASH 3760849629
#define NE_CURSOR_HASH 3504429407
#define EW_CURSOR_HASH 1098442634
#define NS_CURSOR_HASH 1522636070
#define NOT_ALLOWED_CURSOR_HASH 1482285723
#define HAND_POINT_CURSOR_HASH 2478081084
#define HAND_GRAB_CURSOR_HASH 3452761364
#define HAND_GRABBING_CURSOR_HASH 3674173946
#define CROSSHAIR_CURSOR_HASH 1236176635


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

/**
 * @brief Matches the cursor image from the screen to a FractalCursorID
 * 
 * TODO: (abecohen) X11 cursor fonts are overridden by the running application.
 * In this case, chrome will override the cursor library and use its own. If
 * we can find where that cursor file is located, get X11 to use that as its theme,
 * we may be able to just match the cursor via the name parameter in ci. 
 * 
 * @param ci the cursor image from XFixesGetCursorImage 
 * @return FractalCursorID, INVALID when no matching FractalCursorID found 
 */
FractalCursorID get_cursor_id(XFixesCursorImage* ci) {
        FractalCursorID id = INVALID;

        //Need to multiply the size by 4, as the width*height describes 
        //number of pixels, which are 32 bit, so 4 bytes each.
        uint32_t cursor_hash = hash(ci->pixels, 4 * ci->width * ci->height);
        if(cursor_hash == ARROW_CURSOR_HASH) {
                id = FRACTAL_CURSOR_ARROW;
        }
        else if(cursor_hash == IBEAM_CURSOR_HASH) {
                id = FRACTAL_CURSOR_IBEAM;
        }
        else if(cursor_hash == WAIT_PROGRESS_CURSOR_HASH) {
                id = FRACTAL_CURSOR_WAITARROW;
        }
        else if(cursor_hash == WAIT_CURSOR_HASH1 ||
                cursor_hash == WAIT_CURSOR_HASH2 ||
                cursor_hash == WAIT_CURSOR_HASH3 ||
                cursor_hash == WAIT_CURSOR_HASH4 ||
                cursor_hash == WAIT_CURSOR_HASH5 ||
                cursor_hash == WAIT_CURSOR_HASH6 ||
                cursor_hash == WAIT_CURSOR_HASH7 ||
                cursor_hash == WAIT_CURSOR_HASH8 ||
                cursor_hash == WAIT_CURSOR_HASH9 ||
                cursor_hash == WAIT_CURSOR_HASH10 ||
                cursor_hash == WAIT_CURSOR_HASH11 ||
                cursor_hash == WAIT_CURSOR_HASH12 ||
                cursor_hash == WAIT_CURSOR_HASH13 ||
                cursor_hash == WAIT_CURSOR_HASH14 ||
                cursor_hash == WAIT_CURSOR_HASH15 ||
                cursor_hash == WAIT_CURSOR_HASH16 ||
                cursor_hash == WAIT_CURSOR_HASH17 ||
                cursor_hash == WAIT_CURSOR_HASH18 ||
                cursor_hash == WAIT_CURSOR_HASH19 ||
                cursor_hash == WAIT_CURSOR_HASH20 ||
                cursor_hash == WAIT_CURSOR_HASH21 ||
                cursor_hash == WAIT_CURSOR_HASH22 ||
                cursor_hash == WAIT_CURSOR_HASH23 ||
                cursor_hash == WAIT_CURSOR_HASH24
                ) {

                id = FRACTAL_CURSOR_WAIT;
        }
        else if(cursor_hash == NWSE_CURSOR_HASH || cursor_hash == NW_CURSOR_HASH || cursor_hash == SE_CURSOR_HASH) {
                id = FRACTAL_CURSOR_SIZENWSE;
        }
        else if(cursor_hash == NESW_CURSOR_HASH || cursor_hash == NE_CURSOR_HASH || cursor_hash == SW_CURSOR_HASH) {
                id = FRACTAL_CURSOR_SIZENESW;
        }
        else if(cursor_hash == EW_CURSOR_HASH) {
                id = FRACTAL_CURSOR_SIZEWE;
        }
        else if(cursor_hash == NS_CURSOR_HASH) {
                id = FRACTAL_CURSOR_SIZENS;
        }
        else if(cursor_hash == NOT_ALLOWED_CURSOR_HASH) {
                id = FRACTAL_CURSOR_NO;
        }
        else if(cursor_hash == CROSSHAIR_CURSOR_HASH) {
                id = FRACTAL_CURSOR_CROSSHAIR;
        }
        else if(cursor_hash == HAND_POINT_CURSOR_HASH || cursor_hash == HAND_GRAB_CURSOR_HASH || cursor_hash == HAND_GRABBING_CURSOR_HASH) {
                id = FRACTAL_CURSOR_HAND;
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

        XFixesCursorImage* ci = XFixesGetCursorImage(disp);

        if (ci->width > MAX_CURSOR_WIDTH || ci->height > MAX_CURSOR_HEIGHT) {
            LOG_WARNING(
                "fractal/cursor/linuxcursor.c::GetCurrentCursor(): cursor width or height exceeds "
                "maximum dimensions. Truncating cursor from %hu by %hu to %hu by %hu.",
                ci->width, ci->height, MAX_CURSOR_WIDTH, MAX_CURSOR_HEIGHT);
        }

        if(INVALID == (image->cursor_id = get_cursor_id(ci))) {
                image->using_bmp = true;
                image->bmp_width = min(MAX_CURSOR_WIDTH, ci->width);
                image->bmp_height = min(MAX_CURSOR_HEIGHT, ci->height);
                image->bmp_hot_x = ci->xhot;
                image->bmp_hot_y = ci->yhot;

                for(int k = 0; k < image->bmp_width * image->bmp_height; ++k) {
                        // we need to do this in case ci->pixels uses 8 bytes per pixel
                        uint32_t argb = (uint32_t)ci->pixels[k];
                        image->bmp[k] = argb;
                }
        }

        XFree(ci);
    }
}