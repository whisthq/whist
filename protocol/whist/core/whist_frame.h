#ifndef WHIST_FRAME_H
#define WHIST_FRAME_H

#include "whist.h"
#include <whist/cursor/cursor.h>
#include <whist/utils/color.h>
#include "whist/utils/buffer.h"
#include "whist/video/video.h"

/**
 * @brief   VideoFrame struct.
 * @details This contains all of the various types of data needed for a single frame to be rendered.
 *          This includes:
 *              - The videodata buffer consisting of compressed h264 videodata
 *              - The new cursor image if the cursor has just changed
 */
typedef struct VideoFrame {
    int width;
    int height;
    CodecType codec_type;
    VideoFrameType frame_type;
    uint32_t frame_id;

    // TODO: decide if we want to cap the number of windows or not
    WhistWindow window_data[MAX_WINDOWS];

    /**
     * Indicates that the cursor changes on this frame.
     *
     * Cursor information follows, which will be one of:
     * - A cursor ID, to use a standard system cursor.
     * - A hash, to reuse a previously-cached cursor.
     * - A complete PNG image to use as the new cursor.
     */
    bool has_cursor;
    bool is_empty_frame;     // indicates whether this frame is identical to the one last sent
    bool is_window_visible;  // indicates whether the client app is visible. If the client realizes
                             // the server is wrong, it can correct it
    WhistRGBColor corner_color;
    timestamp_us client_input_timestamp;  // Last ping client timestamp + time elapsed. Used for
                                          // E2E latency calculation.
    timestamp_us server_timestamp;        // Server timestamp during capture of this frame

    size_t data_length;
    uint8_t *data;
} VideoFrame;

typedef struct AudioFrame {
    int audio_frequency;
    int data_length;
    unsigned char data[];
} AudioFrame;

// The maximum possible valid size of a VideoFrame*
// It will be guaranteed that no valid VideoFrame will be larger than this.
// Any videoframes larger will be unconditionally dropped and error reported
#define LARGEST_VIDEOFRAME_SIZE 4000000
// The maximum possible valid size of an audio frame: a little more than 8192 bytes, which is the
// frame size of the decoded data
#define LARGEST_AUDIOFRAME_SIZE 9000

// The maximum frame size, excluding the embedded videodata
#define MAX_VIDEOFRAME_METADATA_SIZE (sizeof(VideoFrame) + sizeof(WhistCursorInfo))

// The maximum allowed videodata size that can be embedded in a VideoFrame*
// Setting frame->videodata_length to anything larger than this is invalid and will cause bugs
#define MAX_VIDEOFRAME_DATA_SIZE (LARGEST_VIDEOFRAME_SIZE - MAX_VIDEOFRAME_METADATA_SIZE)

// The maximum frame size, excluding the embedded videodata
#define MAX_AUDIOFRAME_METADATA_SIZE sizeof(AudioFrame)

// The maximum allowed videodata size that can be embedded in a VideoFrame*
// Setting frame->videodata_length to anything larger than this is invalid and will cause problems
#define MAX_AUDIOFRAME_DATA_SIZE (LARGEST_AUDIOFRAME_SIZE - MAX_AUDIOFRAME_METADATA_SIZE)

/**
 * Write a frame header to a buffer.
 *
 * @param wb     Buffer to write to.
 * @param frame  Frame header structure to write.
 */
void whist_write_frame_header(WhistWriteBuffer *wb, const VideoFrame *frame);

/**
 * Read a frame header from a buffer.
 *
 * @param rb     Buffer to read from.
 * @param frame  Frame header structure to fill.
 */
void whist_read_frame_header(WhistReadBuffer *rb, VideoFrame *frame);

/**
 * Get the frame type of a video frame in serialised form.
 *
 * This is provided so that the network code can see whether a frame is
 * a recovery point or not without reading the whole header.
 *
 * TODO: it would probably be better to mark recovery points at a higher
 * level to avoid this.
 *
 * @param data  Buffer containing the frame.
 * @param size  Size of the buffer.
 * @return  The frame type.
 */
VideoFrameType whist_get_video_frame_type(const uint8_t *data, size_t size);

/**
 * Write cursor metadata to a buffer.
 *
 * @param wb      Buffer to write to.
 * @param cursor  Cursor info to write.
 */
void whist_write_cursor(WhistWriteBuffer *wb, const WhistCursorInfo *cursor);

/**
 * Read cursor metadata from a buffer.
 *
 * @param rb      Buffer to read from.
 * @param cursor  Cursor info to fill.
 */
void whist_read_cursor(WhistReadBuffer *rb, WhistCursorInfo *cursor);

#endif
