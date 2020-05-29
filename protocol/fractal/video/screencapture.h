#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file screencapture.h
 * @brief This file defines the proper header for capturing the screen depending on the local OS.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#if defined(_WIN32)
#include "dxgicapture.h"
#else
#include "x11capture.h"
#endif

#endif  // SCREENCAPTURE_H
