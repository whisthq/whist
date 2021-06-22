#ifndef FRACTAL_FRAME_H
#define FRACTAL_FRAME_H

#include "fractal.h"

/**
 * @brief   VideoFrame struct.
 * @details This contains all of the various types of data needed for a single frame to be rendered.
 *          This includes:
 *              - The videodata buffer consisting of compressed h264 videodata
 *              - The new cursor image if the cursor has just changed
 *              - The PeerUpdateMessage's that are a part of this frame, for multi-cursor support
 */
typedef struct VideoFrame {
    int width;
    int height;
    CodecType codec_type;
    bool is_iframe;
    int num_peer_update_msgs;

    bool has_cursor;
    int videodata_length;

    unsigned char data[];
} VideoFrame;

typedef struct AudioFrame {
    int data_length;
    unsigned char data[];
} AudioFrame;

// The maximum possible valid size of a VideoFrame*
// It is guaranteed that no valid VideoFrame will be larger than this,
// since all valid frames will have a videodata_length less than MAX_FRAME_VIDEODATA_SIZE
#define LARGEST_FRAME_SIZE 1000000

// The maximum frame size, excluding the embedded videodata
#define MAX_FRAME_METADATA_SIZE \
    (sizeof(VideoFrame) + sizeof(FractalCursorImage) + sizeof(PeerUpdateMessage) * MAX_NUM_CLIENTS)

// The maximum allowed videodata size that can be embedded in a VideoFrame*
// Setting frame->videodata_length to anything larger than this is invalid and will cause problems
#define MAX_FRAME_VIDEODATA_SIZE (LARGEST_FRAME_SIZE - MAX_FRAME_METADATA_SIZE)

/**
 * @brief                          Sets the fractal frame's cursor image
 *
 * @param frame                    The frame who's data buffer should be written to
 *
 * @param cursor                   The FractalCursorImage who's cursor data should be embedded in
 *                                 the given frame. Pass NULL to embed no cursor whatsoever.
 *                                 Default of a 0'ed VideoFrame* is already a NULL cursor.
 */
void set_frame_cursor_image(VideoFrame* frame, FractalCursorImage* cursor);

/**
 * @brief                          Get a pointer to the FractalCursorImage inside of the VideoFrame*
 *
 * @param frame                    The VideoFrame who's data buffer is being used
 *
 * @returns                        A pointer to the internal FractalCursorImage. May return NULL if
 *                                 no cursor was embedded.
 */
FractalCursorImage* get_frame_cursor_image(VideoFrame* frame);

/**
 * @brief                          Get a pointer to the videodata inside of the VideoFrame*
 *                                 Prerequisites for writing to the returned buffer pointer:
 *                                     frame->videodata_length must be set
 *                                     set_frame_cursor_image must be called
 *                                 Please only read/write up to frame->videodata_length bytes from
 *                                 the returned buffer
 *
 * @param frame                    The VideoFrame that contains the videodata buffer
 *
 * @returns                        The videodata buffer that is stored in this VideoFrame
 */
unsigned char* get_frame_videodata(VideoFrame* frame);

/**
 * @brief                          Get a pointer to the PeerUpdateMessage inside of the VideoFrame*
 *                                 Prerequisites for writing to the returned PeerUpdateMessage
 *                                 pointer: frame->videodata_length must be set
 *                                 set_frame_cursor_image must be called. After writing to
 *                                 the returned PeerUpdateMessage*, please update
 *                                 frame->num_peer_update_msgs
 *
 * @param frame                    The VideoFrame who's data buffer is being used
 *
 * @returns                        A pointer to the internal PeerUpdateMessage buffer
 */
PeerUpdateMessage* get_frame_peer_messages(VideoFrame* frame);

/**
 * @brief                          Get the total VideoFrame size, including all of the data embedded in
 *                                 the VideoFrame's buffer. Even if the VideoFrame* is being stored in a much larger buffer, this function
 *                                 returns only the number of bytes needed for the data inside the VideoFrame* to be read correctly.
 *                                 I.e., these are the only bytes that need to be sent over for example a network connection.
 *
 * @returns                        The number of bytes that the frame uses up.
 */
int get_total_frame_size(VideoFrame* frame);

#endif
