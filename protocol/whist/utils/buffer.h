/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file buffer.h
 * @brief API for data serialisation with read/write buffers.
 */
#ifndef WHIST_UTILS_BUFFER_H
#define WHIST_UTILS_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Write buffer object.
 *
 * @private
 * This should be treated as opaque.
 */
typedef struct WhistWriteBuffer {
    /** The underlying buffer. */
    uint8_t *buf;
    /** The current write location in the buffer. */
    size_t current;
    /** Size of the buffer. */
    size_t size;
    /** Flag set if the buffer overflows (write over the end). */
    bool overflow;
} WhistWriteBuffer;

/**
 * Initialise a new write buffer.
 *
 * @param wb    Write buffer to initialise.
 * @param buf   Memory buffer to write to.
 * @param size  Size of the buffer.
 */
void whist_write_buffer_init(WhistWriteBuffer *wb, uint8_t *buf, size_t size);

/**
 * Get the number of bytes written to a buffer.
 *
 * @param wb  Write buffer to examine.
 * @return  Number of bytes written to the buffer.
 */
size_t whist_write_buffer_bytes_written(WhistWriteBuffer *wb);

/**
 * Test whether a write buffer has overflowed.
 *
 * @param wb  Write buffer to test.
 * @return  Whether the buffer has overflowed.
 */
bool whist_write_buffer_has_overflowed(WhistWriteBuffer *wb);

/**
 * Write a 1, 2, 3, 4 or 8-byte value to a buffer.
 *
 * @param wb     Buffer to write to.
 * @param value  Value to write.
 */
void whist_write_1(WhistWriteBuffer *wb, uint8_t value);
void whist_write_2(WhistWriteBuffer *wb, uint16_t value);
void whist_write_3(WhistWriteBuffer *wb, uint32_t value);
void whist_write_4(WhistWriteBuffer *wb, uint32_t value);
void whist_write_8(WhistWriteBuffer *wb, uint64_t value);

/**
 * Write an LEB128-encoded value to a buffer.
 *
 * This is suitable for values which are normally small but sometimes
 * large, to avoid wasting higher bytes which are almost always zero.
 *
 * @param wb     Buffer to write to.
 * @param value  Value to write.
 */
void whist_write_leb128(WhistWriteBuffer *wb, uint64_t value);

/**
 * Write a block of binary data to a buffer.
 *
 * This is memcpy() in buffer form.
 *
 * @param wb    Buffer to write to.
 * @param data  Data to write.
 * @param size  Number of bytes to write.
 */
void whist_write_data(WhistWriteBuffer *wb, const uint8_t *data, size_t size);

typedef struct WhistReadBuffer {
    /** The underlying buffer. */
    const uint8_t *buf;
    /** The current read location in the buffer. */
    size_t current;
    /** Size of the buffer. */
    size_t size;
    /** Flag set if the buffer underflows (read over the end). */
    bool underflow;
} WhistReadBuffer;

/**
 * Initialise a new read buffer.
 *
 * @param rb    Read buffer to initialise.
 * @param buf   Memory buffer to read from.
 * @param size  Size of the buffer.
 */
void whist_read_buffer_init(WhistReadBuffer *rb, const uint8_t *buf, size_t size);

/**
 * Test whether a read buffer has underflowed.
 *
 * @param rb  Read buffer to test.
 * @return  Whether the buffer has underflowed.
 */
bool whist_read_buffer_has_underflowed(WhistReadBuffer *rb);

/**
 * Skip bytes in the read buffer.
 *
 * @param rb    Read buffer to skip in.
 * @param skip  Number of bytes to skip.
 */
void whist_read_buffer_skip(WhistReadBuffer *rb, size_t skip);

/**
 * Return a pointer to the current position in the buffer.
 *
 * This may be useful for cases where a large segment of data can be
 * used directly without copying.
 */
const uint8_t *whist_read_buffer_get_pointer(WhistReadBuffer *rb);

/**
 * Read a 1, 2, 3, 4 or 8-byte value from a buffer.
 *
 * @param rb  Buffer to read from.
 * @return  The value read.
 */
uint8_t whist_read_1(WhistReadBuffer *rb);
uint16_t whist_read_2(WhistReadBuffer *rb);
uint32_t whist_read_3(WhistReadBuffer *rb);
uint32_t whist_read_4(WhistReadBuffer *rb);
uint64_t whist_read_8(WhistReadBuffer *rb);

/**
 * Read an LEB128-encoded value from a buffer.
 *
 * @param rb  Buffer to read from.
 * @return  The value read.
 */
uint64_t whist_read_leb128(WhistReadBuffer *rb);

/**
 * Read a block of binary data from a buffer.
 *
 * @param rb    Buffer to read from.
 * @param data  Where to write the data.
 * @param size  Number of bytes to read.
 */
void whist_read_data(WhistReadBuffer *rb, uint8_t *data, size_t size);

#endif /* WHIST_UTILS_BUFFER_H */
