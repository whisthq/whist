#include <fractal/core/fractal.h>
#include "png.h"

#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "logging.h"

#ifdef _WIN32
#pragma warning(disable : 4996)
#pragma warning(disable : 4706)
// Check Later
#pragma warning(disable : 4244)
#endif

int bmp_to_png(char* bmp, int bmp_size, char* png, int* png_size) {
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
    int w, h;
    if (bmp_size < 54) {
        LOG_ERROR("BMP size is smaller than the than the smallest possible BMP header");
        return -1;
    }
    if (bmp[0] != 'B' || bmp[1] != 'M') {
        LOG_ERROR("BMP header BM failed");
        return -1;
    }
    int pixeloffset = *((int*)(&bmp[10]));
    w = *((int*)(&bmp[18]));
    h = *((int*)(&bmp[22]));
    if (bmp[28] != 24 && bmp[28] != 32) {
        LOG_ERROR("BMP is not 3 or 4 channel");
        return -1;
    }
    int num_channels = bmp[28] / 8;

    // BMP pixel arrays are always multiples of 4. Images with widths that are not multiples
    //  of 4 are always padded at the end of each row. scanlineBytes is the padded width of each
    //  BMP row in the original image.
    int scanline_bytes = w * num_channels;
    if (scanline_bytes % 4 != 0) scanline_bytes = (scanline_bytes / 4) * 4 + 4;

    int data_size = *((int*)(&bmp[34]));
    if (data_size == 0) {
        data_size = scanline_bytes * h;
    }

    // memcpy will face UB problems below if the calculated size does not match the actual
    // BMP byte array size
    if ((unsigned)bmp_size < (unsigned)data_size + pixeloffset) {
        LOG_WARNING("Actual size does not match given data_size and pixeloffset (%d < %d + %d)",
                    bmp_size, data_size, pixeloffset);
        return -1;
    }
    // Require that data_size is uncompressed raw data
    if (scanline_bytes * h != data_size) {
        LOG_WARNING("BMP size <> BMP header mismatch %d * %d != %d", scanline_bytes, h, data_size);
        return -1;
    }

    // Depending on numChannels, we could be dealing with BGR24 or BGRA32 in the BMP
    // enum AVPixelFormat bmp_format = num_channels == 3 ? AV_PIX_FMT_BGR24 : AV_PIX_FMT_BGRA;

    // Just copy into the buffer without compression for now
    memcpy(png, bmp, bmp_size);
    *png_size = bmp_size;

    return 0;
}

int png_to_bmp(char* png, int png_size, char* bmp, int* bmp_size) {
    // Just copy into the buffer without compression for now
    memcpy(bmp, png, png_size);
    *bmp_size = png_size;
    return 0;
}
