#include <fractal/core/fractal.h>
#include "png.h"

#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lodepng.h"

#include "logging.h"

#ifdef _WIN32
#pragma warning(disable : 4996)
#pragma warning(disable : 4706)
// Check Later
#pragma warning(disable : 4244)
#endif

int bmp_to_png(char* bmp, int bmp_size, char** png, int* png_size) {
    /*
        Converts a bmp array into a png image format, stored within pkt

        Arguments:
            bmp (unsigned char*): BMP byte file representation (includes header)
            size (unsigned int): size of BMP file
            pkt (AVPacket*): AVPacket into which the converted PNG will be stored

        Return:
            ret (int): 0 on success, other integer on failure
            => ARG `pkt` is loaded with output PNG data array

        NOTE:
            After a successful call to bmp_to_png, remember to call av_packet_unref(pkt)
            to free packet memory.
    */

    // Read BMP file header contents
    if (bmp_size < 54) {
        LOG_ERROR("BMP size is smaller than the than the smallest acceptable BMP header");
        return -1;
    }

    // File signature
    if (bmp[0] != 'B' || bmp[1] != 'M') {
        LOG_ERROR("BMP magic number \"BM\" not found");
        return -1;
    }
    // File Size
    uint32_t file_size = *((uint32_t*)(&bmp[2]));
    // Unused, value doesn't matter
    UNUSED(*((uint32_t*)(&bmp[6])));
    // Pixel offset
    uint32_t pixel_offset = *((uint32_t*)(&bmp[10]));
    // Header size (from this point)
    uint32_t header_size = *((uint32_t*)(&bmp[14]));
    if (header_size != 40) {
        LOG_ERROR("DIB header size must be 40");
        return -1;
    }
    // Width/height
    uint32_t w = *((uint32_t*)(&bmp[18]));
    uint32_t h = *((uint32_t*)(&bmp[22]));
    // Number of planes (1)
    if (*((uint16_t*)(&bmp[26])) != 1) {
        LOG_ERROR("Number of BMP planes must be 1");
        return -1;
    }
    // Bits per pixel
    uint16_t bits_per_pixel = *((uint16_t*)(&bmp[28]));
    if (bits_per_pixel != 24 && bits_per_pixel != 32) {
        LOG_ERROR("BMP pixel width must be 3 bytes or 4 bytes, got %d bits instead",
                  bits_per_pixel);
        return -1;
    }
    // Compression (0 for none)
    if (*((uint32_t*)(&bmp[30])) != 0) {
        LOG_ERROR("BMP must be uncompressed BI_RGB data: %d found instead",
                  *((uint32_t*)(&bmp[30])));
        return -1;
    }
    // Size of the pixel data array
    uint32_t data_size = *((uint32_t*)(&bmp[34]));
    // horizontal DPI in pixels/meter, value doesn't matter
    UNUSED(*((uint32_t*)(&bmp[38])));
    // vertical DPI in pixels/meter, value doesn't matter
    UNUSED(*((uint32_t*)(&bmp[42])));
    // Must be 0, no palette
    if (*((uint32_t*)(&bmp[46])) != 0 || *((uint32_t*)(&bmp[50])) != 0) {
        LOG_ERROR("BMP color palettes are not supported");
        return -1;
    }

    uint16_t num_channels = bmp[28] / 8;

    // BMP pixel arrays are always multiples of 4. Images with widths that are not multiples
    //  of 4 are always padded at the end of each row. scanline_bytes is the padded width of each
    //  BMP row in the BMP image data.
    uint32_t scanline_bytes = w * num_channels;
    if (scanline_bytes % 4 != 0) scanline_bytes = (scanline_bytes / 4) * 4 + 4;

    // Data size can be 0, which means that it's scanline_bytes * h by default
    if (data_size == 0) {
        data_size = scanline_bytes * h;
    }
    // Require that data_size is uncompressed raw data
    if (scanline_bytes * h != data_size) {
        LOG_WARNING("BMP scanline * h <> BMP header data size mismatch %d * %d != %d",
                    scanline_bytes, h, data_size);
        return -1;
    }

    // Verify file_size against pixel_offset and data_size
    if (file_size < pixel_offset + data_size) {
        LOG_ERROR("BMP file_size is too small to fit PIXEL_OFFSET + DATA_SIZE, %d < %d + %d",
                  file_size, pixel_offset, data_size);
    }

    // Check that the actual bmp buffer is the expected file size
    if ((unsigned)bmp_size != file_size) {
        LOG_WARNING("Actual size does not match header filesize (%d != %d)", bmp_size, file_size);
        return -1;
    }

    // Create bmp buffer for lodepng
    uint8_t* pixel_data_buffer = (uint8_t*)allocate_region(bmp_size);

    for (uint32_t y = 0; y < h; y++) {
        // BMP rows are stored bottom-to-top,
        // this means we have to flip them by indexing by (h - y - 1)
        // Note that scanline_bytes is often not num_channels*w, due to padding,
        // But the target buffer will not be padded
        uint8_t* source_pixel_ptr = (uint8_t*)bmp + pixel_offset + (h - y - 1) * scanline_bytes;
        uint8_t* target_pixel_ptr = pixel_data_buffer + y * (num_channels * w);
        for (uint32_t x = 0; x < w; x++) {
            // BMPs are stored in BGR(A), and we need them in RGB(A)
            if (num_channels == 3) {
                *(target_pixel_ptr + 0) = *(source_pixel_ptr + 2);  // R
                *(target_pixel_ptr + 1) = *(source_pixel_ptr + 1);  // G
                *(target_pixel_ptr + 2) = *(source_pixel_ptr + 0);  // B
            } else {
                *(target_pixel_ptr + 0) = *(source_pixel_ptr + 2);  // R
                *(target_pixel_ptr + 1) = *(source_pixel_ptr + 1);  // G
                *(target_pixel_ptr + 2) = *(source_pixel_ptr + 0);  // B
                *(target_pixel_ptr + 3) = *(source_pixel_ptr + 3);  // A
            }
            // Source will be BGR or BGRA, depending on num_channels
            source_pixel_ptr += num_channels;
            // Target will be RGB or RGBA, depending on num_channels
            target_pixel_ptr += num_channels;
        }
    }

    // Convert to png
    // We cannot cast a `int*` to a `size_t*`, so we introduce intermediate `temp_png_size`
    int err;
    size_t temp_png_size;
    if (num_channels == 3) {
        err = lodepng_encode24((unsigned char**)png, &temp_png_size, pixel_data_buffer, w, h);
    } else {
        err = lodepng_encode32((unsigned char**)png, &temp_png_size, pixel_data_buffer, w, h);
    }

    deallocate_region(pixel_data_buffer);

    if (temp_png_size > INT_MAX) {
        LOG_WARNING("Converted PNG is too large for int");
        return -1;
    }

    *png_size = (int)temp_png_size;

    if (err != 0) {
        LOG_WARNING("Failed to encode BMP");
        return -1;
    }

    return 0;
}

int png_to_bmp(char* png, int png_size, char** bmp_ptr, int* bmp_size) {
    unsigned char* lodepng_data;
    unsigned int w, h;

    int num_channels = 4;
    if (lodepng_decode32(&lodepng_data, &w, &h, (unsigned char*)png, png_size) != 0) {
        LOG_WARNING("Failed to decode PNG");
        return -1;
    }

    // ====================
    // Create Bitmap Header
    // ====================

    // Scanlines must round up to a multiple of 4
    int scanline_bytes = w * num_channels;
    if (scanline_bytes % 4 != 0) scanline_bytes = (scanline_bytes / 4) * 4 + 4;

    // Create return buffer
    char* bmp = allocate_region(54 + (scanline_bytes * h));
    *bmp_ptr = bmp;
    *bmp_size = 54 + (scanline_bytes * h);

    // File signature
    bmp[0] = 'B';
    bmp[1] = 'M';
    // File Size
    *((uint32_t*)(&bmp[2])) = 54 + h * scanline_bytes;
    // Unused
    *((uint32_t*)(&bmp[6])) = 0;
    // Pixel offset
    *((uint32_t*)(&bmp[10])) = 54;
    // Header size (from this point)
    *((uint32_t*)(&bmp[14])) = 40;
    // Width/height
    *((uint32_t*)(&bmp[18])) = w;
    *((uint32_t*)(&bmp[22])) = h;
    // Number of planes (1)
    *((uint16_t*)(&bmp[26])) = 1;
    // Bits per pixel
    *((uint16_t*)(&bmp[28])) = 8 * num_channels;
    // Compression (0 for none)
    *((uint32_t*)(&bmp[30])) = 0;
    // Size of the pixel data array
    *((uint32_t*)(&bmp[34])) = h * scanline_bytes;
    // horizontal DPI in pixels/meter
    *((uint32_t*)(&bmp[38])) = 2835;
    // vertical DPI in pixels/meter
    *((uint32_t*)(&bmp[42])) = 2835;
    // Must be 0, no palette
    *((uint32_t*)(&bmp[46])) = 0;
    // Must be 0, no palette
    *((uint32_t*)(&bmp[50])) = 0;

    // ====================
    // Copy Bitmap Data
    // ====================

    for (size_t y = 0; y < h; y++) {
        // BMP rows are stored bottom-to-top,
        // this means we have to flip them by indexing by (h - y - 1)
        // Note that scanline_bytes is often not 4*w, due to padding,
        // But the source buffer will not be padded
        uint8_t* source_pixel_ptr = lodepng_data + y * (4 * w);
        uint8_t* target_pixel_ptr = (uint8_t*)bmp + 54 + (h - y - 1) * scanline_bytes;
        for (size_t x = 0; x < w; x++) {
            // lodepng gives us RGB(A), we need BGR(A)
            *(target_pixel_ptr + 0) = *(source_pixel_ptr + 2);  // B
            *(target_pixel_ptr + 1) = *(source_pixel_ptr + 1);  // G
            *(target_pixel_ptr + 2) = *(source_pixel_ptr + 0);  // R
            *(target_pixel_ptr + 3) = *(source_pixel_ptr + 3);  // A
            source_pixel_ptr += 4;
            target_pixel_ptr += 4;
        }
    }

    return 0;
}

void free_png(char* png) { free(png); }
void free_bmp(char* bmp) { deallocate_region(bmp); }
