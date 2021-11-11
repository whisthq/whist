

/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file ProtocolTest.cpp
 * @brief This file contains all the uni tests for the /protocol codebase code f
============================
Usage
============================

Include paths should be relative to the protocol folder
Examples:
  - To include file.h in protocol folder, use #include "file.h"
  - To include file2.h in protocol/client folder, use #include "client/file.h"
To include a C source file, you need to wrap the include statement in extern "C" {}.
*/

/*
============================
Includes
============================
*/

#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "client/client_utils.h"
#include "client/ringbuffer.h"
#include "fractal/utils/color.h"
#include <fractal/core/fractal.h>
#include <fcntl.h>

#ifndef __APPLE__
#include "server/main.h"
#include "server/parse_args.h"
#endif

#include "client/client_utils.h"
#include <fractal/utils/aes.h>
#include <fractal/utils/png.h>
#include <fractal/utils/avpacket_buffer.h>
}

/*
============================
Test Fixtures
============================
*/

#define TEST_OUTPUT_DIRNAME "test_output"

#if defined(_WIN32)
#define STDOUT_FILENO _fileno(stdout)
#define safe_mkdir(dir) _mkdir(dir)
#define safe_dup(fd) _dup(fd)
#define safe_dup2(fd1, fd2) _dup2(fd1, fd2)
#define safe_open(path, flags) _open(path, flags)
#define safe_close(fd) _close(fd)
#else
#define safe_mkdir(dir) mkdir(dir, 0777)
#define safe_dup(fd) dup(fd)
#define safe_dup2(fd1, fd2) dup2(fd1, fd2)
#define safe_open(path, flags) open(path, flags, 0666)
#define safe_close(fd) close(fd)
#endif

class CaptureStdoutTest : public ::testing::Test {
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

/*
============================
Matchers
============================
*/

#define LOG_DEBUG_MATCHER ::testing::HasSubstr("DEBUG")
#define LOG_INFO_MATCHER ::testing::HasSubstr("INFO")
#define LOG_WARNING_MATCHER ::testing::HasSubstr("WARNING")
#define LOG_ERROR_MATCHER ::testing::HasSubstr("ERROR")
#define LOG_FATAL_MATCHER ::testing::HasSubstr("FATAL")

/*
============================
Example Test
============================
*/

// Example of a test using a function from the client module
TEST_F(CaptureStdoutTest, ClientParseArgsEmpty) {
    int argc = 1;

    char argv0[] = "./client/build64/FractalClient";
    char* argv[] = {argv0, NULL};

    int ret_val = client_parse_args(argc, argv);
    EXPECT_EQ(ret_val, -1);

    check_stdout_line(::testing::StartsWith("Usage:"));
    check_stdout_line(::testing::HasSubstr("--help"));
}

/*
============================
Client Tests
============================
*/

/**
 * client/ringbuffer.c
 **/

// Constants for ringbuffer tests
#define NUM_AUDIO_TEST_FRAMES 25
#define MAX_RING_BUFFER_SIZE 500

// Tests that an initialized ring buffer is correct size and has
// frame IDs initialized to -1
TEST(ProtocolTest, InitRingBuffer) {
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, NUM_AUDIO_TEST_FRAMES);

    EXPECT_EQ(rb->ring_buffer_size, NUM_AUDIO_TEST_FRAMES);
    for (int frame_num = 0; frame_num < NUM_AUDIO_TEST_FRAMES; frame_num++)
        EXPECT_EQ(rb->receiving_frames[frame_num].id, -1);

    destroy_ring_buffer(rb);
}

// Tests that an initialized ring buffer with a bad size returns NULL
TEST_F(CaptureStdoutTest, InitRingBufferBadSize) {
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, MAX_RING_BUFFER_SIZE + 1);
    EXPECT_TRUE(rb == NULL);
    check_stdout_line(LOG_ERROR_MATCHER);
}

// Tests adding packets into ringbuffer
TEST_F(CaptureStdoutTest, AddingPacketsToRingBuffer) {
    // initialize ringbuffer
    const size_t num_packets = 1;
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, num_packets);

    // setup packets to add to ringbuffer
    FractalPacket pkt1 = {0};
    pkt1.type = PACKET_VIDEO;
    pkt1.id = 0;
    pkt1.index = 0;
    pkt1.is_a_nack = false;

    FractalPacket pkt2 = {0};
    pkt2.type = PACKET_VIDEO;
    pkt2.id = 1;
    pkt2.index = 0;
    pkt2.is_a_nack = false;

    // checks that everything goes well when adding to an empty ringbuffer
    EXPECT_EQ(receive_packet(rb, &pkt1), 0);
    EXPECT_EQ(get_frame_at_id(rb, pkt1.id)->id, pkt1.id);

    // checks that 1 is returned when overwriting a valid frame
    EXPECT_EQ(receive_packet(rb, &pkt2), 1);
    EXPECT_EQ(get_frame_at_id(rb, pkt2.id)->id, pkt2.id);

    // check that -1 is returned when we get a duplicate
    EXPECT_EQ(receive_packet(rb, &pkt2), -1);

    destroy_ring_buffer(rb);

    // For now we use the CaptureStdoutTest fixture to simply suppress stdout;
    // eventually we should validate output.
}

// Test that resetting the ringbuffer resets the values
TEST(ProtocolTest, ResetRingBufferFrame) {
    // initialize ringbuffer
    const size_t num_packets = 1;
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, num_packets);

    // fill ringbuffer
    FractalPacket pkt1;
    pkt1.type = PACKET_VIDEO;
    pkt1.id = 0;
    pkt1.index = 0;
    pkt1.is_a_nack = false;
    pkt1.payload_size = 0;

    receive_packet(rb, &pkt1);
    reset_frame(rb, get_frame_at_id(rb, pkt1.id));

    EXPECT_EQ(receive_packet(rb, &pkt1), 0);
}

TEST_F(CaptureStdoutTest, LoggerTest) {
    init_logger();
    LOG_DEBUG("This is a debug log!");
    LOG_INFO("This is an info log!");
    flush_logs();
    LOG_WARNING("This is a warning log!");
    LOG_ERROR("This is an error log!");
    flush_logs();
    LOG_INFO("AAA");
    LOG_INFO("BBB");
    LOG_INFO("CCC");
    LOG_INFO("DDD");
    LOG_INFO("EEE");
    destroy_logger();

    // Validate stdout, line-by-line
    check_stdout_line(::testing::HasSubstr("Logging initialized!"));
    check_stdout_line(LOG_DEBUG_MATCHER);
    check_stdout_line(LOG_INFO_MATCHER);
    check_stdout_line(LOG_WARNING_MATCHER);
    check_stdout_line(LOG_ERROR_MATCHER);
    check_stdout_line(::testing::EndsWith("AAA"));
    check_stdout_line(::testing::EndsWith("BBB"));
    check_stdout_line(::testing::EndsWith("CCC"));
    check_stdout_line(::testing::EndsWith("DDD"));
    check_stdout_line(::testing::EndsWith("EEE"));
}

/*
============================
Server Tests
============================
*/

#ifndef __APPLE__  // Server tests do not compile on macOS

/**
 * server/main.c
 **/

// Testing that good values passed into server_parse_args returns success
TEST_F(CaptureStdoutTest, ServerParseArgsUsage) {
    int argc = 2;

    char argv0[] = "./server/build64/FractalServer";
    char argv1[] = "--help";
    char* argv[] = {argv0, argv1, NULL};

    int ret_val = server_parse_args(argc, argv);
    EXPECT_EQ(ret_val, 1);

    check_stdout_line(::testing::HasSubstr("Usage:"));
}

#endif

/*
============================
Whist Library Tests
============================
*/

// Constants used for testing encryption
#define DEFAULT_BINARY_PRIVATE_KEY \
    "\xED\x5E\xF3\x3C\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\x19\xEF"
#define SECOND_BINARY_PRIVATE_KEY "\xED\xED\xED\xED\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\xED\xED"

/**
 * utils/color.c
 **/

TEST(ProtocolTest, FractalColorTest) {
    FractalRGBColor cyan = {0, 255, 255};
    FractalRGBColor magenta = {255, 0, 255};
    FractalRGBColor dark_gray = {25, 25, 25};
    FractalRGBColor light_gray = {150, 150, 150};
    FractalRGBColor fractal_purple_rgb = {79, 53, 222};
    FractalYUVColor fractal_purple_yuv = {85, 198, 127};

    // equality works
    EXPECT_EQ(rgb_compare(cyan, cyan), 0);
    EXPECT_EQ(rgb_compare(magenta, magenta), 0);

    // inequality works
    EXPECT_EQ(rgb_compare(cyan, magenta), 1);
    EXPECT_EQ(rgb_compare(magenta, cyan), 1);

    // dark color wants light text
    EXPECT_FALSE(color_requires_dark_text(dark_gray));

    // light color wants dark text
    EXPECT_TRUE(color_requires_dark_text(light_gray));

    // yuv conversion works (with some fuzz)
    EXPECT_NEAR(yuv_to_rgb(fractal_purple_yuv).red, fractal_purple_rgb.red, 2);
    EXPECT_NEAR(yuv_to_rgb(fractal_purple_yuv).green, fractal_purple_rgb.green, 2);
    EXPECT_NEAR(yuv_to_rgb(fractal_purple_yuv).blue, fractal_purple_rgb.blue, 2);
}

/**
 * utils/clock.c
 **/

TEST(ProtocolTest, TimersTest) {
    // Note: This test is currently a no-op, as the GitHub Actions runner is too slow for
    // sleep/timer to work properly. Uncomment the code to run it locally.

    // Note that this test will detect if either the timer or the sleep function
    // is broken, but not necessarily if both are broken.
    // clock timer;
    // start_timer(&timer);
    // fractal_sleep(25);
    // double elapsed = get_timer(timer);
    // EXPECT_GE(elapsed, 0.025);
    // EXPECT_LE(elapsed, 0.035);

    // start_timer(&timer);
    // fractal_sleep(100);
    // elapsed = get_timer(timer);
    // EXPECT_GE(elapsed, 0.100);
    // EXPECT_LE(elapsed, 0.110);
}

/**
 * utils/aes.c
 **/

// This test makes a packet, encrypts it, decrypts it, and confirms the latter is
// the original packet
TEST(ProtocolTest, EncryptAndDecrypt) {
    const char* data = "testing...testing";
    size_t len = strlen(data);

    // Construct test packet
    FractalPacket original_packet;

    // Contruct packet metadata
    original_packet.id = -1;
    original_packet.type = PACKET_MESSAGE;
    original_packet.index = 0;
    original_packet.payload_size = (int)len;
    original_packet.num_indices = 1;
    original_packet.is_a_nack = false;

    // Copy packet data
    memcpy(original_packet.data, data, len);

    // Encrypt the packet using aes encryption
    int original_len = PACKET_HEADER_SIZE + original_packet.payload_size;

    FractalPacket encrypted_packet;
    int encrypted_len = encrypt_packet(&original_packet, original_len, &encrypted_packet,
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    // decrypt packet
    FractalPacket decrypted_packet;

    int decrypted_len = decrypt_packet(&encrypted_packet, encrypted_len, &decrypted_packet,
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    // compare original and decrypted packet
    EXPECT_EQ(decrypted_len, original_len);
    EXPECT_EQ(decrypted_packet.payload_size, len);
    EXPECT_EQ(strncmp((char*)decrypted_packet.data, (char*)original_packet.data, len), 0);
}

// This test encrypts a packet with one key, then attempts to decrypt it with a differing
// key, confirms that it returns -1
TEST_F(CaptureStdoutTest, BadDecrypt) {
    const char* data = "testing...testing";
    size_t len = strlen(data);

    // Construct test packet
    FractalPacket original_packet;

    // Contruct packet metadata
    original_packet.id = -1;
    original_packet.type = PACKET_MESSAGE;
    original_packet.index = 0;
    original_packet.payload_size = (int)len;
    original_packet.num_indices = 1;
    original_packet.is_a_nack = false;

    // Copy packet data
    memcpy(original_packet.data, data, len);

    // Encrypt the packet using aes encryption
    int original_len = PACKET_HEADER_SIZE + original_packet.payload_size;

    FractalPacket encrypted_packet;
    int encrypted_len = encrypt_packet(&original_packet, original_len, &encrypted_packet,
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    // decrypt packet with differing key
    FractalPacket decrypted_packet;

    int decrypted_len = decrypt_packet(&encrypted_packet, encrypted_len, &decrypted_packet,
                                       (unsigned char*)SECOND_BINARY_PRIVATE_KEY);

    EXPECT_EQ(decrypted_len, -1);

    check_stdout_line(LOG_WARNING_MATCHER);
}

/**
 *  Only run on macOS and Linux for 2 reaons:
 *  1) There is an encoding difference on Windows that causes the
 *  images to be read differently, thus causing them to fail
 *  2) These tests on Windows add an additional 3-5 minutes for the Workflow
 */
#ifndef _WIN32
// Tests that by converting a PNG to a BMP then converting that back
// to a PNG returns the original image. Note that the test image
// must be the output of a lodepng encode, as other PNG encoders
// (including FFmpeg) may produce different results (lossiness,
// different interpolation, etc.).
TEST(ProtocolTest, PngToBmpToPng) {
    // Read in PNG
    std::ifstream png_image("assets/image.png", std::ios::binary);

    // copies all data into buffer
    std::vector<unsigned char> png_vec(std::istreambuf_iterator<char>(png_image), {});
    int png_buffer_size = (int)png_vec.size();
    char* png_buffer = (char*)&png_vec[0];

    // Convert to BMP
    char* bmp_buffer;
    int bmp_buffer_size;
    ASSERT_FALSE(png_to_bmp(png_buffer, png_buffer_size, &bmp_buffer, &bmp_buffer_size));

    // Convert back to PNG
    char* new_png_buffer;
    int new_png_buffer_size;
    ASSERT_FALSE(bmp_to_png(bmp_buffer, bmp_buffer_size, &new_png_buffer, &new_png_buffer_size));

    free_bmp(bmp_buffer);

    // compare for equality
    EXPECT_EQ(png_buffer_size, new_png_buffer_size);
    for (int i = 0; i < png_buffer_size; i++) {
        EXPECT_EQ(png_buffer[i], new_png_buffer[i]);
    }

    free_png(new_png_buffer);
    png_image.close();
}

// Tests that by converting a PNG to a BMP then converting that back
// to a PNG returns the original image. Note that the test image
// must be a BMP of the BITMAPINFOHEADER specification, where the
// now-optional paramters for x/y pixel resolutions are set to 0.
// `ffmpeg -i input-image.{ext} output.bmp` will generate such a BMP.
TEST(ProtocolTest, BmpToPngToBmp) {
    // Read in PNG
    std::ifstream bmp_image("assets/image.bmp", std::ios::binary);

    // copies all data into buffer
    std::vector<unsigned char> bmp_vec(std::istreambuf_iterator<char>(bmp_image), {});
    int bmp_buffer_size = (int)bmp_vec.size();
    char* bmp_buffer = (char*)&bmp_vec[0];

    // Convert to PNG
    char* png_buffer;
    int png_buffer_size;
    ASSERT_FALSE(bmp_to_png(bmp_buffer, bmp_buffer_size, &png_buffer, &png_buffer_size));

    // Convert back to BMP
    char* new_bmp_buffer;
    int new_bmp_buffer_size;
    ASSERT_FALSE(png_to_bmp(png_buffer, png_buffer_size, &new_bmp_buffer, &new_bmp_buffer_size));

    free_png(png_buffer);

    // compare for equality
    EXPECT_EQ(bmp_buffer_size, new_bmp_buffer_size);
    for (int i = 0; i < bmp_buffer_size; i++) {
        EXPECT_EQ(bmp_buffer[i], new_bmp_buffer[i]);
    }

    free_bmp(new_bmp_buffer);
    bmp_image.close();
}
#endif

// Adds AVPackets to an buffer via write_packets_to_buffer and
// confirms that buffer structure is correct
TEST(ProtocolTest, PacketsToBuffer) {
    // Make some dummy packets

    const char* data1 = "testing...testing";

    AVPacket avpkt1;
    avpkt1.buf = NULL;
    avpkt1.pts = AV_NOPTS_VALUE;
    avpkt1.dts = AV_NOPTS_VALUE;
    avpkt1.data = (uint8_t*)data1;
    avpkt1.size = (int)strlen(data1);
    avpkt1.stream_index = 0;
    avpkt1.side_data = NULL;
    avpkt1.side_data_elems = 0;
    avpkt1.duration = 0;
    avpkt1.pos = -1;

    // add them to AVPacket array
    AVPacket* packets = &avpkt1;

    // create buffer and add them to a buffer
    int buffer[28];
    write_avpackets_to_buffer(1, packets, buffer);

    // Confirm buffer creation was successful
    EXPECT_EQ(*buffer, 1);
    EXPECT_EQ(*(buffer + 1), (int)strlen(data1));
    EXPECT_EQ(strncmp((char*)(buffer + 2), data1, strlen(data1)), 0);
}

TEST(ProtocolTest, BitArrayMemCpyTest) {
    // A bunch of prime numbers + {10,100,200,250,299,300}
    std::vector<int> bitarray_sizes{1,  2,  3,  5,  7,  10, 11,  13,  17,  19, 23,
                                    29, 31, 37, 41, 47, 53, 100, 250, 299, 300};

    for (auto test_size : bitarray_sizes) {
        BitArray* bit_arr = bit_array_create(test_size);
        EXPECT_TRUE(bit_arr);

        bit_array_clear_all(bit_arr);
        for (int i = 0; i < test_size; i++) {
            EXPECT_EQ(bit_array_test_bit(bit_arr, i), 0);
        }

        std::vector<bool> bits_arr_check;

        for (int i = 0; i < test_size; i++) {
            int coin_toss = rand() % 2;
            EXPECT_TRUE(coin_toss == 0 || coin_toss == 1);
            if (coin_toss) {
                bits_arr_check.push_back(true);
                bit_array_set_bit(bit_arr, i);
            } else {
                bits_arr_check.push_back(false);
            }
        }

        unsigned char ba_raw[BITS_TO_CHARS(MAX_RING_BUFFER_SIZE)];
        memcpy(ba_raw, bit_array_get_bits(bit_arr), BITS_TO_CHARS(test_size));
        bit_array_free(bit_arr);

        BitArray* bit_arr_recovered = bit_array_create(test_size);
        EXPECT_TRUE(bit_arr_recovered);

        EXPECT_TRUE(bit_arr_recovered->array);
        EXPECT_TRUE(bit_arr_recovered->array != NULL);
        EXPECT_TRUE(ba_raw);
        EXPECT_TRUE(ba_raw != NULL);
        memcpy(bit_array_get_bits(bit_arr_recovered), ba_raw, BITS_TO_CHARS(test_size));

        for (int i = 0; i < test_size; i++) {
            if (bits_arr_check[i]) {
                EXPECT_GE(bit_array_test_bit(bit_arr_recovered, i), 1);
            } else {
                EXPECT_EQ(bit_array_test_bit(bit_arr_recovered, i), 0);
            }
        }

        bit_array_free(bit_arr_recovered);
    }
}

#ifndef _WIN32
// This test is disabled on Windows for the time being, since UTF-8
// seems to behave differently in MSVC, which causes indefinite hanging
// in our CI. See the implementation of `trim_utf8_string` for a bit
// more context.
TEST(ProtocolTest, Utf8Truncation) {
    // Test that a string with a UTF-8 character that is truncated
    // is fixed correctly.

    // UTF-8 string:
    char buf[] = {'\xe2', '\x88', '\xae', '\x20', '\x45', '\xe2', '\x8b', '\x85', '\x64',
                  '\x61', '\x20', '\x3d', '\x20', '\x51', '\x2c', '\x20', '\x20', '\x6e',
                  '\x20', '\xe2', '\x86', '\x92', '\x20', '\xe2', '\x88', '\x9e', '\x2c',
                  '\x20', '\xf0', '\x90', '\x8d', '\x88', '\xe2', '\x88', '\x91', '\x20',
                  '\x66', '\x28', '\x69', '\x29', '\x20', '\x3d', '\x20', '\xe2', '\x88',
                  '\x8f', '\x20', '\x67', '\x28', '\x69', '\x29', '\0'};

    // truncation boundaries that need to be trimmed
    const int bad_utf8_tests[] = {2, 3, 30, 31, 32};
    // truncation boundaries that are at legal positions
    const int good_utf8_tests[] = {4, 29, 33, 42, 50, 100};

    for (auto test : bad_utf8_tests) {
        char* truncated_buf = (char*)malloc(test);
        char* fixed_buf = (char*)malloc(test);
        safe_strncpy(truncated_buf, buf, test);
        safe_strncpy(fixed_buf, buf, test);
        trim_utf8_string(fixed_buf);
        EXPECT_TRUE(strncmp(truncated_buf, fixed_buf, test));
        free(fixed_buf);
        free(truncated_buf);
    }
    for (auto test : good_utf8_tests) {
        char* truncated_buf = (char*)malloc(test);
        char* fixed_buf = (char*)malloc(test);
        safe_strncpy(truncated_buf, buf, test);
        safe_strncpy(truncated_buf, buf, test);
        safe_strncpy(fixed_buf, buf, test);
        trim_utf8_string(fixed_buf);
        EXPECT_FALSE(strncmp(truncated_buf, fixed_buf, test));
        free(fixed_buf);
        free(truncated_buf);
    }
}
#endif  // _WIN32

/*
============================
Run Tests
============================
*/

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
