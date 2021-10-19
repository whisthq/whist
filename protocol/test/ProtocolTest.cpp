

/**
 * Copyright Fractal Computers, Inc. 2021
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
#include "gtest/gtest.h"

extern "C" {
#include "client/client_utils.h"
#include "client/ringbuffer.h"
#include "fractal/utils/color.h"
#include <fractal/core/fractal.h>

#ifndef __APPLE__
#include "server/main.h"
#include "server/parse_args.h"
#endif

#include "client/client_utils.h"
#include "fractal/utils/aes.h"
#include "fractal/utils/png.h"
#include "fractal/utils/avpacket_buffer.h"
}

/*
============================
Example Test
============================
*/

// Example of a test using a function from the client module
TEST(ProtocolTest, ArgParsingEmptyArgsTest) {
    int argc = 1;

    char argv0[] = "./client/build64/FractalClient";
    char* argv[] = {argv0, NULL};

    int ret_val = client_parse_args(argc, argv);
    EXPECT_EQ(ret_val, -1);
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
TEST(ProtocolTest, InitRingBufferBadSize) {
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, MAX_RING_BUFFER_SIZE + 1);
    EXPECT_TRUE(rb == NULL);
}

// Tests adding packets into ringbuffer
TEST(ProtocolTest, AddingPacketsToRingBuffer) {
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

// Test that set_rendering works
TEST(ProtocolTest, SetRenderingTest) {
    // initialize ringbuffer
    const size_t num_packets = 1;
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, num_packets);

    set_rendering(rb, 5);
    EXPECT_EQ(rb->currently_rendering_id, 5);

    set_rendering(rb, -5);
    EXPECT_EQ(rb->currently_rendering_id, -5);
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
TEST(ProtocolTest, ArgParsingUsageArgTest) {
    int argc = 2;

    char argv0[] = "./server/build64/FractalServer";
    char argv1[] = "--help";
    char* argv[] = {argv0, argv1, NULL};

    int ret_val = server_parse_args(argc, argv);
    EXPECT_EQ(ret_val, 1);
}

#endif

/*
============================
Fractal Library Tests
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
TEST(ProtocolTest, BadDecrypt) {
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
}

/**
 *  Only run on macOS and Linux for 2 reaons:
 *  1) There is an encoding difference on Windows that causes the
 *  images to be read differently, thus causing them to fail
 *  2) These tests on Windows add an additional 3-5 minutes for the Workflow
 */
#ifndef _WIN32
// Tests that by converting a PNG to a BMP then converting that back
// to a PNG returns the original image
TEST(ProtocolTest, PngToBmpToPng) {
    // Read in PNG
    std::ifstream png_image("images/image.png", std::ios::binary);

    // copies all data into buffer
    std::vector<unsigned char> png_vec(std::istreambuf_iterator<char>(png_image), {});
    int img_size = (int)png_vec.size();
    char* png_buffer = (char*)&png_vec[0];

    // Convert to BMP
    char* bmp_buffer = new char[img_size];
    EXPECT_FALSE(png_to_bmp(png_buffer, img_size, (char**)&bmp_buffer, &img_size));
    // Convert back to PNG
    char* new_png_data = new char[img_size];
    EXPECT_FALSE(bmp_to_png(bmp_buffer, img_size, (char**)&new_png_data, &img_size));

    // compare for equality
    for (int index = 0; index < img_size; index++)
        EXPECT_EQ(png_buffer[index], new_png_data[index]);

    delete[] bmp_buffer;
    delete[] png_buffer;
}

// Tests that by converting a PNG to a BMP then converting that back
// to a PNG returns the original image
TEST(ProtocolTest, BmpToPngToBmp) {
    // Read in PNG
    std::ifstream bmp_image("images/image.bmp", std::ios::binary);

    // copies all data into buffer
    std::vector<unsigned char> bmp_vec(std::istreambuf_iterator<char>(bmp_image), {});
    int img_size = (int)bmp_vec.size();
    char* bmp_buffer = (char*)&bmp_vec[0];

    // Convert to PNG
    char* png_buffer = new char[img_size];
    EXPECT_FALSE(bmp_to_png(bmp_buffer, img_size, (char**)&png_buffer, &img_size));
    // Convert back to BMP
    char* new_bmp_data = new char[img_size];
    EXPECT_FALSE(png_to_bmp(png_buffer, img_size, (char**)&new_bmp_data, &img_size));

    // compare for equality
    for (int index = 0; index < img_size; index++)
        EXPECT_EQ(bmp_buffer[index], new_bmp_data[index]);

    delete[] png_buffer;
    delete[] new_bmp_data;
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

/*
============================
Run Tests
============================
*/

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
