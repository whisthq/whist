/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file video.h
 * @brief Definitions common to all video elements.
 */
#ifndef WHIST_CODEC_VIDEO_H
#define WHIST_CODEC_VIDEO_H

typedef enum {
    /**
     * A normal frame.
     *
     * It requires the previous frame to have been decoded correctly to
     * decode it.
     */
    VIDEO_FRAME_TYPE_NORMAL,
    /**
     * An intra frame.
     *
     * It can be decoded without needing any previous data from the
     * stream, and will includes parameter sets if necessary.  If
     * long-term referencing is enabled, it will also be placed in the
     * first long-term reference slot and all others will be cleared.
     */
    VIDEO_FRAME_TYPE_INTRA,
    /**
     * A frame creating a long term reference.
     *
     * Like a normal frame it requires the previous frame to have been
     * decoded correctly to decode, but it is then also placed in a
     * long-term reference slot to possibly be used by future frames.
     */
    VIDEO_FRAME_TYPE_CREATE_LONG_TERM,
    /**
     * A frame using a long term reference.
     *
     * This requires a specific long term frame to have been previously
     * decoded correctly in order to decode it.
     */
    VIDEO_FRAME_TYPE_REFER_LONG_TERM,
} VideoFrameType;

/**
 * True if the given frame type can be used as a recovery point.
 */
#define VIDEO_FRAME_TYPE_IS_RECOVERY_POINT(type) \
    ((type) == VIDEO_FRAME_TYPE_INTRA || (type) == VIDEO_FRAME_TYPE_REFER_LONG_TERM)

/**
 * Return string representation of a frame type, for log messages.
 *
 * @param type  Frame type.
 * @return      Pointer to a static string representing the frame type.
 */
const char *video_frame_type_string(VideoFrameType type);

#endif /* WHIST_CODEC_VIDEO_H */
