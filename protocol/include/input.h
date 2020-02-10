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


FractalStatus ReplayUserInput(struct FractalClientMessage fmsg[6], int len);

FractalStatus EnterWinString(enum FractalKeycode keycodes[100], int len);

void LoadCursors(FractalCursorTypes *types);

FractalCursorImage GetCursorImage(FractalCursorTypes *types, PCURSORINFO pci);

FractalCursorImage GetCurrentCursor(FractalCursorTypes *types);

#endif