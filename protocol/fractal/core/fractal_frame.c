#include "fractal_frame.h"

void set_frame_cursor_image(Frame* frame, FractalCursorImage* cursor) {
    if (cursor == NULL) {
        frame->has_cursor = false;
    } else {
        frame->has_cursor = true;
        memcpy(frame->data, cursor, sizeof(FractalCursorImage));
    }
}

FractalCursorImage* get_frame_cursor_image(Frame* frame) {
    if (frame->has_cursor) {
        return (FractalCursorImage*)frame->data;
    } else {
        return NULL;
    }
}

unsigned char* get_frame_videodata(Frame* frame) {
    unsigned char* ret = frame->data;
    if (frame->has_cursor) {
        ret += sizeof(FractalCursorImage);
    }
    return ret;
}

PeerUpdateMessage* get_frame_peer_messages(Frame* frame) {
    return (PeerUpdateMessage*)(get_frame_videodata(frame) + frame->videodata_length);
}

int get_total_frame_size(Frame* frame) {
    int ret = frame->videodata_length;
    if (frame->has_cursor) {
        ret += sizeof(FractalCursorImage);
    }
    return sizeof(Frame) + ret + sizeof(PeerUpdateMessage) * frame->num_peer_update_msgs;
}
