#include <iostream>
#include "gtest/gtest.h"

extern "C" {
    #include "client/client_utils.h"
    #include "fractal/utils/color.h"
    #include <lib/bitarray/bitarray.h>

    struct bit_array_t
{
    unsigned char *array;       /* pointer to array containing bits */
    unsigned int numBits;       /* number of bits in array */
};
}
// Include paths should be relative to the protocol folder
//      Examples:
//      - To include file.h in protocol folder, use #include "file.h"
//      - To include file2.h in protocol/client folder, use #include "client/file.h"
// To include a C source file, you need to wrap the include statement in extern "C" {}.


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


TEST(ClientTest, BitArrayMemCpyTest) {
    #define MAX_RING_BUFFER_SIZE 300
    // A bunch of prime numbers + {10,100,200,250,299,300}
    std::vector<int> bitarray_sizes {1, 2, 3, 5, 7, 10, 11, 13, 17, 19, 23, 29, 31, 37, 41, 47, 53, 100, 250, 299, 300};

    for (auto test_size : bitarray_sizes) {
        bit_array_t *bit_arr = BitArrayCreate(test_size);
        EXPECT_TRUE(bit_arr);

        BitArrayClearAll(bit_arr);
        for (int i=0; i<test_size; i++) {
            EXPECT_EQ(BitArrayTestBit(bit_arr, i), 0);
        }

        std::vector<bool>bits_arr_check;

        for (int i=0; i<test_size; i++) {
            int coin_toss = rand() % 2;
            EXPECT_TRUE(coin_toss == 0 || coin_toss == 1);
            if (coin_toss) {
                bits_arr_check.push_back(true);
                BitArraySetBit(bit_arr, i);
            } else {
                bits_arr_check.push_back(false);
            }
        }

        unsigned char ba_raw[BITS_TO_CHARS(MAX_RING_BUFFER_SIZE)];
        memcpy(ba_raw, BitArrayGetBits(bit_arr), BITS_TO_CHARS(test_size));
        BitArrayDestroy(bit_arr);

        bit_array_t *bit_arr_recovered = BitArrayCreate(test_size);
        EXPECT_TRUE(bit_arr_recovered);

        EXPECT_TRUE(bit_arr_recovered->array);
        EXPECT_TRUE(bit_arr_recovered->array != NULL);
        EXPECT_TRUE(ba_raw);
        EXPECT_TRUE(ba_raw != NULL);
        memcpy(BitArrayGetBits(bit_arr_recovered), ba_raw, BITS_TO_CHARS(test_size));

        for (int i=0; i<test_size; i++) {
            if (bits_arr_check[i]) {
                EXPECT_GE(BitArrayTestBit(bit_arr_recovered, i), 1);
            } else {
                EXPECT_EQ(BitArrayTestBit(bit_arr_recovered, i), 0);
            }   
        }

        BitArrayDestroy(bit_arr_recovered);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
