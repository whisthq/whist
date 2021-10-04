#include <iostream>
#include "gtest/gtest.h"

extern "C" {
    #include <fractal/core/fractal.h>
    #include "client/client_utils.h"
    #include "client/ringbuffer.h"
    #include "fractal/utils/color.h"
}
// Include paths should be relative to the protocol folder
//      Examples:
//      - To include file.h in protocol folder, use #include "file.h"
//      - To include file2.h in protocol/client folder, use #include "client/file.h"
// To include a C source file, you need to wrap the include statement in extern "C" {}.

#define NUM_AUDIO_TEST_FRAMES 25
#define MAX_RING_BUFFER_SIZE 500

// Below are a few sample tests for illustration purposes.

TEST(ClientTest, EqualityTest) {
    int i = 3;
    int j = 3;
    EXPECT_EQ(i, j);
}

TEST(ClientTest, NotEqualityTest) {
    int i = 150;
    int j = 350;
    EXPECT_NE(i,j);
}

// Example of a test using a function from the client module
TEST(ClientTest, ArgParsingEmptyArgsTest) {
    int argc = 1;

    char argv0[] = "./client/build64/FractalClient";
    char *argv[] = {argv0, NULL};

    int ret_val = client_parse_args(argc,argv);
    EXPECT_EQ(ret_val,-1);
}

TEST(ClientTest, FractalColorTest) {
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

TEST(ClientTest, TimersTest) {
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


<<<<<<< HEAD
TEST(ClientTest, BitArrayMemCpyTest) {
    #define MAX_RING_BUFFER_SIZE 300
    // A bunch of prime numbers + {10,100,200,250,299,300}
    std::vector<int> bitarray_sizes {1, 2, 3, 5, 7, 10, 11, 13, 17, 19, 23, 29, 31, 37, 41, 47, 53, 100, 250, 299, 300};

    for (auto test_size : bitarray_sizes) {
        BitArray *bit_arr = bit_array_create(test_size);
        EXPECT_TRUE(bit_arr);

        bit_array_clear_all(bit_arr);
        for (int i=0; i<test_size; i++) {
            EXPECT_EQ(bit_array_test_bit(bit_arr, i), 0);
        }

        std::vector<bool>bits_arr_check;

        for (int i=0; i<test_size; i++) {
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

        BitArray *bit_arr_recovered = bit_array_create(test_size);
        EXPECT_TRUE(bit_arr_recovered);

        EXPECT_TRUE(bit_arr_recovered->array);
        EXPECT_TRUE(bit_arr_recovered->array != NULL);
        EXPECT_TRUE(ba_raw);
        EXPECT_TRUE(ba_raw != NULL);
        memcpy(bit_array_get_bits(bit_arr_recovered), ba_raw, BITS_TO_CHARS(test_size));

        for (int i=0; i<test_size; i++) {
            if (bits_arr_check[i]) {
                EXPECT_GE(bit_array_test_bit(bit_arr_recovered, i), 1);
            } else {
                EXPECT_EQ(bit_array_test_bit(bit_arr_recovered, i), 0);
            }   
        }

        bit_array_free(bit_arr_recovered);
    }
}

=======
/** ringbuffer.c **/


//Tests that an initialized ring buffer is correct size and has
//frame IDs initialized to -1
TEST(ClientTest, InitRingBuffer) {
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, NUM_AUDIO_TEST_FRAMES);

    EXPECT_EQ(rb->ring_buffer_size, NUM_AUDIO_TEST_FRAMES);
    EXPECT_EQ(rb->currently_rendering_id, -1);
    EXPECT_EQ(rb->currently_rendering_frame.id, get_frame_at_id(rb, -1)->id);

    for(int frame_num = 0; frame_num < NUM_AUDIO_TEST_FRAMES; frame_num++)
        EXPECT_EQ(rb->receiving_frames[frame_num].id, -1);
    
    destroy_ring_buffer(rb);
}

//Tests that an initialized ring buffer with a bad size returns NULL 
TEST(ClientTest, InitRingBufferBadSize) {
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, MAX_RING_BUFFER_SIZE+1);
    EXPECT_TRUE(rb == NULL);
}

//Tests adding packets into ringbuffer
TEST(ClientTest, AddingPacketsToRingBuffer) {
    //initialize ringbuffer
    const size_t num_packets = 1;
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, num_packets);

    //setup packets to add to ringbuffer
    FractalPacket pkt1;
    pkt1.type = PACKET_VIDEO;
    pkt1.id = 0;
    pkt1.index = 0;
    pkt1.is_a_nack = false;

    FractalPacket pkt2;
    pkt2.type = PACKET_VIDEO;
    pkt2.id = 1;
    pkt2.index = 0;
    pkt2.is_a_nack = false;


    //checks that everything goes well when adding to an empty ringbuffer
    EXPECT_EQ(receive_packet(rb, &pkt1), 0);
    EXPECT_EQ(get_frame_at_id(rb, pkt1.id)->id, pkt1.id);

    //checks that 1 is returned when overwriting a valid frame
    EXPECT_EQ(receive_packet(rb, &pkt2), 1);
    EXPECT_EQ(get_frame_at_id(rb, pkt2.id)->id, pkt2.id);

    //check that -1 is returned when we get a duplicate
    EXPECT_EQ(receive_packet(rb, &pkt2), -1);

    destroy_ring_buffer(rb);
}

//Test that resetting the ringbuffer resets the values
TEST(ClientTest, ResetRingBufferFrame) {
    //initialize ringbuffer
    const size_t num_packets = 1;
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, num_packets);

    //fill ringbuffer
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

//Test that set_rendering works
TEST(ClientTest, SetRenderingTest) {
    //initialize ringbuffer
    const size_t num_packets = 1;
    RingBuffer* rb = init_ring_buffer(FRAME_VIDEO, num_packets);

    set_rendering(rb, 5);
    EXPECT_EQ(rb->currently_rendering_id, 5);

    set_rendering(rb, -5);
    EXPECT_EQ(rb->currently_rendering_id, -5);
}


>>>>>>> Adding unit tests to ClientTest.cpp for ringbuffer.c, modifying unit tests to FractalLibraryTest.cpp, updating CMakeList.txt to create FractalLibrary unit test executable, update README to include unit testing information, fixing absence of null-check for destroy_ring_buffer, adding negative size check for init_ring_buffer
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
