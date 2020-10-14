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
    */

    // Read BMP file header contents
    int w, h;
    static const unsigned MINHEADER = 54;
    if (size < MINHEADER) return -1;
    if (bmp[0] != 'B' || bmp[1] != 'M') return -1;
    printf("File bit size = %d\n", bmp[28]);
    unsigned pixeloffset = *((int*)(&bmp[10]));
    w = *((int*)(&bmp[18]));
    h = *((int*)(&bmp[22]));
    if (bmp[28] != 24 && bmp[28] != 32) return -1;
    unsigned numChannels = bmp[28] / 8;
    unsigned scanlineBytes = w * numChannels;
    if (scanlineBytes % 4 != 0) scanlineBytes = (scanlineBytes / 4) * 4 + 4;

    // BMP pixel arrays are always multiples of 4. Images with widths that are not multiples
    //  of 4 are always padded at the end of each row. orig_bmp_w is the padded width of each
    //  BMP row in the original image.
    int orig_bmp_w = w % 4 == 0 ? w : (w / 4) * 4 + 4;

    // Depending on numChannels, we could be dealing with BGR24 or BGR32 in the BMP, but
    //  always save to PNG RGBA
    enum AVPixelFormat bmp_format = numChannels == 3 ? AV_PIX_FMT_BGR24 : AV_PIX_FMT_BGR32;
    enum AVPixelFormat png_format = AV_PIX_FMT_RGBA;

    // BMP files are stored with each row left to right, but rows are then stored bottom to top.
    //  This means that rows must be reversed into a new buffer before being passed on for conversion.
    uint8_t* bmp_original_buffer = (uint8_t*)(bmp + pixeloffset);
    int bmp_bytes = av_image_get_buffer_size(bmp_format, w, h, numChannels * 8);
    uint8_t* bmp_buffer = (uint8_t*)av_malloc(bmp_bytes);
    for (int y = 0; y < h; y++) {
        memcpy(
            bmp_buffer + w * (h - y - 1) * numChannels, 
            bmp_original_buffer + orig_bmp_w * y * numChannels, 
            w * numChannels
        );
    }

    // Create a new buffer for the PNG image
    int png_bytes = av_image_get_buffer_size(png_format, w, h, 32);
    uint8_t* png_buffer = (uint8_t*)av_malloc(png_bytes);

    // Create and fill the frames for both the original BMP image and the new PNG image
    AVFrame* bmp_frame = av_frame_alloc();
    bmp_frame->format = bmp_format;
    bmp_frame->width = w;
    bmp_frame->height = h;
    AVFrame* png_frame = av_frame_alloc();
    png_frame->format = png_format;
    png_frame->width = w;
    png_frame->height = h;

    av_image_fill_arrays(bmp_frame->data, bmp_frame->linesize, bmp_buffer, bmp_format, w, h, 1);
    av_image_fill_arrays(png_frame->data, png_frame->linesize, png_buffer, png_format, w, h, 1);

    // Convert the BMP image into the PNG image
    struct SwsContext* conversionContext =
        sws_getContext(w, h, bmp_format, w, h, png_format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    sws_scale(conversionContext, bmp_frame->data, bmp_frame->linesize, 0, h, png_frame->data,
              png_frame->linesize);

    // Encode the PNG image frame into an actual PNG image 
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!codec) {
        printf("Codec not found\n");
        exit(1);
    }
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        printf("Could not allocate video codec context\n");
        exit(1);
    }

    c->bit_rate = 400000;
    c->width = w;
    c->height = h;
    c->time_base = (AVRational){1, 25};
    c->color_range = AVCOL_RANGE_MPEG;
    c->pix_fmt = png_format;

    if (avcodec_open2(c, codec, NULL) < 0) {
        printf("Could not open codec\n");
        exit(1);
    }

    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;

    avcodec_send_frame(c, png_frame);
    if (avcodec_receive_packet(c, pkt) < 0) {
        printf("Error encoding frame\n");
        exit(1);
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
    static AVInputFormat* infmt = NULL;
    if (!infmt) infmt = av_find_input_format("png");
    *pctx = avformat_alloc_context();
    const size_t bufferSize = data_size;
    uint8_t* buffer = av_malloc(bufferSize);
    struct buffer_data opaque = {.ptr = data, .size = data_size};
    AVIOContext* pbctx = avio_alloc_context(buffer, (int)bufferSize, 0, &opaque, read_open, NULL, NULL);
    (*pctx)->pb = pbctx;

    avformat_open_input(pctx, NULL, NULL, NULL);
    avformat_find_stream_info(*pctx, NULL);
    av_free(pbctx);
    return 0;
}

int load_png(uint8_t* data[4], int linesize[4], unsigned int* w, unsigned int* h,
             enum AVPixelFormat* pix_fmt, char* png_data, int size) {
    AVFormatContext* format_ctx = NULL;
    AVCodec* codec;
    AVCodecContext* codec_ctx;
    AVFrame* frame;
    int frame_decoded, ret = 0;
    AVPacket pkt;

    if ((ret = read_char_open(&format_ctx, png_data, size)) < 0) {
        printf("Fails 0\n");
        return ret;
    }

    codec_ctx = format_ctx->streams[0]->codec;
    codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (!codec) {
        printf("Fails 1\n");
        ret = AVERROR(EINVAL);
        goto end;
    }

    if ((ret = avcodec_open2(codec_ctx, codec, NULL)) < 0) {
        printf("Fails 2\n");
        goto end;
    }

    if (!(frame = av_frame_alloc())) {
        printf("Fails 3\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ret = av_read_frame(format_ctx, &pkt);
    if (ret < 0) {
        printf("Fails 4\n");
        goto end;
    }

    ret = avcodec_decode_video2(codec_ctx, frame, &frame_decoded, &pkt);

    if (ret < 0 || !frame_decoded) {
        printf("Fails 5\n");
        goto end;
    }
    ret = 0;

    *w = frame->width;
    *h = frame->height;
    *pix_fmt = frame->format;

    if ((ret = av_image_alloc(data, linesize, (int)*w, (int)*h, *pix_fmt, 32)) < 0) goto end;
    ret = 0;

    av_image_copy(data, linesize, (const uint8_t**)frame->data, frame->linesize, *pix_fmt, (int)*w,
                  (int)*h);

end:
    avcodec_close(codec_ctx);
    avformat_close_input(&format_ctx);
    av_freep(&frame);
    return ret;
}

int load_png_file(uint8_t* data[4], int linesize[4], unsigned int* w, unsigned int* h,
                  enum AVPixelFormat* pix_fmt, char* png_filename) {
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

    struct SwsContext* swsContext =
        sws_getContext(*w, *h, *pix_fmt, *w, *h, AV_PIX_FMT_RGB24, 0, NULL, NULL, NULL);

    sws_scale(swsContext, (uint8_t const* const*)frame->data, frame->linesize, 0, *h, data,
              linesize);

end:
    avcodec_close(codec_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    av_freep(&frame);
    return ret;
}

int png_to_bmp_char(char* png, int size, AVPacket* pkt) {
    uint8_t* input[4];
    int linesize[4];
    unsigned int width, height;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_RGB24;
    load_png(input, linesize, &width, &height, &pix_fmt, png, size);

    unsigned int pixeloffset = 54;
    unsigned int numChannels = 3;
    unsigned int scanlineBytes = width * numChannels;
    unsigned int dataSize = scanlineBytes * height + pixeloffset;

    unsigned char* bmp = calloc(sizeof(unsigned char*), dataSize);
    // File type Data
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = dataSize;
    bmp[3] = dataSize >> 8u;
    bmp[4] = dataSize >> 16u;
    bmp[5] = dataSize >> 24u;
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
    pkt->size = (int)(dataSize);
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

int png_to_bmp(char* png, AVPacket* pkt) {
    uint8_t* input[4];
    int linesize[4];
    unsigned int width, height;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_RGB24;
    load_png_file(input, linesize, &width, &height, &pix_fmt, png);

    unsigned int pixeloffset = 54;
    unsigned int numChannels = 3;
    unsigned int scanlineBytes = width * numChannels;
    unsigned int dataSize = scanlineBytes * height + pixeloffset;

    unsigned char* bmp = calloc(sizeof(unsigned char*), dataSize);
    // File type Data
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = dataSize;
    bmp[3] = dataSize >> 8u;
    bmp[4] = dataSize >> 16u;
    bmp[5] = dataSize >> 24u;
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
    pkt->size = (int)(dataSize);
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