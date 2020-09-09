#ifndef WINAPI_INPUT_DRIVER_H
#define WINAPI_INPUT_DRIVER_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file winapi_input_driver.h
 * @brief This file defines the Windows input device type as a char dummy.
============================
Usage
============================

You can create and use an input device to receive input (keystrokes,
mouse clicks, etc.) via `CreateInputDevice` and other functions defined
in input_driver.h
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

typedef char input_device_t;

#endif  // WINAPI_INPUT_DRIVER_H