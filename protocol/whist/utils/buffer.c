/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file buffer.h
 * @brief Implementation of data serialisation with read/write buffers.
 */

#include <stdbool.h>
#include <string.h>
#include <libavutil/intreadwrite.h>

#include "buffer.h"

void whist_write_buffer_init(WhistWriteBuffer *wb, uint8_t *buf, size_t size) {
    wb->buf = buf;
    wb->current = 0;
    wb->size = size;
    wb->overflow = false;
}

size_t whist_write_buffer_bytes_written(WhistWriteBuffer *wb) { return wb->current; }

bool whist_write_buffer_has_overflowed(WhistWriteBuffer *wb) { return wb->overflow; }

void whist_write_1(WhistWriteBuffer *wb, uint8_t value) {
    if (wb->current < wb->size) {
        wb->buf[wb->current++] = value;
    } else {
        wb->overflow = true;
    }
}

void whist_write_2(WhistWriteBuffer *wb, uint16_t value) {
    if (wb->current <= wb->size - 2) {
        AV_WL16(&wb->buf[wb->current], value);
        wb->current += 2;
    } else {
        wb->overflow = true;
    }
}

void whist_write_3(WhistWriteBuffer *wb, uint32_t value) {
    if (wb->current <= wb->size - 3) {
        AV_WL24(&wb->buf[wb->current], value);
        wb->current += 3;
    } else {
        wb->overflow = true;
    }
}

void whist_write_4(WhistWriteBuffer *wb, uint32_t value) {
    if (wb->current <= wb->size - 4) {
        AV_WL32(&wb->buf[wb->current], value);
        wb->current += 4;
    } else {
        wb->overflow = true;
    }
}

void whist_write_8(WhistWriteBuffer *wb, uint64_t value) {
    if (wb->current <= wb->size - 8) {
        AV_WL64(&wb->buf[wb->current], value);
        wb->current += 8;
    } else {
        wb->overflow = true;
    }
}

void whist_write_leb128(WhistWriteBuffer *wb, uint64_t value) {
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value) {
            byte |= 0x80;
        }
        whist_write_1(wb, byte);
    } while (value);
}

void whist_write_data(WhistWriteBuffer *wb, const uint8_t *data, size_t size) {
    size_t size_left = wb->size - wb->current;
    if (size <= size_left) {
        memcpy(&wb->buf[wb->current], data, size);
        wb->current += size;
    } else {
        wb->overflow = true;
    }
}

void whist_read_buffer_init(WhistReadBuffer *rb, const uint8_t *mem, size_t size) {
    rb->buf = mem;
    rb->current = 0;
    rb->size = size;
    rb->underflow = false;
}

bool whist_read_buffer_has_underflowed(WhistReadBuffer *rb) { return rb->underflow; }

void whist_read_buffer_skip(WhistReadBuffer *rb, size_t skip) {
    if (rb->current + skip <= rb->size) {
        rb->current += skip;
    } else {
        rb->current = rb->size;
        rb->underflow = true;
    }
}

const uint8_t *whist_read_buffer_get_pointer(WhistReadBuffer *rb) { return &rb->buf[rb->current]; }

uint8_t whist_read_1(WhistReadBuffer *rb) {
    if (rb->current < rb->size) {
        return rb->buf[rb->current++];
    } else {
        rb->underflow = true;
        return 0;
    }
}

uint16_t whist_read_2(WhistReadBuffer *rb) {
    if (rb->current <= rb->size - 2) {
        uint16_t value = AV_RL16(&rb->buf[rb->current]);
        rb->current += 2;
        return value;
    } else {
        rb->underflow = true;
        return 0;
    }
}

uint32_t whist_read_3(WhistReadBuffer *rb) {
    if (rb->current <= rb->size - 3) {
        uint32_t value = AV_RL24(&rb->buf[rb->current]);
        rb->current += 3;
        return value;
    } else {
        rb->underflow = true;
        return 0;
    }
}

uint32_t whist_read_4(WhistReadBuffer *rb) {
    if (rb->current <= rb->size - 4) {
        uint32_t value = AV_RL32(&rb->buf[rb->current]);
        rb->current += 4;
        return value;
    } else {
        rb->underflow = true;
        return 0;
    }
}

uint64_t whist_read_8(WhistReadBuffer *rb) {
    if (rb->current <= rb->size - 8) {
        uint64_t value = AV_RL64(&rb->buf[rb->current]);
        rb->current += 8;
        return value;
    } else {
        rb->underflow = true;
        return 0;
    }
}

uint64_t whist_read_leb128(WhistReadBuffer *rb) {
    uint64_t value = 0;
    int shift = 0;
    uint8_t byte;
    do {
        byte = whist_read_1(rb);
        value |= (uint64_t)(byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);
    return value;
}

void whist_read_data(WhistReadBuffer *rb, uint8_t *data, size_t size) {
    size_t size_left = rb->size - rb->current;
    if (size <= size_left) {
        memcpy(data, &rb->buf[rb->current], size);
        rb->current += size;
    } else {
        memset(data, 0, size);
        rb->underflow = true;
    }
}
