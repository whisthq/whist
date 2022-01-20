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

struct WhistCursorTypes l_types = {0};
struct WhistCursorTypes* types = &l_types;

/*
============================
Private Functions
============================
*/

static WhistCursorImage get_cursor_image(PCURSORINFO pci);

static void load_cursors(void);

/*
============================
Private Function Implementations
============================
*/

void load_cursors(void) {
    /*
        Load each cursor by type
    */

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

WhistCursorImage get_cursor_image(PCURSORINFO pci) {
    /*
        Get the corresponding cursor image for by cursor type

        Arguments:
            pci (PCURSORINFO): Windows cursor info

        Returns:
            (WhistCursorImage): the corresponding WhistCursorImage
                for the `pci` cursor type
    */

    HCURSOR cursor = pci->hCursor;
    WhistCursorImage image = {0};

    if (cursor == types->CursorArrow) {
        image.cursor_id = WHIST_CURSOR_ARROW;
    } else if (cursor == types->CursorCross) {
        image.cursor_id = WHIST_CURSOR_CROSSHAIR;
    } else if (cursor == types->CursorHand) {
        image.cursor_id = WHIST_CURSOR_HAND;
    } else if (cursor == types->CursorIBeam) {
        image.cursor_id = WHIST_CURSOR_IBEAM;
    } else if (cursor == types->CursorNo) {
        image.cursor_id = WHIST_CURSOR_NO;
    } else if (cursor == types->CursorSizeAll) {
        image.cursor_id = WHIST_CURSOR_SIZEALL;
    } else if (cursor == types->CursorSizeNESW) {
        image.cursor_id = WHIST_CURSOR_SIZENESW;
    } else if (cursor == types->CursorSizeNS) {
        image.cursor_id = WHIST_CURSOR_SIZENS;
    } else if (cursor == types->CursorSizeNWSE) {
        image.cursor_id = WHIST_CURSOR_SIZENWSE;
    } else if (cursor == types->CursorSizeWE) {
        image.cursor_id = WHIST_CURSOR_SIZEWE;
    } else if (cursor == types->CursorWait) {
        image.cursor_id = WHIST_CURSOR_WAITARROW;
    } else {
        image.cursor_id = WHIST_CURSOR_ARROW;
    }

    return image;
}

/*
============================
Public Function Implementations
============================
*/

void init_cursors(void) {
    /*
        Initialize all cursors by loading cursors
    */

    load_cursors();
}

void get_current_cursor(WhistCursorImage* image) {
    /*
        Returns the current cursor image

        Returns:
            (WhistCursorImage): Current WhistCursorImage
    */

    CURSORINFO pci;
    pci.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&pci);

    *image = get_cursor_image(&pci);

    image->cursor_state = pci.flags;
}
