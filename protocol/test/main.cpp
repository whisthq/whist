#include <iostream>
#include "gtest/gtest.h"

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

