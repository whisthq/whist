/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file codec_test.cpp
 * @brief This file contains unit tests for codecs in the /protocol codebase
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <fstream>

extern "C" {
#include <fcntl.h>
#include "whist/video/codec/encode.h"
#include "whist/video/codec/decode.h"
}

#define TEST_OUTPUT_DIRNAME "test_output"

class CodecTest : public ::testing::Test {
    /*
        This class is a test fixture which redirects stdout to a file
        for the duration of the test. The file is then read and compared
        to registered expected output matchers on a line-by-line basis.
    */
   protected:
    void SetUp() override {
        /*
            This function captures the output of stdout to a file for the current
            test. The file is named after the test name, and is located in the
            test/test_output directory. The file is overwritten if it already
            exists.
        */
        old_stdout = safe_dup(STDOUT_FILENO);
        safe_mkdir(TEST_OUTPUT_DIRNAME);
        std::string filename = std::string(TEST_OUTPUT_DIRNAME) + "/" +
                               ::testing::UnitTest::GetInstance()->current_test_info()->name() +
                               ".log";
        fd = safe_open(filename.c_str(), O_WRONLY | O_CREAT);
        EXPECT_GE(fd, 0);
        fflush(stdout);
        safe_dup2(fd, STDOUT_FILENO);
    }

    void TearDown() override {
        /*
            This function releases the output of stdout captured by the `SetUp()`
            function. We then open the file and read it line by line, comparing
            the output to matchers we have registerd with `check_stdout_line()`.
        */
        fflush(stdout);
        safe_dup2(old_stdout, STDOUT_FILENO);
        safe_close(fd);
        std::ifstream file(std::string(TEST_OUTPUT_DIRNAME) + "/" +
                           ::testing::UnitTest::GetInstance()->current_test_info()->name() +
                           ".log");

        for (::testing::Matcher<std::string> matcher : line_matchers) {
            std::string line;
            std::getline(file, line);
            EXPECT_THAT(line, matcher);
        }

        file.close();
    }

    void check_stdout_line(::testing::Matcher<std::string> matcher) {
        /*
            This function registers a matcher to be used to compare the output
            of the current test to the expected output. For example, if we want
            to check that the output contains the string "hello", we can do so
            by calling `check_stdout_line(::testing::HasSubstr("hello"))`. If we
            then want to verify that the next line of output contains the string
            "world", we then call `check_stdout_line(::testing::HasSubstr("world"))`.

            The matcher is added to a vector of matchers, which is used to
            compare the output line-by-line. To add multiple matchers for a
            single line, we can use `::testing::AllOf()` or `::testing::AnyOf()`
            to combine multiple matchers into a single matcher.

            Note that the matchers are not checked until the `TearDown()` function
            is called.

            Arguments:
                matcher (::testing::Matcher<std::string>): The matcher to use
                    to compare the output line.
        */
        line_matchers.push_back(matcher);
    }

    int old_stdout;
    int fd;
    std::vector<::testing::Matcher<std::string>> line_matchers;
};

static void test_write_image(uint8_t *data, int width, int height, int pitch, int value) {
    // Encode a 16-bit number in binary using black and white patches in the video.
    int block_y = height / 4;
    int block_x = width / 4;
    for (int y = 0; y < height; y++) {
        uint8_t *line = data + y * pitch;
        int vy = y / block_y;
        for (int x = 0; x < width; x++) {
            int vx = x / block_x;
            uint8_t byte = value & (1 << (vy * 4 + vx)) ? 0xff : 0x00;
            *line++ = byte;
            *line++ = byte;
            *line++ = byte;
            *line++ = byte;
        }
    }
}

static int test_read_image(const uint8_t *data, int width, int height, int pitch, bool rgb) {
    // Read back the number used to create the image, which might be somewhat
    // fuzzy since it has been through and encode/decode at unknown quality.
    int block_y = height / 4;
    int block_x = width / 4;
    int value = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            uint8_t byte = *(data + (y * block_y + block_y / 2) * pitch +
                             (x * block_x + block_x / 2) * (rgb ? 4 : 1));
            // Something has gone wrong if it's not close to an extreme value.
            if (byte > 0x80) {
                EXPECT_GT(byte, 0xe0);
                value |= 1 << (y * 4 + x);
            } else {
                EXPECT_LT(byte, 0x20);
            }
        }
    }
    return value;
}

// Meta-test to make sure the image setup for encode/decode tests is correct.
TEST_F(CodecTest, ImageSetupTest) {
    int width = 1280;
    int height = 720;
    int pitch = 4 * width;
    uint8_t *image = (uint8_t *)malloc(pitch * height);

    int value_in, value_out;
    for (value_in = 0; value_in < 0x10000; value_in += 3299) {
        test_write_image(image, width, height, pitch, value_in);
        value_out = test_read_image(image, width, height, pitch, true);
        EXPECT_EQ(value_in, value_out);
    }

    free(image);
}

// Pre-encoded 64x64 stream of 10 frames each containing their frame number in binary.
typedef struct {
    size_t size;
    const uint8_t *packet;
} DecodeTestInput;
static const DecodeTestInput decode_test_input[] = {
#define PACKET(...)                                   \
    [] {                                              \
        static const uint8_t bytes[] = __VA_ARGS__;   \
        DecodeTestInput tmp = {sizeof(bytes), bytes}; \
        return tmp;                                   \
    }()
    PACKET({0x01, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x67,
            0x64, 0x00, 0x14, 0xac, 0xb6, 0x21, 0x34, 0x20, 0x00, 0x00, 0x03, 0x00, 0x20,
            0x00, 0x00, 0x0f, 0x11, 0xe2, 0x85, 0x5c, 0x00, 0x00, 0x00, 0x01, 0x68, 0xea,
            0xcc, 0xb2, 0x2c, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x06, 0xbf, 0xfe, 0xf7,
            0xd4, 0xb7, 0xcc, 0xb2, 0xee, 0x07, 0x22, 0xb6, 0x0b, 0xa8, 0xf7, 0xa2, 0x9c,
            0x8c, 0x81, 0xee, 0x0c, 0x19, 0x51, 0xa2, 0x7c, 0x81, 0xbd, 0x7b, 0x46, 0xcd}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x01, 0x41, 0x9a, 0x3b, 0x10, 0x6b, 0xff, 0xe3, 0x43, 0xff, 0x9f,
            0xd8, 0xff, 0xc0, 0xf9, 0xcb, 0x96, 0x03, 0xdd, 0x76, 0xa7, 0x2b}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41,
            0x9a, 0x46, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xf0, 0x9d, 0xf7, 0x9f, 0xb1,
            0x99, 0x11, 0x52, 0x99, 0x94, 0xcf, 0xba, 0x45, 0x5e, 0xbd, 0xe9, 0x0d, 0x71}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41, 0x9a,
            0x66, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xff, 0xbd, 0xd7, 0x6a, 0x72, 0xb0}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41,
            0x9a, 0x86, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xfe, 0x9a, 0xd9, 0x78, 0x14,
            0xd0, 0xe4, 0x1b, 0x0f, 0x76, 0xb9, 0x6a, 0xf6, 0xcd, 0xa9, 0x68, 0x6b, 0xc1}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41, 0x9a,
            0xa6, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xff, 0xbd, 0xd7, 0x6a, 0x72, 0xb1}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x41, 0x9a, 0xc6, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xf0, 0x9d, 0xf7,
            0x9f, 0xfd, 0xce, 0x84, 0x9f, 0x7a, 0xf3, 0x6e, 0x2e, 0x6c, 0x1d, 0xad}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41, 0x9a,
            0xe6, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xff, 0xbd, 0xd7, 0x6a, 0x72, 0xb1}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x41, 0x9b, 0x06, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xfe, 0x9a, 0xde,
            0x79, 0x4a, 0x5e, 0xfb, 0xcf, 0xfe, 0xfb, 0xb6, 0xda, 0xaa, 0xc0}),
    PACKET({0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41, 0x9b,
            0x26, 0x08, 0x35, 0xff, 0xaa, 0x7f, 0xff, 0xff, 0xbd, 0xd7, 0x6a, 0x72, 0xb0}),
#undef PACKET
};

// Decode the provided stream and check the output.
TEST_F(CodecTest, DecodeTest) {
    int width = 64;
    int height = 64;

    VideoDecoder *dec = create_video_decoder(width, height, false, CODEC_TYPE_H264);
    EXPECT_TRUE(dec);

    int ret;
    for (int n = 0; n < 10; n++) {
        const DecodeTestInput *input = &decode_test_input[n];

        ret = video_decoder_send_packets(dec, (void *)input->packet, (int)input->size);
        EXPECT_EQ(ret, 0);

        ret = video_decoder_decode_frame(dec);
        EXPECT_EQ(ret, 0);

        DecodedFrameData decode_out = video_decoder_get_last_decoded_frame(dec);

        AVFrame *frame = decode_out.decoded_frame;
        EXPECT_EQ(frame->format, AV_PIX_FMT_YUV420P);
        EXPECT_EQ(frame->width, width);
        EXPECT_EQ(frame->height, height);
        EXPECT_EQ(frame->pict_type, n == 0 ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_P);

        int value = test_read_image(frame->data[0], width, height, frame->linesize[0], false);

        EXPECT_EQ(value, n);

        video_decoder_free_decoded_frame(&decode_out);
    }

    destroy_video_decoder(dec);
}

// Run frames through an encode-decode pair and check the output.
#if __linux__
// We only have encode support on Linux.
TEST(CodecTest, EncodeDecodeTest) {
    int width = 1280;
    int height = 720;
    int pitch = 4 * width;
    uint8_t *image_rgb_in = (uint8_t *)malloc(pitch * height);
    EXPECT_TRUE(image_rgb_in);

    size_t packet_buffer_size = 4 * 1024 * 1024;
    uint8_t *packet_buffer = (uint8_t *)malloc(packet_buffer_size);
    EXPECT_TRUE(packet_buffer);

    VideoEncoder *enc =
        create_video_encoder(width, height, width, height, 1000000, CODEC_TYPE_H264);
    EXPECT_TRUE(enc);

    VideoDecoder *dec = create_video_decoder(width, height, false, CODEC_TYPE_H264);
    EXPECT_TRUE(dec);

    int ret;
    for (int frame = 0; frame < 100; frame++) {
        if (frame == 60) {
            // The first frame is implicitly an intra frame, also ask for one
            // partway through (and check below to make sure it was generated).
            video_encoder_set_iframe(enc);
        }

        test_write_image(image_rgb_in, width, height, pitch, frame);

        ret = ffmpeg_encoder_frame_intake(enc->ffmpeg_encoder, image_rgb_in, pitch);
        EXPECT_EQ(ret, 0);

        ret = video_encoder_encode(enc);
        EXPECT_EQ(ret, 0);
        EXPECT_LT(enc->encoded_frame_size, packet_buffer_size);

        write_avpackets_to_buffer(enc->num_packets, enc->packets, (int *)packet_buffer);

        ret = video_decoder_send_packets(dec, packet_buffer, enc->encoded_frame_size);
        EXPECT_EQ(ret, 0);

        ret = video_decoder_decode_frame(dec);
        EXPECT_EQ(ret, 0);

        DecodedFrameData decode_out = video_decoder_get_last_decoded_frame(dec);

        AVFrame *frame_out = decode_out.decoded_frame;
        EXPECT_EQ(frame_out->format, AV_PIX_FMT_YUV420P);
        EXPECT_EQ(frame_out->width, width);
        EXPECT_EQ(frame_out->height, height);
        if (frame == 0 || frame == 60)
            EXPECT_EQ(frame_out->pict_type, AV_PICTURE_TYPE_I);
        else
            EXPECT_EQ(frame_out->pict_type, AV_PICTURE_TYPE_P);

        int value =
            test_read_image(frame_out->data[0], width, height, frame_out->linesize[0], false);

        EXPECT_EQ(value, frame);

        video_decoder_free_decoded_frame(&decode_out);
    }

    destroy_video_encoder(enc);
    destroy_video_decoder(dec);

    free(image_rgb_in);
    free(packet_buffer);
}
#endif
