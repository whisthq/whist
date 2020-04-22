/*
 * User input processing on Windows.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef WINDOWS_INPUT_H
#define WINDOWS_INPUT_H

#include "fractal.h"

typedef char input_device_t;

void EnterWinString(enum FractalKeycode* keycodes, int len);

#endif // WINDOWS_INPUT_H
