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
    unsigned char* data = frame->data;
    if (frame->has_cursor) {
        data += get_cursor_info_size(get_frame_cursor_info(frame));
    }
    return data;
}

int get_total_frame_size(VideoFrame* frame) {
    // Size of static content
    size_t size = sizeof(VideoFrame);
    // Size of dynamic content
    if (frame->has_cursor) {
        size += get_cursor_info_size(get_frame_cursor_info(frame));
    }
    size += frame->videodata_length;
    return (int)size;
}
