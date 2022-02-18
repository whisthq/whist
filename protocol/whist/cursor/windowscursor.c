/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file windowscursor.c
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

#include "cursor.h"

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
static WhistCursorID get_cursor_id(PCURSORINFO pci) {
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
        return WHIST_CURSOR_NO;
    } else if (cursor == types->CursorSizeAll) {
        return WHIST_CURSOR_SIZEALL;
    } else if (cursor == types->CursorSizeNESW) {
        return WHIST_CURSOR_SIZENESW;
    } else if (cursor == types->CursorSizeNS) {
        return WHIST_CURSOR_SIZENS;
    } else if (cursor == types->CursorSizeNWSE) {
        return WHIST_CURSOR_SIZENWSE;
    } else if (cursor == types->CursorSizeWE) {
        return WHIST_CURSOR_SIZEWE;
    } else if (cursor == types->CursorWait) {
        return WHIST_CURSOR_WAITARROW;
    } else {
        return WHIST_CURSOR_ARROW;
    }
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
    return whist_cursor_info_from_id(get_cursor_id(&cursor_info), cursor_info.flags);
}
