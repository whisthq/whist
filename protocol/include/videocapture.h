/*
 * This file contains the headers and definitions of the functions to capture
 * a Windows 10 desktop screen.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef VIDEOCAPTURE_H  // include guard
#define VIDEOCAPTURE_H

#include "fractal.h" // contains all the headers


// struct holding the frame
typedef struct {
	int x, y, width, height;
} frame_area;

// struct to capture a frame
typedef struct {
	HWND window;
	HDC windowDC;
	HDC memoryDC;
	HBITMAP bitmap;
	BITMAPINFOHEADER bitmapInfo;
	int width;
	int height;
	void *pixels;
	frame_area frame;
} video_capture_device;

// @brief creates a struct device to capture a Windows 10 screen
// @details returns the capture device created for a specific window and area
video_capture_device *create_video_capture_device(HWND window, frame_area frame);

// @brief destroy the capture device
// @details deletes the capture device struct
FractalStatus destroy_video_capture_device(video_capture_device *device);

// @brief captures a frame
// @details uses the capture device to capture a desktop frame
void *capture_screen(video_capture_device *device);

#endif // VIDEOCAPTURE_H
