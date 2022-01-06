#ifndef CLIENT_VIDEO_H
#define CLIENT_VIDEO_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file video.h
 * @brief This file contains all code that interacts directly with receiving and
 *        processing video packets on the client.
============================
Usage
============================

VideoContext* video_context = init_video();

In one thread:
update_video(video_context);
receive_video(video_context, packet_1);
update_video(video_context);
update_video(video_context);
receive_video(video_context, packet_2);
update_video(video_context);
update_video(video_context);
receive_video(video_context, packet_3);
update_video(video_context);
update_video(video_context);

In another thread:
render_video(video_context);
render_video(video_context);
render_video(video_context);
render_video(video_context);

// ~~
// Join both threads..

destroy_video(video_context);
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>

/*
============================
Defines
============================
*/

#ifndef RENDERING_IN_VIRTUAL_ENVIRONMENT
#define RENDERING_IN_VIRTUAL_ENVIRONMENT 0
#endif

typedef void VideoContext;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          This will initialize the video system.
 *                                 The video system will receive video packets,
 *                                 and render the video out to the window
 *
 * @returns                        The new video context
 */
VideoContext* init_video();

/**
 * @brief                          Receive video packet
 *
 * @param video_context            The video context to give a video packet to
 *
 * @param packet                   Packet as received from the server
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 */
void receive_video(VideoContext* video_context, WhistPacket* packet);

/**
 * @brief                          Does any pending work the video context
 *                                 wants to do. (Including decoding frames,
 *                                 and calculating statistics)
 *
 * @param video_context            The video context to update
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 *
 * @note                           In order for video to be responsive,
 *                                 this function *MUST* be called in a tight loop,
 *                                 at least once every millisecond.
 */
void update_video(VideoContext* video_context);

/**
 * @brief                          Render the video frame (If any are available to render)
 *
 * @param video_context            The video context that wants to render a frame
 *
 * @note                           This function is thread-safe, and may be called
 *                                 independently of receive_video/update_video
 */
int render_video(VideoContext* video_context);

/**
 * @brief                          Destroy the video context
 *
 * @param video_context            The video context to destroy
 */
void destroy_video(VideoContext* video_context);

#endif  // CLIENT_VIDEO_H
