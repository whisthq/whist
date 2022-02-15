#include "whist_frame.h"

void set_frame_cursor_info(VideoFrame* frame, WhistCursorInfo* cursor) {
    if (cursor == NULL) {
        frame->has_cursor = false;
    } else {
        frame->has_cursor = true;
        memcpy(frame->data, cursor, get_cursor_info_size(cursor));
    }
}

WhistCursorInfo* get_frame_cursor_info(VideoFrame* frame) {
    if (frame->has_cursor) {
        return (WhistCursorInfo*)frame->data;
    } else {
        return NULL;
    }
}

unsigned char* get_frame_videodata(VideoFrame* frame) {
    return frame->data + get_cursor_info_size(get_frame_cursor_info(frame));
}

int get_total_frame_size(VideoFrame* frame) {
    return (int)(sizeof(VideoFrame) + get_cursor_info_size(get_frame_cursor_info(frame)) +
                 frame->videodata_length);
}
