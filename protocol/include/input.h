/*
 * This file contains the headers and definitions of the DXGI screen capture functions.

 Protocol version: 1.0
 Last modification: 1/15/20

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef INPUT_H
#define INPUT_H

#include "fractal.h" // contains all the headers

typedef struct FractalCursorTypes {
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
} FractalCursorTypes;

int GetWindowsKeyCode(int sdl_keycode);

void updateKeyboardState( struct FractalClientMessage* fmsg );

FractalStatus ReplayUserInput(struct FractalClientMessage fmsg[6], int len);

FractalStatus EnterWinString(enum FractalKeycode *keycodes, int len);

void LoadCursors(FractalCursorTypes *types);

FractalCursorImage GetCursorImage(FractalCursorTypes *types, PCURSORINFO pci);

FractalCursorImage GetCurrentCursor(FractalCursorTypes *types);

#endif