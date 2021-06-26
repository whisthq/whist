#include "fractal_frame.h"

void set_frame_cursor_image(Frame* frame, FractalCursorImage* cursor) {
    /*
        Sets the fractal frame's cursor image

        Arguments:
            frame (Frame*): The frame who's data buffer should be written to
            cursor (FractalCursorImage*): The FractalCursorImage who's cursor data
                should be embedded in the given frame. Pass NULL to embed no cursor
                whatsoever. Default of a 0'ed Frame* is already a NULL cursor.
    */

    if (cursor == NULL) {
        frame->has_cursor = false;
    } else {
        frame->has_cursor = true;
        memcpy(frame->data, cursor, sizeof(FractalCursorImage));
    }
}

FractalCursorImage* get_frame_cursor_image(Frame* frame) {
    /*
        Get a pointer to the FractalCursorImage inside of the Frame*

        Arguments:
            frame (Frame*): The Frame who's data buffer is being used

        Returns:
            (FractalCursorImage*): A pointer to the internal FractalCursorImage.
                May return NULL if no cursor was embedded.
    */

    if (frame->has_cursor) {
        return (FractalCursorImage*)frame->data;
    } else {
        return NULL;
    }
}

unsigned char* get_frame_videodata(Frame* frame) {
    /*
        Get a pointer to the videodata inside of the Frame*
        Prerequisites for writing to the returned buffer pointer:
            frame->videodata_length must be set
            set_frame_cursor_image must be called

        NOTE: Please only read/write up to frame->videodata_length bytes from
              the returned buffer

        Arguments:
            frame (Frame*): The Frame that contains the videodata buffer

        Returns:
            (unsigned char*): The videodata buffer that is stored in this Frame
    */

    unsigned char* ret = frame->data;
    if (frame->has_cursor) {
        ret += sizeof(FractalCursorImage);
    }
    return ret;
}

PeerUpdateMessage* get_frame_peer_messages(Frame* frame) {
    /*
        Get a pointer to the PeerUpdateMessage inside of the Frame*

        NOTES: Prerequisites for writing to the returned PeerUpdateMessage
        pointer: frame->videodata_length must be set set_frame_cursor_image
        must be called. After writing to the returned PeerUpdateMessage*,
        please update frame->num_peer_update_msgs.

        Arguments:
            frame (Frame*): The Frame who's data buffer is being used

        Returns:
            (PeerUpdateMessage*): A pointer to the internal PeerUpdateMessage buffer
    */

    return (PeerUpdateMessage*)(get_frame_videodata(frame) + frame->videodata_length);
}

int get_total_frame_size(Frame* frame) {
    /*
        Get the total Frame size, including all of the data embedded in
        the Frame's buffer.

        Arguments:
            frame (Frame*): The Frame whose size we are getting

        Returns:
            (int): The number of bytes that the frame uses up.
    */

    int ret = frame->videodata_length;
    if (frame->has_cursor) {
        ret += sizeof(FractalCursorImage);
    }
    return sizeof(Frame) + ret + sizeof(PeerUpdateMessage) * frame->num_peer_update_msgs;
}
