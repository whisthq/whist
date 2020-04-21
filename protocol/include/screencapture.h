/*
 * Dynamically selects Windows or Linux screen capture based on server OS.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#if defined(_WIN32)
#include "dxgicapture.h"
#else
#include "x11capture.h"
#endif

#endif // SCREENCAPTURE_H