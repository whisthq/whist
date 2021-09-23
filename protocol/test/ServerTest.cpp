#include <iostream>
#include "gtest/gtest.h"

extern "C" {
#include "server/main.h"
#include "server/parse_args.h"
}
// Include paths should be relative to the protocol folder
//      Examples:
//      - To include file.h in protocol folder, use #include "file.h"
//      - To include file2.h in protocol/server folder, use #include "server/file.h"
// To include a C source file, you need to wrap the include statement in extern "C" {}.


// Below are a few sample tests for illustration purposes.

TEST(ServerTest, EqualityTest) {
    int i = 3;
    int j = 3;
    EXPECT_EQ(i, j);
}

TEST(ServerTest, NotEqualityTest) {
    int i = 150;
    int j = 350;
    EXPECT_NE(i,j);
}

// Example of a test using a function from the client module
TEST(ServerTest, ArgParsingUsageArgTest) {
    int argc = 2;

    char argv0[] = "./server/build64/FractalServer";
    char argv1[] = "--help";
    char *argv[] = {argv0, argv1, NULL};

    int ret_val = server_parse_args(argc,argv);
    EXPECT_EQ(ret_val,1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

