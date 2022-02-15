#ifndef WHIST_FRAME_H
#define WHIST_FRAME_H

#include "whist.h"
#include <whist/cursor/cursor.h>
#include <whist/utils/color.h>

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
    bool is_iframe;

    bool has_cursor;
    bool is_empty_frame;     // indicates whether this frame is identical to the one last sent
    bool is_window_visible;  // indicates whether the client app is visible. If the client realizes
                             // the server is wrong, it can correct it
    int videodata_length;
    WhistRGBColor corner_color;
    timestamp_us client_input_timestamp;  // Last ping client timestamp + time elapsed. Used for
                                          // E2E latency calculation.
    timestamp_us server_timestamp;        // Server timestamp during capture of this frame

    unsigned char data[];
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
 * @brief                          Sets the whist frame's cursor image
 *
 * @param frame                    The frame who's data buffer should be written to
 *
 * @param cursor                   The WhistCursorInfo who's cursor data should be embedded in
 *                                 the given frame. Pass NULL to embed no cursor whatsoever.
 *                                 Default of a 0'ed VideoFrame* is already a NULL cursor.
 */
void set_frame_cursor_info(VideoFrame* frame, WhistCursorInfo* cursor);

/**
 * @brief                          Get a pointer to the WhistCursorInfo inside of the VideoFrame*
 *
 * @param frame                    The VideoFrame who's data buffer is being used
 *
 * @returns                        A pointer to the internal WhistCursorInfo. May return NULL if
 *                                 no cursor was embedded.
 */
WhistCursorInfo* get_frame_cursor_info(VideoFrame* frame);

/**
 * @brief                          Get a pointer to the videodata inside of the VideoFrame*
 *                                 Prerequisites for writing to the returned buffer pointer:
 *                                     frame->videodata_length must be set
 *                                     set_frame_cursor_info must be called
 *                                 Please only read/write up to frame->videodata_length bytes from
 *                                 the returned buffer
 *
 * @param frame                    The VideoFrame that contains the videodata buffer
 *
 * @returns                        The videodata buffer that is stored in this VideoFrame
 */
unsigned char* get_frame_videodata(VideoFrame* frame);

/**
 * @brief                          Get the total VideoFrame size, including all of the data embedded
 * in the VideoFrame's buffer. Even if the VideoFrame* is being stored in a much larger buffer, this
 * function returns only the number of bytes needed for the data inside the VideoFrame* to be read
 * correctly. I.e., these are the only bytes that need to be sent over for example a network
 * connection.
 *
 * @returns                        The number of bytes that the frame uses up.
 */
int get_total_frame_size(VideoFrame* frame);

#endif
