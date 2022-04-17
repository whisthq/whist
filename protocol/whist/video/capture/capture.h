#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file capture.h
 * @brief This file defines the proper header for capturing the screen depending
 *        on the local OS.
============================
Usage
============================

Toggles automatically between the screen capture files based on OS.
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include <whist/utils/color.h>
#ifdef __linux__
#include <X11/Xlib.h>
#include "nvidiacapture.h"
#include "x11capture.h"
#endif

/*
============================
Custom Types
============================
*/

// TODO: more members needed?
typedef struct WhistWindow {
#ifdef __linux__
    Window window;
#endif
} WhistWindow;

// TODO: this should go in a separate file 
typedef enum WindowMessageType {
    // tell the client to open a new window
    WINDOW_CREATE;
    // tell the server or client to close a window
    WINDOW_DELETE;
    // tell the server to resize a window
    WINDOW_RESIZE;
    // tell the server to switch active window
    WINDOW_ACTIVE;
}

typedef struct WindowMessage {
    WindowMessageType type;
    int id;
    int width;
    int height;
} WindowMessage;

typedef struct CaptureDevice {
    int id;
    int width;
    int height;
    int pitch;
    void* frame_data;
    WhistRGBColor corner_color;
    WhistWindow active_window;
    void* internal;

#ifdef __linux__
    CaptureDeviceType active_capture_device;  // the device currently used for capturing
    CaptureDeviceType last_capture_device;  // the device used for the last capture, so we can pick
                                            // the right encoder
    bool pending_destruction;
    WhistThread nvidia_manager;
    WhistSemaphore nvidia_device_semaphore;
    WhistSemaphore nvidia_device_created;
    bool nvidia_context_is_stale;
    // Shared X11 state
    Display* display;
    Window root;
    // Underlying X11/Nvidia capture devices
    NvidiaCaptureDevice* nvidia_capture_device;
    X11CaptureDevice* x11_capture_device;
#endif
} CaptureDevice;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Creates a screen capture device
 *
 * @param device                   Capture device struct to hold the capture
 *                                 device
 * @param width                    Width of the screen to capture, in pixels
 * @param height                   Height of the screen to capture, in pixels
 * @param dpi                      Dots per sq inch of the screen, (Where 96 is neutral)
 *
 * @returns                        0 if succeeded, else -1
 */
int create_capture_device(CaptureDevice* device, WhistWindow window, uint32_t width, uint32_t height, uint32_t dpi);

/**
 * @brief                          Tries to reconfigure the capture device
 *                                 using the newly provided parameters
 *
 * @param device                   The Capture Device to reconfigure
 * @param width                    New width of the screen to capture, in pixels
 * @param height                   New height of the screen to capture, in pixels
 * @param dpi                      New DPI
 *
 * @returns                        0 if succeeded, else -1
 */
bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height,
                                uint32_t dpi);

/**
 * @brief                          Capture a bitmap snapshot of the screen
 *                                 The width/height of the image is guaranteed to be
 *                                 the width/height passed into create_/reconfigure_ capture device,
 *                                 If the screen dimensions changed, then -1 will be returned
 *
 * @param device                   The device used to capture the screen
 *
 * @returns                        Number of frames since last capture if succeeded, else -1
 */
int capture_screen(CaptureDevice* device);

/**
 * @brief                          Transfers screen capture to CPU buffer
 *                                 This is used if all attempts of a
 *                                 hardware screen transfer have failed
 *
 * @param device                   The capture device used to capture the screen
 *
 * @returns                        0 always
 */
int transfer_screen(CaptureDevice* device);

/**
 * @brief                          Destroys and frees the memory of a capture device
 *
 * @param device                   The capture device to free
 */
void destroy_capture_device(CaptureDevice* device);

/**
 * Set filename used by the file-capture test device.
 *
 * TODO: this should be passed via create_capture_device() rather than
 * separately like this.
 *
 * @param filename  Filename to use the next time the file-capture
 *                  device is opened.  Must be a static string.
 */
void file_capture_set_input_filename(const char* filename);

/**
 * @brief Get the current active window
 * 
 * @returns A WhistWindow struct with the current window
 */
WhistWindow get_active_window(void);

bool device_has_window(CaptureDevice* device, WhistWindow window);

#endif  // VIDEO_CAPTURE_H
