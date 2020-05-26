#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

/**
 Copyright Fractal Computers, Inc. 2020
 @file dxgicapture.h
 @date 26 may 2020
 @brief This file defines the proper header for capturing the screen depending on the local OS.

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
