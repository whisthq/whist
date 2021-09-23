#include <iostream>
#include "gtest/gtest.h"

extern "C" {
    #include "client/client_utils.h"
    #include "fractal/utils/color.h"
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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
