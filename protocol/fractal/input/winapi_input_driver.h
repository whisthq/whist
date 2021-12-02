#ifndef WINAPI_INPUT_DRIVER_H
#define WINAPI_INPUT_DRIVER_H
/**
 * Copyright 2021 Whist Technologies, Inc.
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

#include <fractal/core/whist.h>

/*
============================
Custom Types
============================
*/

typedef char InputDevice;

#endif  // WINAPI_INPUT_DRIVER_H
