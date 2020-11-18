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

void* xcalloc(size_t nmemb, size_t size) {
    void* p;

    if (!(p = calloc(nmemb, size))) {
        LOG_ERROR("the memory could not be allocated");
    }
    return p;
}

char* read_file(const char* filename, size_t* char_nb) {
    FILE* file;
    char* code;
    // We try to open the file with the given filename
    if (!(file = fopen((const char*)filename, "rb"))) {
        LOG_ERROR("the file %s could not be opened", filename);
    }

    /*
     * We count the number of characters in the file
     * to allocate the correct amount of memory for
     * the char * that will contain the code
     */

    for ((*char_nb) = 0; fgetc(file) != EOF; ++(*char_nb))
        ;
    rewind(file);
    code = xcalloc((*char_nb) + 1, sizeof(unsigned char));

    // We copy the content of the file into the char *code
    if (fread(code, 1, *char_nb, file) < *char_nb) {
        LOG_ERROR("the file %s could not be read", filename);
    }
    fclose(file);

    return code;
}

int bmp_to_png(unsigned char* bmp, unsigned int size, AVPacket* pkt) {
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
    static const unsigned minheader = 54;
    if (size < minheader) {
        LOG_ERROR("BMP size < MINHEADER");
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
    if (size < (unsigned)data_size + pixeloffset) {
        LOG_WARNING("Actual size does not match given data_size and pixeloffset (%d < %d + %d)",
                    size, data_size, pixeloffset);
        return -1;
    }
    // Require that data_size is uncompressed raw data
    if (scanline_bytes * h != data_size) {
        LOG_WARNING("BMP size <> BMP header mismatch %d * %d != %d", scanline_bytes, h, data_size);
        return -1;
    }

    // Depending on numChannels, we could be dealing with BGR24 or BGR32 in the BMP, but
    //  always save to PNG RGB (not RGBA)
    LOG_INFO("NUM CHANNELS %d", num_channels);
    enum AVPixelFormat bmp_format = num_channels == 3 ? AV_PIX_FMT_BGR24 : AV_PIX_FMT_BGRA;
    enum AVPixelFormat png_format = AV_PIX_FMT_RGB24;

    // BMP files are stored with each row left to right, but rows are then stored bottom to top.
    //  This means that rows must be reversed into a new buffer before being passed on for
    //  conversion. Additionally, padding must be removed from the end of of each BMP row.
    uint8_t* bmp_original_buffer = (uint8_t*)(bmp + pixeloffset);
    int bmp_bytes = av_image_get_buffer_size(bmp_format, w, h, 1);
    uint8_t* bmp_buffer = (uint8_t*)av_malloc(bmp_bytes);
    if (!bmp_buffer) {
        LOG_ERROR("BMP buffer could not be allocated");
        return -1;
    }

    for (int y = 0; y < h; y++) {
        memcpy(bmp_buffer + w * (h - y - 1) * num_channels, bmp_original_buffer + scanline_bytes * y,
               w * num_channels);
    }

    // Create a new buffer for the PNG image
    int png_bytes = av_image_get_buffer_size(png_format, w, h, 1);
    uint8_t* png_buffer = (uint8_t*)av_malloc(png_bytes);
    if (!png_buffer) {
        av_free(bmp_buffer);
        LOG_ERROR("PNG buffer could not be allocated");
        return -1;
    }

    // Create and fill the frames for both the original BMP image and the new PNG image
    AVFrame* bmp_frame = av_frame_alloc();
    if (!bmp_frame) {
        av_free(bmp_buffer);
        av_free(png_buffer);
        LOG_ERROR("BMP frame could not be allocated");
        return -1;
    }
    bmp_frame->format = bmp_format;
    bmp_frame->width = w;
    bmp_frame->height = h;

    AVFrame* png_frame = av_frame_alloc();
    if (!png_frame) {
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        LOG_ERROR("PNG frame could not be allocated");
        return -1;
    }
    png_frame->format = png_format;
    png_frame->width = w;
    png_frame->height = h;

    if (av_image_fill_arrays(bmp_frame->data, bmp_frame->linesize, bmp_buffer, bmp_format, w, h,
                             1) < 0) {
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);
        LOG_ERROR("av_image_fill_arrays failed for BMP");
        return -1;
    }
    if (av_image_fill_arrays(png_frame->data, png_frame->linesize, png_buffer, png_format, w, h,
                             1) < 0) {
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);
        LOG_ERROR("av_image_fill_arrays failed for PNG");
        return -1;
    }

    // Convert the BMP image into the PNG image
    struct SwsContext* conversion_context =
        sws_getContext(w, h, bmp_format, w, h, png_format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (!conversion_context) {
        LOG_ERROR("sws_getContext failed");
        return -1;
    }
    sws_scale(conversion_context, (const uint8_t* const*)bmp_frame->data, bmp_frame->linesize, 0, h,
              png_frame->data, png_frame->linesize);

    // Encode the PNG image frame into an actual PNG image
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!codec) {
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);

        LOG_ERROR("Codec not found\n");
        return -1;
    }
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);

        LOG_ERROR("Could not allocate video codec context\n");
        return -1;
    }

    c->bit_rate = 400000;
    c->width = w;
    c->height = h;
    c->time_base = (AVRational){1, 25};
    c->color_range = AVCOL_RANGE_MPEG;
    c->pix_fmt = png_format;

    if (avcodec_open2(c, codec, NULL) < 0) {
        av_free(c);
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);

        LOG_ERROR("Could not open codec\n");
        return -1;
    }

    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;

    if (avcodec_send_frame(c, png_frame) < 0) {
        avcodec_close(c);
        av_free(c);
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);

        LOG_ERROR("Error sending frame\n");
        return -1;
    }

    if (avcodec_receive_packet(c, pkt) < 0) {
        avcodec_close(c);
        av_free(c);
        av_free(bmp_buffer);
        av_free(png_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);

        LOG_ERROR("Error encoding frame\n");
        return -1;
    }

    avcodec_close(c);
    av_free(c);
    av_free(bmp_buffer);
    av_free(png_buffer);
    av_frame_free(&bmp_frame);
    av_frame_free(&png_frame);
    return 0;
}

struct buffer_data {
    const char* ptr;
    size_t size;
};
static int read_open(void* opaque, uint8_t* buf, int buf_size) {
    struct buffer_data* bd = (struct buffer_data*)opaque;
    buf_size = FFMIN(buf_size, (int)bd->size);
    if (!buf_size) return AVERROR_EOF;
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;
    return buf_size;
}

int read_char_open(AVFormatContext** pctx, const char* data, int data_size) {
    /*
        Loads PNG file data into an ffmpeg stream stored in `pctx`

        Arguments:
            pctx (AVFormatContext**): format context that will contain the PNG stream
            data (const char*): the actual PNG file data array to be put into a stream
            data_size (int): size of the PNG file data array

        Return:
            ret (int): 0 on success, negative value on failure
            => ARG pctx is loaded with PNG stream

        NOTE: after a successful call, be sure to free (*pctx)->pb->buffer and then (*pctx->pb)
        before closing and freeing *pctx.
    */

    static AVInputFormat* infmt = NULL;
    if (!infmt) infmt = av_find_input_format("png");

    *pctx = avformat_alloc_context();
    if (*pctx == NULL) {
        return -1;
    }

    const size_t buffer_size = data_size;
    uint8_t* buffer = av_malloc(buffer_size);
    if (buffer == NULL) {
        avformat_free_context(*pctx);
        return -2;
    }

    struct buffer_data opaque = {.ptr = data, .size = data_size};
    AVIOContext* pbctx =
        avio_alloc_context(buffer, (int)buffer_size, 0, &opaque, read_open, NULL, NULL);
    if (pbctx == NULL) {
        av_free(buffer);
        avformat_free_context(*pctx);
        return -3;
    }
    (*pctx)->pb = pbctx;

    if (avformat_open_input(pctx, NULL, NULL, NULL) < 0) {
        av_free(buffer);
        av_free(pbctx);
        // avformat_free_context(*pctx); pctx will be freed on failure
        return -4;
    }

    if (avformat_find_stream_info(*pctx, NULL) < 0) {
        av_free(buffer);
        av_free(pbctx);
        avformat_free_context(*pctx);
        return -5;
    }

    return 0;
}

int load_png(uint8_t* data[4], int linesize[4], int* w, int* h, enum AVPixelFormat* pix_fmt,
             char* png_data, int size) {
    /*
        Loads PNG pixel data into `data[0]` given PNG file byte data in `png_data`
    */

    AVFormatContext* format_ctx = NULL;
    AVCodec* codec = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVFrame* frame;
    int ret = 0;
    AVPacket pkt;

    if ((ret = read_char_open(&format_ctx, png_data, size)) < 0) {
        LOG_ERROR("load_png could not load PNG file byte data into an ffmpeg stream: %d", ret);
        return ret;
    }

    codec = avcodec_find_decoder(format_ctx->streams[0]->codecpar->codec_id);
    if (!codec) {
        ret = AVERROR(EINVAL);
        goto end;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        ret = AVERROR(EINVAL);
        goto end;
    }

    if ((ret = avcodec_parameters_to_context(codec_ctx, format_ctx->streams[0]->codecpar)) < 0) {
        goto end;
    }

    if ((ret = avcodec_open2(codec_ctx, codec, NULL)) < 0) {
        goto end;
    }

    if (!(frame = av_frame_alloc())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ret = av_read_frame(format_ctx, &pkt);
    if (ret < 0) {
        goto end;
    }

    ret = avcodec_send_packet(codec_ctx, &pkt);

    if (ret < 0) {
        goto end;
    }

    ret = avcodec_receive_frame(codec_ctx, frame);

    if (ret < 0) {
        goto end;
    }
    ret = 0;

    *w = frame->width;
    *h = frame->height;
    *pix_fmt = frame->format;

    if ((ret = av_image_alloc(data, linesize, *w, *h, *pix_fmt, 1)) < 0) goto end;
    ret = 0;

    av_image_copy(data, linesize, (const uint8_t**)frame->data, frame->linesize, *pix_fmt, *w, *h);

end:

    avcodec_close(codec_ctx);
    avcodec_free_context(&codec_ctx);

    av_free(format_ctx->pb->buffer);
    av_free(format_ctx->pb);
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);

    av_freep(&frame);
    return ret;
}

int load_png_file(uint8_t* data[4], int linesize[4], unsigned int* w, unsigned int* h,
                  enum AVPixelFormat* pix_fmt, char* png_filename) {
    /*
        Loads PNG pixel data into `data[0]` given PNG filename in `png_filename`
    */

    AVFormatContext* format_ctx = NULL;
    AVCodec* codec = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVFrame* frame = NULL;
    int ret = 0;
    AVPacket pkt;

#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
    av_register_all();
    avfilter_register_all();
#endif

    char err_buf[1000];

    if ((ret = avformat_open_input(&format_ctx, png_filename, NULL, NULL)) < 0) {
        av_make_error_string(err_buf, sizeof(err_buf), ret);
        LOG_ERROR("avformat_open_input failed: %s", err_buf);
        return ret;
    }

    codec = avcodec_find_decoder(format_ctx->streams[0]->codecpar->codec_id);

    if (!codec) {
        LOG_ERROR("avcodec_find_decoder failed");
        ret = AVERROR(EINVAL);
        goto end;
    }

    codec_ctx = avcodec_alloc_context3(codec);

    ret = avcodec_parameters_to_context(codec_ctx, format_ctx->streams[0]->codecpar);
    if (ret < 0) {
        av_make_error_string(err_buf, sizeof(err_buf), ret);
        LOG_ERROR("avcodec_parameters_to_context failed: %s", err_buf);
        goto end;
    }

    if ((ret = avcodec_open2(codec_ctx, codec, NULL)) < 0) {
        av_make_error_string(err_buf, sizeof(err_buf), ret);
        LOG_ERROR("avcodec_open2 failed: %s", err_buf);
        goto end;
    }

    if (!(frame = av_frame_alloc())) {
        LOG_ERROR("av_frame_alloc failed");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ret = av_read_frame(format_ctx, &pkt);
    if (ret < 0) {
        av_make_error_string(err_buf, sizeof(err_buf), ret);
        LOG_ERROR("av_read_frame failed: %s", err_buf);
        goto end;
    }

    ret = avcodec_send_packet(codec_ctx, &pkt);

    if (ret < 0) {
        av_make_error_string(err_buf, sizeof(err_buf), ret);
        LOG_ERROR("avcodec_send_packet failed: %s", err_buf);
        goto end;
    }

    // TODO: for now we correctly assume only one frame pops out per packet, but
    // to be robust we could try to handle more as in videoencode.c
    ret = avcodec_receive_frame(codec_ctx, frame);

    if (ret < 0) {
        av_make_error_string(err_buf, sizeof(err_buf), ret);
        LOG_ERROR("avcodec_receive_frame failed: %s", err_buf);
        goto end;
    }

    ret = 0;

    *w = frame->width;
    *h = frame->height;
    *pix_fmt = frame->format;

    if ((ret = av_image_alloc(data, linesize, (int)*w, (int)*h, AV_PIX_FMT_RGB24, 32)) < 0) {
        av_make_error_string(err_buf, sizeof(err_buf), ret);
        LOG_ERROR("av_image_alloc failed: %s", err_buf);
        goto end;
    }
    ret = 0;

    struct SwsContext* sws_context =
        sws_getContext(*w, *h, *pix_fmt, *w, *h, AV_PIX_FMT_RGB24, 0, NULL, NULL, NULL);

    sws_scale(sws_context, (uint8_t const* const*)frame->data, frame->linesize, 0, *h, data,
              linesize);

end:
    avcodec_close(codec_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    av_freep(&frame);
    return ret;
}

int png_data_to_bmp_data(uint8_t* png_buffer, AVPacket* pkt, int* width, int* height,
                         enum AVPixelFormat* png_format, enum AVPixelFormat* bmp_format) {
    /*
        Convert PNG byte data into BMP byte data

        Arguments:
            png_buffer (uint8_t*): pointer to PNG pixel data byte buffer
            pkt (AVPacket*): pointer to AVPacket that will contain the output BMP data
            width (int*): pointer to width of image
            height (int*): pointer to height of image
            png_format (enum AVPixelFormat*): pointer to pixel format for PNG byte array
            bmp_format (enum AVPiexlFormat*): pointer to pixel format for BMP byte array

        Returns:
            ret (int): 0 on success, other integer on failure
            => ARG `pkt` is loaded with output BMP data array

        NOTE:
            After a successful call to png_data_to_bmp_data, remember to call
            av_packet_unref(pkt) to free packet memory.
    */

    // Create an empty BMP buffer based on image size.
    //  Note that align is 1 (not 4 or 32) because ffmpeg handles image encoding later on
    int bmp_bytes = av_image_get_buffer_size(*bmp_format, *width, *height, 1);
    uint8_t* bmp_buffer = (uint8_t*)av_malloc(bmp_bytes);

    // Create and fill the frames for both the original PNG image and the new BMP images
    AVFrame* png_frame = av_frame_alloc();
    png_frame->format = *png_format;
    png_frame->width = *width;
    png_frame->height = *height;
    AVFrame* bmp_frame = av_frame_alloc();
    bmp_frame->format = *bmp_format;
    bmp_frame->width = *width;
    bmp_frame->height = *height;

    av_image_fill_arrays(png_frame->data, png_frame->linesize, png_buffer, *png_format, *width,
                         *height, 1);
    av_image_fill_arrays(bmp_frame->data, bmp_frame->linesize, bmp_buffer, *bmp_format, *width,
                         *height, 1);

    // Convert the PNG pixel array to the BMP pixel array
    struct SwsContext* conversion_context =
        sws_getContext(*width, *height, *png_format, *width, *height, *bmp_format,
                       SWS_FAST_BILINEAR, NULL, NULL, NULL);
    sws_scale(conversion_context, (const uint8_t* const*)png_frame->data, png_frame->linesize, 0,
              *height, bmp_frame->data, bmp_frame->linesize);

    // Encode the BMP image frame into an actual BMP image
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_BMP);
    if (!codec) {
        LOG_ERROR("Codec not found\n");
        return -1;
    }
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        LOG_ERROR("Could not allocate video codec context\n");
        return -1;
    }

    c->bit_rate = 400000;
    c->width = *width;
    c->height = *height;
    c->time_base = (AVRational){1, 25};
    c->color_range = AVCOL_RANGE_MPEG;
    c->pix_fmt = *bmp_format;

    if (avcodec_open2(c, codec, NULL) < 0) {
        av_free(c);
        av_free(bmp_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);

        LOG_ERROR("Could not open codec\n");
        return -1;
    }

    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;

    avcodec_send_frame(c, bmp_frame);
    if (avcodec_receive_packet(c, pkt) < 0) {
        avcodec_close(c);
        av_free(c);
        av_free(bmp_buffer);
        av_frame_free(&bmp_frame);
        av_frame_free(&png_frame);

        LOG_ERROR("Error encoding frame\n");
        return -1;
    }

    avcodec_close(c);
    av_free(c);
    av_free(bmp_buffer);
    av_frame_free(&bmp_frame);
    av_frame_free(&png_frame);
    return 0;
}

int png_char_to_bmp(char* png, int size, AVPacket* pkt) {
    /*
        Convert a PNG byte array into a BMP byte array

        Arguments:
            png (char*): the PNG image byte representation (this is the entire file - headers and
       all) size (int): the size of the PNG image pkt (AVPacket*): the AVPacket into which the
       converted BMP byte array will be placed

        Returns:
            ret (int): 0 on success, other integer on failure
            => ARG `pkt` is loaded with output BMP data array

        NOTE:
            After a successful call to png_char_to_bmp, remember to call av_packet_unref(pkt)
            to free packet memory.
    */

    uint8_t* input[4];
    int linesize[4];
    int width, height;
    enum AVPixelFormat png_format = AV_PIX_FMT_RGB24;
    enum AVPixelFormat bmp_format = AV_PIX_FMT_BGR24;
    load_png(input, linesize, &width, &height, &png_format, png, size);

    uint8_t* png_buffer = (uint8_t*)input[0];
    if (png_data_to_bmp_data(png_buffer, pkt, &width, &height, &png_format, &bmp_format) != 0)
        return -1;

    av_freep(input);
    return 0;
}

int png_file_to_bmp(char* png, AVPacket* pkt) {
    /*
        Convert a PNG file to a BMP array.
        Legacy: this is used only for the loading screen and thus is written in the old
        conversion form instead of using ffmpeg libraries. If this function needs to be
        used for other uses in the future, UPDATE IT to use png_data_to_bmp_data.

        Arguments:
            png (char*): filename where the PNG image can be found
            pkt (AVPacket*): the AVPacket into which the converted BMP byte array will be placed

        Returns:
            ret (int): 0 on success, other integer on failure
            => ARG `pkt` is loaded with output BMP data array; pkt.data must be freed by the caller

        NOTE:
            After a successful call to png_file_to_bmp, remember to call free(pkt->data) to
            free memory.
    */

    uint8_t* input[4];
    int linesize[4];
    unsigned int width, height;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_RGB24;
    load_png_file(input, linesize, &width, &height, &pix_fmt, png);

    unsigned int pixeloffset = 54;
    unsigned int num_channels = 3;
    unsigned int scanline_bytes = width * num_channels;
    unsigned int data_size = scanline_bytes * height + pixeloffset;

    unsigned char* bmp = calloc(sizeof(unsigned char*), data_size);
    // File type Data
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = data_size;
    bmp[3] = data_size >> 8u;
    bmp[4] = data_size >> 16u;
    bmp[5] = data_size >> 24u;
    bmp[10] = pixeloffset;
    bmp[11] = pixeloffset >> 8u;
    bmp[12] = pixeloffset >> 16u;
    bmp[13] = pixeloffset >> 24u;
    // Image information Data
    bmp[14] = 40;
    bmp[18] = width;
    bmp[19] = width >> 8u;
    bmp[20] = width >> 16u;
    bmp[21] = width >> 24u;
    bmp[22] = height;
    bmp[23] = height >> 8u;
    bmp[24] = height >> 16u;
    bmp[25] = height >> 24u;
    bmp[26] = 1;
    bmp[28] = 24;

    unsigned char* output = bmp + pixeloffset;
    pkt->data = bmp;
    pkt->size = (int)(data_size);
    unsigned int offset, offset_base;
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            offset = (y * width + x) * 3;
            offset_base = ((height - y - 1) * width + x) * 3;
            output[offset] = input[0][offset_base + 2];
            output[offset + 1] = input[0][offset_base + 1];
            output[offset + 2] = input[0][offset_base + 0];
        }
    }

    av_freep(input);
    return 0;
}