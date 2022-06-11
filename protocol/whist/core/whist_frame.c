/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file whist_frame.c
 * @brief Serialisation functions for video frames and associated metadata.
 */

#include "whist/utils/buffer.h"
#include "whist_frame.h"

static void whist_write_color(WhistWriteBuffer *wb, const WhistRGBColor *color) {
    whist_write_1(wb, color->red);
    whist_write_1(wb, color->green);
    whist_write_1(wb, color->blue);
}

static void whist_read_color(WhistReadBuffer *rb, WhistRGBColor *color) {
    color->red = whist_read_1(rb);
    color->green = whist_read_1(rb);
    color->blue = whist_read_1(rb);
}

enum {
    WINDOW_FLAG_IS_FULLSCREEN = 0,
};

static void whist_write_window(WhistWriteBuffer *wb, const WhistWindow *window) {
    whist_write_leb128(wb, window->id);
    whist_write_2(wb, window->x);
    whist_write_2(wb, window->y);
    whist_write_2(wb, window->width);
    whist_write_2(wb, window->height);
    whist_write_color(wb, &window->corner_color);

    uint8_t flags = (window->is_fullscreen << WINDOW_FLAG_IS_FULLSCREEN);
    whist_write_1(wb, flags);
}

static void whist_read_window(WhistReadBuffer *rb, WhistWindow *window) {
    window->id = whist_read_leb128(rb);
    window->x = whist_read_2(rb);
    window->y = whist_read_2(rb);
    window->width = whist_read_2(rb);
    window->height = whist_read_2(rb);
    whist_read_color(rb, &window->corner_color);

    uint8_t flags = whist_read_1(rb);
    window->is_fullscreen = (flags >> WINDOW_FLAG_IS_FULLSCREEN) & 1;
}

enum {
    FRAME_FLAG_HAS_CURSOR = 0,
    FRAME_FLAG_IS_EMPTY_FRAME,
    FRAME_FLAG_IS_WINDOW_VISIBLE,
};

void whist_write_frame_header(WhistWriteBuffer *wb, const VideoFrame *frame) {
    uint8_t flags = (frame->has_cursor << FRAME_FLAG_HAS_CURSOR |
                     frame->is_empty_frame << FRAME_FLAG_IS_EMPTY_FRAME |
                     frame->is_window_visible << FRAME_FLAG_IS_WINDOW_VISIBLE);
    whist_write_1(wb, flags);

    if (frame->is_empty_frame) {
        // An empty frame doesn't contain anything else.
        return;
    }

    // Frame type is placed early in this function so we can easily find
    // it in whist_get_video_frame_type().
    whist_write_1(wb, frame->frame_type);

    whist_write_2(wb, frame->width);
    whist_write_2(wb, frame->height);
    whist_write_2(wb, frame->codec_type);
    whist_write_4(wb, frame->frame_id);

    // Count how many windows are actually present, then write only
    // those ones.
    int windows = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (frame->window_data[i].id != (uint64_t)-1) {
            ++windows;
        }
    }
    whist_write_1(wb, windows);
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (frame->window_data[i].id == (uint64_t)-1) {
            continue;
        }
        whist_write_window(wb, &frame->window_data[i]);
    }

    whist_write_color(wb, &frame->corner_color);
    whist_write_8(wb, frame->client_input_timestamp);
    whist_write_8(wb, frame->server_timestamp);
}

void whist_read_frame_header(WhistReadBuffer *rb, VideoFrame *frame) {
    uint8_t flags = whist_read_1(rb);
    frame->has_cursor = (flags >> FRAME_FLAG_HAS_CURSOR) & 1;
    frame->is_empty_frame = (flags >> FRAME_FLAG_IS_EMPTY_FRAME) & 1;
    frame->is_window_visible = (flags >> FRAME_FLAG_IS_WINDOW_VISIBLE) & 1;

    if (frame->is_empty_frame) {
        return;
    }

    frame->frame_type = whist_read_1(rb);

    frame->width = whist_read_2(rb);
    frame->height = whist_read_2(rb);
    frame->codec_type = whist_read_2(rb);
    frame->frame_id = whist_read_4(rb);

    int windows = whist_read_1(rb);
    FATAL_ASSERT(windows <= MAX_WINDOWS);
    int w;
    for (w = 0; w < windows; w++) {
        whist_read_window(rb, &frame->window_data[w]);
    }
    for (; w < MAX_WINDOWS; w++) {
        frame->window_data[w].id = (uint64_t)-1;
    }

    whist_read_color(rb, &frame->corner_color);
    frame->client_input_timestamp = whist_read_8(rb);
    frame->server_timestamp = whist_read_8(rb);
}

VideoFrameType whist_get_video_frame_type(const uint8_t *data, size_t size) {
    FATAL_ASSERT(size >= 1);
    if (size == 1 || ((data[0] >> FRAME_FLAG_IS_EMPTY_FRAME) & 1)) {
        // Empty frame, treat as if normal frame.
        return VIDEO_FRAME_TYPE_NORMAL;
    } else {
        // Frame type is the next byte.
        return data[1];
    }
}

void whist_write_cursor(WhistWriteBuffer *wb, const WhistCursorInfo *cursor) {
    whist_write_1(wb, cursor->cursor_state);
    whist_write_4(wb, cursor->hash);
    whist_write_1(wb, cursor->using_png);
    if (cursor->using_png) {
        whist_write_2(wb, cursor->png_width);
        whist_write_2(wb, cursor->png_height);
        whist_write_2(wb, cursor->png_hot_x);
        whist_write_2(wb, cursor->png_hot_y);
        whist_write_leb128(wb, cursor->png_size);
    }
}

void whist_read_cursor(WhistReadBuffer *rb, WhistCursorInfo *cursor) {
    cursor->cursor_state = whist_read_1(rb);
    cursor->hash = whist_read_4(rb);
    cursor->using_png = whist_read_1(rb);
    if (cursor->using_png) {
        cursor->png_width = whist_read_2(rb);
        cursor->png_height = whist_read_2(rb);
        cursor->png_hot_x = whist_read_2(rb);
        cursor->png_hot_y = whist_read_2(rb);
        cursor->png_size = whist_read_leb128(rb);
    }
}
