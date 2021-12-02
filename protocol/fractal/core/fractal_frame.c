#include "fractal_frame.h"

void set_frame_cursor_image(VideoFrame* frame, WhistCursorImage* cursor) {
    /*
        Sets the fractal frame's cursor image

        Arguments:
            frame (Frame*): The frame who's data buffer should be written to
            cursor (WhistCursorImage*): The WhistCursorImage who's cursor data
                should be embedded in the given frame. Pass NULL to embed no cursor
                whatsoever. Default of a 0'ed VideoFrame* is already a NULL cursor.
    */

    if (cursor == NULL) {
        frame->has_cursor = false;
    } else {
        frame->has_cursor = true;
        memcpy(frame->data, cursor, sizeof(WhistCursorImage));
    }
}

WhistCursorImage* get_frame_cursor_image(VideoFrame* frame) {
    /*
        Get a pointer to the WhistCursorImage inside of the VideoFrame*

        Arguments:
            frame (Frame*): The VideoFrame who's data buffer is being used

        Returns:
            (WhistCursorImage*): A pointer to the internal WhistCursorImage.
                May return NULL if no cursor was embedded.
    */

    if (frame->has_cursor) {
        return (WhistCursorImage*)frame->data;
    } else {
        return NULL;
    }
}

unsigned char* get_frame_videodata(VideoFrame* frame) {
    /*
        Get a pointer to the videodata inside of the VideoFrame*
        Prerequisites for writing to the returned buffer pointer:
            frame->videodata_length must be set
            set_frame_cursor_image must be called

        NOTE: Please only read/write up to frame->videodata_length bytes from
              the returned buffer

        Arguments:
            frame (Frame*): The VideoFrame that contains the videodata buffer

        Returns:
            (unsigned char*): The videodata buffer that is stored in this VideoFrame
    */

    unsigned char* ret = frame->data;
    if (frame->has_cursor) {
        ret += sizeof(WhistCursorImage);
    }
    return ret;
}

int get_total_frame_size(VideoFrame* frame) {
    /*
        Get the total VideoFrame size, including all of the data embedded in
        the VideoFrame's buffer.

        Arguments:
            frame (Frame*): The VideoFrame whose size we are getting

        Returns:
            (int): The number of bytes that the frame uses up.
    */

    int ret = frame->videodata_length;
    if (frame->has_cursor) {
        ret += sizeof(WhistCursorImage);
    }
    return sizeof(VideoFrame) + ret;
}
