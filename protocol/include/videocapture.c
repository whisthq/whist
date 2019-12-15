/*
 * This file contains the implementation of the functions to capture  a Windows
 * 10 desktop screen.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#include <stdio.h>
#include <stdlib.h>

#include "videocapture.h" // header file for this file

// @brief creates a struct device to capture a Windows 10 screen
// @details returns the capture device created for a specific window and area
capture_device *create_capture_device(HWND window, frame_area frame) {
  // malloc required memory and set to 0
  capture_device *device = (capture_device *) malloc(sizeof(capture_device));
  memset(device, 0, sizeof(capture_device));

  // get client screen rectangle
  RECT rect;
  GetClientRect(window, &rect);

  // store rectangle information in capture device
	device->window = window;
	device->width = 1920 * 0.7;
	device->height = 1080 * 0.7;
	device->frame = frame;
	printf("width is %d\n", device->width);
	printf("height is %d\n", device->height);
  // adjust dimensions if height or width is zero
	if (frame.width == 0 || frame.height == 0 ) {
		device->frame.width = device->width - frame.x;
		device->frame.height = device->height - frame.y;
	}

  // get final dimensions after adjustement (if happened)
	// device->width = device->frame.width;
	// device->height = device->frame.height;
	printf("width is %d\n", device->width);
	printf("height is %d\n", device->height);

  // set window features
	device->windowDC = GetDC(window);
	device->memoryDC = CreateCompatibleDC(device->windowDC);
	device->bitmap = CreateCompatibleBitmap(device->windowDC, device->width, device->height);

  // set bitmap features
	device->bitmapInfo.biSize = sizeof(BITMAPINFOHEADER);
	device->bitmapInfo.biPlanes = 1; // single plane
	device->bitmapInfo.biBitCount = 32; // 32 bit
	device->bitmapInfo.biWidth = device->width;
	device->bitmapInfo.biHeight = -device->height;
	device->bitmapInfo.biCompression = BI_RGB; // RGB color format
	device->bitmapInfo.biSizeImage = 0;

  // malloc dimensions size
	device->pixels = malloc(device->width * device->height * 4);

  // return capture device
  return device;
}

// @brief destroy the capture device
// @details deletes the capture device struct
FractalStatus destroy_capture_device(capture_device *device) {
  // check if capture device exists
	if (device == NULL) {
    printf("Cannot destroy video capture device.\n");
    return CAPTURE_ERR_DESTROY;
  }

  // release Microsoft objects memory
	ReleaseDC(device->window, device->windowDC);
  DeleteDC(device->memoryDC);
  DeleteObject(device->bitmap);

  // release standard C objects memory
	free(device->pixels);
	free(device);

  // return success
  return FRACTAL_OK;
}

// @brief captures a frame
// @details uses the capture device to capture a desktop frame
void *capture_screen(capture_device *device) {
  // select object to copy into
  SelectObject(device->memoryDC, device->bitmap);

  // get pixels
	BitBlt(device->memoryDC, 0, 0, device->width, device->height, device->windowDC, device->frame.x, device->frame.y, SRCCOPY);
	GetDIBits(device->memoryDC, device->bitmap, 0, device->height, device->pixels, (BITMAPINFO *) &(device->bitmapInfo), DIB_RGB_COLORS);

  // return pointer to captured pixels
	return device->pixels;
}
