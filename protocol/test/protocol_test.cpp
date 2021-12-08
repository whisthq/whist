

/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file protocol_test.cpp
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

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS  // stupid Windows warnings
#endif

#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "client/client_utils.h"
#include "whist/utils/color.h"
#include <whist/core/whist.h>
#include <whist/network/ringbuffer.h>
#include <fcntl.h>

#ifndef __APPLE__
#include "server/state.h"
#include "server/parse_args.h"
#endif

#include "client/client_utils.h"
#include <whist/logging/log_statistic.h>
#include <whist/utils/aes.h>
#include <whist/utils/png.h>
#include <whist/utils/avpacket_buffer.h>
#include <whist/utils/atomic.h>
#include <whist/utils/fec.h>
#include <whist/core/whist_pollset.h>
}

/*
============================
Test Fixtures
============================
*/

#define TEST_OUTPUT_DIRNAME "test_output"

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

    char argv0[] = "./client/build64/WhistClient";
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
    RingBuffer* rb = init_ring_buffer(PACKET_VIDEO, NUM_AUDIO_TEST_FRAMES, NULL);

    EXPECT_EQ(rb->ring_buffer_size, NUM_AUDIO_TEST_FRAMES);
    for (int frame_num = 0; frame_num < NUM_AUDIO_TEST_FRAMES; frame_num++)
        EXPECT_EQ(rb->receiving_frames[frame_num].id, -1);

    destroy_ring_buffer(rb);
}

// Tests that an initialized ring buffer with a bad size returns NULL
TEST_F(CaptureStdoutTest, InitRingBufferBadSize) {
    RingBuffer* rb = init_ring_buffer(PACKET_VIDEO, MAX_RING_BUFFER_SIZE + 1, NULL);
    EXPECT_TRUE(rb == NULL);
    check_stdout_line(LOG_ERROR_MATCHER);
}

// Tests adding packets into ringbuffer
TEST_F(CaptureStdoutTest, AddingPacketsToRingBuffer) {
    // initialize ringbuffer
    const size_t num_packets = 1;
    RingBuffer* rb = init_ring_buffer(PACKET_VIDEO, num_packets, NULL);

    // setup packets to add to ringbuffer
    // note that = {} in C++ is the same as = {0} in C
    WhistPacket pkt1 = {};
    pkt1.type = PACKET_VIDEO;
    pkt1.id = 0;
    pkt1.index = 0;
    pkt1.is_a_nack = false;
    pkt1.num_indices = 1;
    pkt1.num_fec_indices = 0;

    WhistPacket pkt2 = {};
    pkt2.type = PACKET_VIDEO;
    pkt2.id = 1;
    pkt2.index = 0;
    pkt2.is_a_nack = false;
    pkt2.num_indices = 1;
    pkt2.num_fec_indices = 0;

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
    RingBuffer* rb = init_ring_buffer(PACKET_VIDEO, num_packets, NULL);

    // fill ringbuffer
    WhistPacket pkt1;
    pkt1.type = PACKET_VIDEO;
    pkt1.id = 0;
    pkt1.index = 0;
    pkt1.is_a_nack = false;
    pkt1.payload_size = 0;
    pkt1.num_indices = 1;
    pkt1.num_fec_indices = 0;

    receive_packet(rb, &pkt1);
    reset_frame(rb, get_frame_at_id(rb, pkt1.id));

    EXPECT_EQ(receive_packet(rb, &pkt1), 0);
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
    whist_server_config config;
    int argc = 2;

    char argv0[] = "./server/build64/WhistServer";
    char argv1[] = "--help";
    char* argv[] = {argv0, argv1, NULL};

    int ret_val = server_parse_args(&config, argc, argv);
    EXPECT_EQ(ret_val, 1);

    check_stdout_line(::testing::HasSubstr("Usage:"));
}

#endif

/*
============================
Whist Library Tests
============================
*/

/**
 * logging/logging.c
 **/
TEST_F(CaptureStdoutTest, LoggerTest) {
    whist_init_logger();
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

/**
 * logging/log_statistic.c
 **/
TEST_F(CaptureStdoutTest, LogStatistic) {
    StatisticInfo statistic_info[] = {
        {"TEST1", true, true, false},
        {"TEST2", false, false, true},
        {"TEST3", true, true, false},  // Don't log this. Want to check for "count == 0" condition
    };
    whist_init_logger();
    whist_init_statistic_logger(3, NULL, 2);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("Logging initialized!"));
    check_stdout_line(::testing::HasSubstr("StatisticInfo is NULL"));

    log_double_statistic(0, 10.0);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("all_statistics is NULL"));

    whist_init_statistic_logger(3, statistic_info, 2);
    log_double_statistic(3, 10.0);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("index is out of bounds"));
    log_double_statistic(4, 10.0);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("index is out of bounds"));
    log_double_statistic(0, 10.0);
    log_double_statistic(0, 21.5);
    log_double_statistic(1, 30.0);
    log_double_statistic(1, 20.0);
    whist_sleep(2010);
    log_double_statistic(1, 60.0);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("\"TEST1\" : 15.75"));
    check_stdout_line(::testing::HasSubstr("\"MAX_TEST1\" : 21.50"));
    check_stdout_line(::testing::HasSubstr("\"MIN_TEST1\" : 10.00"));
    check_stdout_line(::testing::HasSubstr("\"TEST2\" : 55.00"));

    destroy_statistic_logger();
    destroy_logger();
}

// Constants used for testing encryption
#define DEFAULT_BINARY_PRIVATE_KEY \
    "\xED\x5E\xF3\x3C\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\x19\xEF"
#define SECOND_BINARY_PRIVATE_KEY "\xED\xED\xED\xED\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\xED\xED"

/**
 * utils/color.c
 **/

TEST(ProtocolTest, WhistColorTest) {
    WhistRGBColor cyan = {0, 255, 255};
    WhistRGBColor magenta = {255, 0, 255};
    WhistRGBColor dark_gray = {25, 25, 25};
    WhistRGBColor light_gray = {150, 150, 150};
    WhistRGBColor whist_purple_rgb = {79, 53, 222};
    WhistYUVColor whist_purple_yuv = {85, 198, 127};

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
    EXPECT_NEAR(yuv_to_rgb(whist_purple_yuv).red, whist_purple_rgb.red, 2);
    EXPECT_NEAR(yuv_to_rgb(whist_purple_yuv).green, whist_purple_rgb.green, 2);
    EXPECT_NEAR(yuv_to_rgb(whist_purple_yuv).blue, whist_purple_rgb.blue, 2);
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
    // whist_sleep(25);
    // double elapsed = get_timer(timer);
    // EXPECT_GE(elapsed, 0.025);
    // EXPECT_LE(elapsed, 0.035);

    // start_timer(&timer);
    // whist_sleep(100);
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
    WhistPacket original_packet = {};

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

    WhistPacket encrypted_packet;
    int encrypted_len = encrypt_packet(&original_packet, original_len, &encrypted_packet,
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    // decrypt packet
    WhistPacket decrypted_packet;

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
    WhistPacket original_packet = {};

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

    WhistPacket encrypted_packet;
    int encrypted_len = encrypt_packet(&original_packet, original_len, &encrypted_packet,
                                       (unsigned char*)DEFAULT_BINARY_PRIVATE_KEY);

    // decrypt packet with differing key
    WhistPacket decrypted_packet;

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

#if !defined(__APPLE__) || \
    (defined(__aarch64__) || defined(__arm64__))  // do run the test on MacOs X86

#ifdef _WIN32

int socketpair(int domain, int type, int protocol, SOCKET socks[2]) {
    union {
        struct sockaddr_in inaddr;

        struct sockaddr addr;
    } a;

    SOCKET listener;
    int e;
    socklen_t addrlen = sizeof(a.inaddr);
    int make_overlapped = 0;
    DWORD flags = (make_overlapped ? WSA_FLAG_OVERLAPPED : 0);
    int reuse = 1;

    if (socks == NULL) {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener == INVALID_SOCKET) return SOCKET_ERROR;

    memset(&a, 0, sizeof(a));
    a.inaddr.sin_family = AF_INET;
    a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.inaddr.sin_port = 0;

    socks[0] = socks[1] = INVALID_SOCKET;

    do {
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse,
                       (socklen_t)sizeof(reuse)) == -1)
            break;

        if (bind(listener, &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR) break;

        memset(&a, 0, sizeof(a));

        if (getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR) break;

        // win32 getsockname may only set the port number, p=0.0005.
        // ( http://msdn.microsoft.com/library/ms738543.aspx ):
        a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.inaddr.sin_family = AF_INET;

        set_timeout(listener, 0);  // non-blocking
        if (listen(listener, 1) == SOCKET_ERROR) break;

        socks[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, flags);
        if (socks[0] == INVALID_SOCKET) break;

        set_timeout(socks[0], 0);  // non-blocking
        if (connect(socks[0], &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR &&
            GetLastError() != WSAEWOULDBLOCK)
            break;

        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(listener, &rset);

        /* let 1 second for the socket to be connected */
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if (select(listener + 1, &rset, NULL, NULL, &tv) <= 0) break;

        socks[1] = accept(listener, NULL, NULL);
        if (socks[1] == INVALID_SOCKET) break;

        set_timeout(socks[0], -1);  // blocking
        closesocket(listener);
        return 0;
    } while (0);

    e = WSAGetLastError();
    closesocket(listener);
    closesocket(socks[0]);
    closesocket(socks[1]);
    WSASetLastError(e);
    return SOCKET_ERROR;
}

#endif

typedef struct {
    SOCKET sockets[2];
    int nreads;
    int nwrite;
    int ntimeout;

    int nreadErrors;
    int nreadBytes;
    int readSocketIndex;
} LocalContext;

static void event_counter_cb(WhistPollsetSource idx, int mask, void* data) {
    LocalContext* ctx = (LocalContext*)data;
    int ret;
    char buf[100];

    // printf("mask=0x%x\n", mask);
    if (mask & WHIST_POLLEVENT_READ) {
        ctx->nreads++;

        ret = recv(ctx->sockets[ctx->readSocketIndex], buf, sizeof(buf), 0);
        if (ret < 0)
            ctx->nreadErrors++;
        else
            ctx->nreadBytes += ret;
    }

    if (mask & WHIST_POLLEVENT_WRITE) ctx->nwrite++;

    if (mask & WHIST_POLLEVENT_TIMEOUT) ctx->ntimeout++;
}

#define MANY_SOCKS 40

typedef struct {
    SOCKET sockets[MANY_SOCKS * 2];
    WhistPollsetSource sources[MANY_SOCKS];
    int nreads;
    int nwrite;
    int ntimeout;

    int nreadErrors;
    int nreadBytes;
    size_t drainSocketIndex;
    WhistPollsetSource lastSource;
} LocalContext2;

static void event_counter_cb2(WhistPollsetSource idx, int mask, void* data) {
    LocalContext2* ctx = (LocalContext2*)data;

    ctx->lastSource = idx;
    if (mask & WHIST_POLLEVENT_READ) {
        char buf[20];

        ctx->nreads++;

        int ret = recv(ctx->sockets[ctx->drainSocketIndex], buf, sizeof(buf), 0);
        if (ret < 0)
            ctx->nreadErrors++;
        else
            ctx->nreadBytes += ret;
    }
}

TEST(ProtocolTest, PollsetTest) {
    WhistPollset* set = whist_pollset_new();
    LocalContext ctx;
    WhistPollsetSource s1_source, s2_source;
    char buf[1000];

    /* setup with a socket pair, we'll use sockets[0] for reading, sockets[1] for writing  */
    memset(&ctx, 0, sizeof(ctx));
    whist_init_networking();
    EXPECT_GE(socketpair(PF_LOCAL, SOCK_STREAM, 0, ctx.sockets), 0);

    s1_source =
        whist_pollset_add(set, ctx.sockets[0], WHIST_POLLEVENT_READ, 100, event_counter_cb, &ctx);
    ASSERT_FALSE(s1_source == WHIST_INVALID_SOURCE);

    /* polling 10ms so timeout shall not trigger */
    EXPECT_EQ(whist_pollset_poll(set, 10), 0);
    EXPECT_EQ(ctx.ntimeout, 0);
    EXPECT_EQ(ctx.nreads, 0);
    EXPECT_EQ(ctx.nwrite, 0);

    /* polling a max of 500ms, so we shall wait for ~50ms and trigger the timeout */
    EXPECT_EQ(whist_pollset_poll(set, 500), 1);
    EXPECT_EQ(ctx.ntimeout, 1);

    /* polling 2 times 10ms the timeout has fired before so it shall not trigger again */
    EXPECT_EQ(whist_pollset_poll(set, 10), 0);
    EXPECT_EQ(ctx.ntimeout, 1);

    EXPECT_EQ(whist_pollset_poll(set, 10), 0);
    EXPECT_EQ(ctx.ntimeout, 1);

    /* inject some bytes and check that it's read available */
    EXPECT_EQ(send(ctx.sockets[1], "aa", 2, 0), 2);
    EXPECT_EQ(whist_pollset_poll(set, 200), 1);
    EXPECT_EQ(ctx.nreads, 1);
    EXPECT_EQ(ctx.nreadBytes, 2);
    EXPECT_EQ(ctx.nreadErrors, 0);
    EXPECT_EQ(ctx.ntimeout, 1);

    /* check that write availability is reported correctly on socket[1] */
    s2_source =
        whist_pollset_add(set, ctx.sockets[1], WHIST_POLLEVENT_WRITE, 0, event_counter_cb, &ctx);
    ASSERT_FALSE(s2_source == WHIST_INVALID_SOURCE);

    EXPECT_EQ(whist_pollset_poll(set, 200), 1);
    EXPECT_EQ(ctx.nreads, 1);
    EXPECT_EQ(ctx.nwrite, 1);
    EXPECT_EQ(ctx.ntimeout, 1);

    /* update s2_source to switch to read polling, counters shall not have changed */
    EXPECT_EQ(whist_pollset_update(set, s2_source, WHIST_POLLEVENT_READ, 0, event_counter_cb, &ctx),
              s2_source);
    EXPECT_EQ(whist_pollset_poll(set, 20), 0);
    EXPECT_EQ(ctx.nreads, 1);
    EXPECT_EQ(ctx.nwrite, 1);
    EXPECT_EQ(ctx.ntimeout, 1);

    /* remove s1 which has a timeout, so now nothing shall happen when we poll */
    whist_pollset_remove(set, &s1_source);
    EXPECT_EQ(s1_source, WHIST_INVALID_SOURCE);
    EXPECT_EQ(whist_pollset_poll(set, 100), 0);

    /* inject bytes on sockets[0] */
    EXPECT_EQ(send(ctx.sockets[0], "aaa", 3, 0), 3);
    ctx.readSocketIndex = 1;
    EXPECT_EQ(whist_pollset_poll(set, 20), 1);
    EXPECT_EQ(ctx.nreads, 2);
    EXPECT_EQ(ctx.nreadBytes, 5);

    /* try to fill sockets[0] until we block write, then test write availability */
    set_timeout(ctx.sockets[1], 0);
    while (send(ctx.sockets[1], "aaa", 3, 0) > 0)
        ;

    /* socket is not write available */
    EXPECT_EQ(
        whist_pollset_update(set, s2_source, WHIST_POLLEVENT_WRITE, 0, event_counter_cb, &ctx),
        s2_source);
    EXPECT_EQ(whist_pollset_poll(set, 20), 0);

    /* let's give some air by pulling some bytes in sockets[0], and then write availability should
     * be back */
    set_timeout(ctx.sockets[0], 0);
    while (recv(ctx.sockets[0], buf, sizeof(buf), 0) > 0)
        ;
    EXPECT_EQ(whist_pollset_poll(set, 20), 1);
    EXPECT_EQ(ctx.nwrite, 2);

    WHIST_CLOSE_SOCKET(ctx.sockets[0]);

    whist_pollset_remove(set, &s2_source);
    WHIST_CLOSE_SOCKET(ctx.sockets[1]);

    /*
     * give a try with lots of sockets and sources so that we can test the dynamic
     * arrays in whist_pollset
     */
    LocalContext2 ctx2;
    size_t i;
    memset(&ctx2, 0, sizeof(ctx2));

    for (i = 0; i < MANY_SOCKS; i++) {
        SOCKET socks[2];
        EXPECT_EQ(socketpair(PF_LOCAL, SOCK_STREAM, 0, socks), 0);

        ctx2.sockets[i * 2] = socks[0];
        ctx2.sockets[1 + (i * 2)] = socks[1];

        ctx2.sources[i] =
            whist_pollset_add(set, socks[0], WHIST_POLLEVENT_READ, 0, event_counter_cb2, &ctx2);
        EXPECT_NE(ctx2.sources[i], WHIST_INVALID_SOURCE);
    }

    /* no activity, we shall not have any event */
    EXPECT_EQ(whist_pollset_poll(set, 100), 0);
    EXPECT_EQ(ctx2.nreads, 0);

    /* let's trigger a read event */
    ctx2.drainSocketIndex = 0;
    EXPECT_EQ(send(ctx2.sockets[1], "aa", 2, 0), 2);
    EXPECT_EQ(whist_pollset_poll(set, 1 * 1000), 1);
    EXPECT_EQ(ctx2.nreads, 1);
    EXPECT_EQ(ctx2.nreadBytes, 2);
    EXPECT_EQ(ctx2.lastSource, ctx2.sources[0]);

    /* socket has been drained there should have no more activity */
    EXPECT_EQ(whist_pollset_poll(set, 100), 0);
    EXPECT_EQ(ctx2.nreads, 1);

    /* let's just keep the first and last source */
    for (i = 1; i < MANY_SOCKS - 1; i++) {
        whist_pollset_remove(set, &ctx2.sources[i]);
    }

    EXPECT_EQ(whist_pollset_poll(set, 100), 0);
    EXPECT_EQ(ctx2.nreads, 1);

    /* poll shall still work */
    EXPECT_EQ(send(ctx2.sockets[1], "aa", 2, 0), 2);
    EXPECT_EQ(whist_pollset_poll(set, 1 * 1000), 1);
    EXPECT_EQ(ctx2.nreads, 2);
    EXPECT_EQ(ctx2.nreadBytes, 4);
    EXPECT_EQ(ctx2.lastSource, ctx2.sources[0]);

    /* tear down */
    whist_pollset_remove(set, &ctx2.sources[0]);
    whist_pollset_remove(set, &ctx2.sources[MANY_SOCKS - 1]);

    for (i = 0; i < MANY_SOCKS * 2; i++) WHIST_CLOSE_SOCKET(ctx2.sockets[i]);

    whist_pollset_free(&set);
}

#endif

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

// Test atomics and threads.
// This uses four threads operating simultaneously on atomic variables,
// making sure that the results are consistent with the operations have
// actually happened atomically.

static atomic_int atomic_test_cmpswap;
static atomic_int atomic_test_addsub;
static atomic_int atomic_test_xor;

static int atomic_test_thread(void* arg) {
    int thread = (int)(intptr_t)arg;

    // Compare/swap test.
    // Each thread looks for values equal to its thread number mod 4.  When
    // found, it swaps with a value one higher for the next thread to find.
    // After N iterations each, the final value should be 4N.  This test
    // also causes the four threads to be at roughly the same point when it
    // finishes, to maximise the chance of operations happening
    // simultaneously in the following tests.

    for (int i = thread; i < 64; i += 4) {
        while (1) {
            int expected = i;
            int desired = i + 1;
            bool ret = atomic_compare_exchange_strong(&atomic_test_cmpswap, &expected, desired);
            if (ret) {
                break;
            } else {
                // If the expected value didn't match then the actual value
                // must be lower.  If not, something has gone very wrong.
                EXPECT_LT(expected, i);

                // If this test takes too long because of threads spinning
                // then it might help to add something like sched_yield()
                // here to increase the change that the single thread which
                // can make forward has a chance to run.
            }

            // Attempt to swap in other nearby values which should not work.
            expected = i + 1;
            ret = atomic_compare_exchange_strong(&atomic_test_cmpswap, &expected, desired);
            EXPECT_FALSE(ret);
            expected = i - 4;
            ret = atomic_compare_exchange_weak(&atomic_test_cmpswap, &expected, desired);
            EXPECT_FALSE(ret);
        }
    }

    // Add/sub test.
    // Two of the four threads atomically add to the variable and the
    // other two subtract the same values.  After all have fully run, the
    // variable should be zero again (which can only be tested on the
    // main thread after all others have finished).

    int min = +1000000, max = -1000000;
    for (int i = 0; i < 10000; i++) {
        int old_value;
        switch (thread) {
            case 0:
                old_value = atomic_fetch_add(&atomic_test_addsub, 1);
                break;
            case 1:
                old_value = atomic_fetch_add(&atomic_test_addsub, 42);
                break;
            case 2:
                old_value = atomic_fetch_sub(&atomic_test_addsub, 1);
                break;
            case 3:
                old_value = atomic_fetch_sub(&atomic_test_addsub, 42);
                break;
            default:
                EXPECT_TRUE(false);
                return -1;
        }
        if (old_value < min) min = old_value;
        if (old_value > max) max = old_value;
    }

    // Uncomment to check that the add/sub test is working properly (that
    // operations are actually happening simultaneously).  We can't build
    // in a deterministic check of this because it probably will sometimes
    // run serially anyway.
    // LOG_INFO("Atomic Test Thread %d: min = %d, max = %d", thread, min, max);

    // Xor test.
    // Each thread xors in a sequence of random(ish) numbers, then the
    // same sequence again to cancel it.  The result should be zero.

    int val = 0;
    for (int i = 0; i < 10000; i++) {
        if (i == 5000) val = 0;
        val = val * 7 + 1 + thread;
        atomic_fetch_xor(&atomic_test_xor, val);
    }

    return thread;
}

TEST(ProtocolTest, Atomics) {
    atomic_init(&atomic_test_cmpswap, 0);
    atomic_init(&atomic_test_addsub, 0);
    atomic_init(&atomic_test_xor, 0);

    WhistThread threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] =
            whist_create_thread(&atomic_test_thread, "Atomic Test Thread", (void*)(intptr_t)i);
        EXPECT_FALSE(threads[i] == NULL);
    }

    for (int i = 0; i < 4; i++) {
        int ret;
        whist_wait_thread(threads[i], &ret);
        EXPECT_EQ(ret, i);
    }

    EXPECT_EQ(atomic_load(&atomic_test_addsub), 0);
    EXPECT_EQ(atomic_load(&atomic_test_cmpswap), 64);
    EXPECT_EQ(atomic_load(&atomic_test_xor), 0);
}

TEST(ProtocolTest, FECTest) {
#define NUM_FEC_PACKETS 2

#define NUM_ORIGINAL_PACKETS 2
#define PACKET1_SIZE 97
#define PACKET2_SIZE 90

#define NUM_TOTAL_PACKETS (NUM_ORIGINAL_PACKETS + NUM_FEC_PACKETS)

    char packet1[PACKET1_SIZE] = {0};
    char packet2[PACKET2_SIZE] = {0};
    packet1[0] = 92;
    packet2[PACKET2_SIZE - 1] = 31;

    // ** ENCODING **

    FECEncoder* fec_encoder =
        create_fec_encoder(NUM_ORIGINAL_PACKETS, NUM_FEC_PACKETS, MAX_PACKET_SIZE);

    // Register the original packets
    fec_encoder_register_buffer(fec_encoder, packet1, sizeof(packet1));
    fec_encoder_register_buffer(fec_encoder, packet2, sizeof(packet2));

    // Get the encoded packets
    void* encoded_buffers_tmp[NUM_TOTAL_PACKETS];
    int encoded_buffer_sizes[NUM_TOTAL_PACKETS];
    fec_get_encoded_buffers(fec_encoder, encoded_buffers_tmp, encoded_buffer_sizes);

    // Since destroying the fec encoder drops the void*'s data, we must memcpy it over
    char encoded_buffers[NUM_TOTAL_PACKETS][MAX_PACKET_SIZE];
    for (int i = 0; i < NUM_TOTAL_PACKETS; i++) {
        memcpy(encoded_buffers[i], encoded_buffers_tmp[i], encoded_buffer_sizes[i]);
    }

    // Now we can safely destroy the encoder
    destroy_fec_encoder(fec_encoder);

    // ** DECODING **

    // Now, we decode
    FECDecoder* fec_decoder =
        create_fec_decoder(NUM_ORIGINAL_PACKETS, NUM_FEC_PACKETS, MAX_PACKET_SIZE);

    // Register some sufficiently large subset of the encoded packets
    fec_decoder_register_buffer(fec_decoder, 0, encoded_buffers[0], encoded_buffer_sizes[0]);
    // It's not possible to reconstruct 2 packets, only being given 1 FEC packet
    EXPECT_EQ(fec_get_decoded_buffer(fec_decoder, NULL), -1);
    // Given the FEC packets, it should be possible to reconstruct packet #2
    fec_decoder_register_buffer(fec_decoder, 2, encoded_buffers[2], encoded_buffer_sizes[2]);
    fec_decoder_register_buffer(fec_decoder, 3, encoded_buffers[3], encoded_buffer_sizes[3]);

    // Decode the buffer using FEC
    char decoded_buffer[NUM_ORIGINAL_PACKETS * MAX_PACKET_SIZE];
    int decoded_size = fec_get_decoded_buffer(fec_decoder, decoded_buffer);

    // Confirm that we correctly reconstructed the original data
    EXPECT_EQ(decoded_size, PACKET1_SIZE + PACKET2_SIZE);
    EXPECT_EQ(memcmp(decoded_buffer, packet1, PACKET1_SIZE), 0);
    EXPECT_EQ(memcmp(decoded_buffer + PACKET1_SIZE, packet2, PACKET2_SIZE), 0);
}

/*
============================
Run Tests
============================
*/

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
