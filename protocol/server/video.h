#ifndef SERVER_VIDEO_H
#define SERVER_VIDEO_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file video.h
 * @brief This file contains all code that interacts directly with processing
 *        video on the server.
============================
Usage
============================

send_video() is called on its own thread and loops repeatedly to capture and send video.
*/

/*
============================
Public Functions
============================
*/

/**
 * @brief                          This loops and captures video frames, encodes
 *                                 them if needed, and sends them to the client
 *                                 along with cursor images if necessary
 */
int32_t send_video(void* opaque);

#endif  // SERVER_VIDEO_H
