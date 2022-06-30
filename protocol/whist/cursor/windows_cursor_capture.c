/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file windows_cursor_capture.c
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

#include <windows.h>
#include <whist/core/whist_memory.h>
#include <whist/utils/aes.h>

#include "cursor_internal.h"

/*
============================
Custom Types
============================
*/

typedef struct WhistCursorTypes {
    HCURSOR CursorAppStarting;
    HCURSOR CursorArrow;
    HCURSOR CursorCross;
    HCURSOR CursorHand;
    HCURSOR CursorHelp;
    HCURSOR CursorIBeam;
    HCURSOR CursorIcon;
    HCURSOR CursorNo;
    HCURSOR CursorSize;
    HCURSOR CursorSizeAll;
    HCURSOR CursorSizeNESW;
    HCURSOR CursorSizeNS;
    HCURSOR CursorSizeNWSE;
    HCURSOR CursorSizeWE;
    HCURSOR CursorUpArrow;
    HCURSOR CursorWait;
} WhistCursorTypes;

static struct WhistCursorTypes l_types = {0};
static struct WhistCursorTypes* types = &l_types;

/*
============================
Private Function Implementations
============================
*/

/**
 * @brief   Load each cursor image by type.
 */
static void load_cursors(void) {
    types->CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
    types->CursorArrow = LoadCursor(NULL, IDC_ARROW);
    types->CursorCross = LoadCursor(NULL, IDC_CROSS);
    types->CursorHand = LoadCursor(NULL, IDC_HAND);
    types->CursorHelp = LoadCursor(NULL, IDC_HELP);
    types->CursorIBeam = LoadCursor(NULL, IDC_IBEAM);
    types->CursorIcon = LoadCursor(NULL, IDC_ICON);
    types->CursorNo = LoadCursor(NULL, IDC_NO);
    types->CursorSize = LoadCursor(NULL, IDC_SIZE);
    types->CursorSizeAll = LoadCursor(NULL, IDC_SIZEALL);
    types->CursorSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
    types->CursorSizeNS = LoadCursor(NULL, IDC_SIZENS);
    types->CursorSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);
    types->CursorSizeWE = LoadCursor(NULL, IDC_SIZEWE);
    types->CursorUpArrow = LoadCursor(NULL, IDC_UPARROW);
    types->CursorWait = LoadCursor(NULL, IDC_WAIT);
}

/**
 * @brief       Get the Whist cursor ID from the cursor info struct
 *
 * @param pci   A pointer to the cursor info struct
 */
static WhistCursorType get_cursor_type(PCURSORINFO pci) {
    HCURSOR cursor = pci->hCursor;

    if (cursor == types->CursorArrow) {
        return WHIST_CURSOR_ARROW;
    } else if (cursor == types->CursorCross) {
        return WHIST_CURSOR_CROSSHAIR;
    } else if (cursor == types->CursorHand) {
        return WHIST_CURSOR_HAND;
    } else if (cursor == types->CursorIBeam) {
        return WHIST_CURSOR_IBEAM;
    } else if (cursor == types->CursorNo) {
        return WHIST_CURSOR_NOT_ALLOWED;
    } else if (cursor == types->CursorSizeAll) {
        return WHIST_CURSOR_ALL_SCROLL;
    } else if (cursor == types->CursorSizeNESW) {
        return WHIST_CURSOR_RESIZE_NESW;
    } else if (cursor == types->CursorSizeNS) {
        return WHIST_CURSOR_RESIZE_NS;
    } else if (cursor == types->CursorSizeNWSE) {
        return WHIST_CURSOR_RESIZE_NWSE;
    } else if (cursor == types->CursorSizeWE) {
        return WHIST_CURSOR_RESIZE_WE;
    } else if (cursor == types->CursorWait) {
        return WHIST_CURSOR_WAIT;
    }

    // We don't support PNG cursors, so fallback to a regular arrow.
    return WHIST_CURSOR_ARROW;
}

/*
============================
Public Function Implementations
============================
*/

void whist_cursor_capture_init(void) { load_cursors(); }

void whist_cursor_capture_destroy(void) {}

WhistCursorInfo* whist_cursor_capture(void) {
    CURSORINFO cursor_info;
    cursor_info.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&cursor_info);
    bool cursor_visible = cursor_info.flags & CURSOR_SHOWING;
    // We used to use cursor_info.flags as a proxy for cursor capture state, but that's incorrect.
    // Since we don't really care about Windows server functionality, just always set this to
    // CURSOR_CAPTURE_STATE_NORMAL.
    return whist_cursor_info_from_type(
        cursor_visible ? get_cursor_type(&cursor_info) : WHIST_CURSOR_NONE,
        CURSOR_CAPTURE_STATE_NORMAL);
}
