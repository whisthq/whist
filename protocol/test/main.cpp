#include <iostream>
#include "gtest/gtest.h"

extern "C" {
    #include "client/client_utils.h"
}
// Include paths should be relative to the protocol folder
//      Examples:
//      - To include file.h in protocol folder, use #include "file.h"
//      - To include file2.h in protocol/client folder, use #include "client/file.h"
// To include a C source file, you need to wrap the include statement in extern "C" {}.



TEST(ClientTest, DumbTest) {
    int i = 3;
    int j = 3;
    std::cout << "About to run dumb test using GoogleTest!" << std::endl;
    EXPECT_EQ(i, j);
}

TEST(ClientTest, EvenDumberTest) {
    int i = 3;
    int j = 4;
    std::cout << "About to run even dumber test using GoogleTest!" << std::endl;
    EXPECT_EQ(i, j);
}

TEST(ClientTest, AnotherTest) {
    int i = 150;
    int j = 350;
    EXPECT_NE(i,j);
}


TEST(ClientTest, ArgParsingEmptyArgsTest) {
    int argc = 1;

    char argv0[] = "./client/build64/FractalClient";
    char *argv[] = {argv0, NULL};

    int ret_val = client_parse_args(argc,argv);
    EXPECT_EQ(ret_val,-1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

