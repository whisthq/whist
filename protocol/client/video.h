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

NOTE: Only ONE thread can call a function on video_context at a time,
      unless a function says otherwise.

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

In another thread: (NOTE: ONLY render_video is thread-safe. The rest of the functions are not)
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

typedef struct VideoContext VideoContext;

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
VideoContext* init_video(int initial_width, int initial_height);

/**
 * @brief                          Receive video packet
 *
 * @param video_context            The video context to give a video packet to
 *
 * @param video_frame              The video frame
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 */
void receive_video(VideoContext* video_context, VideoFrame* video_frame);

/**
 * @brief                          Render the video frame (If any are available to render)
 *
 * @param video_context            The video context that wants to render a frame
 *
 * @note                           This function is thread-safe, and may be called in a way
 *                                 that overlaps other functions that use the vide context.
 */
int render_video(VideoContext* video_context);

/**
 * @brief                          Returns whether or not a video frame has been rendered.
 *                                 (NOT including any "loading animation" frames)
 *                                 This can be false even after calling render_video,
 *                                 if there simply weren't enough packets yet or if the video
 *                                 is still being decoded.
 *
 *
 * @param video_context            The video context that has potentially rendered a frame
 *
 * @returns                        Returns true iff a video frame has actually
 *                                 been rendered out to the SDL framebuffer.
 *                                 (NOT including any "loading animation" frames)
 */
bool has_video_rendered_yet(VideoContext* video_context);

/**
 * @brief                          Destroy the video context
 *
 * @param video_context            The video context to destroy
 */
void destroy_video(VideoContext* video_context);

/**
 * @brief          Whether or not the video context is ready to render a new frame
 *
 * @param video_context       The video context to query
 */
bool video_ready_for_frame(VideoContext* video_context);

#endif  // CLIENT_VIDEO_H
