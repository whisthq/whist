#ifndef INPUT_H
#define INPUT_H

#include "fractal.h"

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

FractalStatus ReplayUserInput(struct FractalClientMessage* fmsg);

FractalStatus EnterWinString(enum FractalKeycode *keycodes, int len);

void LoadCursors(FractalCursorTypes *types);

FractalCursorImage GetCursorImage(FractalCursorTypes *types, PCURSORINFO pci);

FractalCursorImage GetCurrentCursor(FractalCursorTypes *types);

#endif // INPUT_H