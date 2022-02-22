#ifndef TEST_FIXTURES_HPP
#define TEST_FIXTURES_HPP

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#error "_CRT_SECURE_NO_WARNINGS must be defined to include this file"
#endif  // _CRT_SECURE_NO_WARNINGS
#endif  // _WIN32

#include <iostream>
#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include <fcntl.h>
#include <whist/core/whist.h>
}

#define TEST_OUTPUT_DIRNAME "test_output"

class CaptureStdoutFixture : public ::testing::Test {
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
        path = std::string(TEST_OUTPUT_DIRNAME) + "/" +
               ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name();
        safe_mkdir(path.c_str());
        filename =
            path + "/" + ::testing::UnitTest::GetInstance()->current_test_info()->name() + ".log";
        safe_remove(filename.c_str());
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
        std::ifstream file(filename.c_str());

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
    std::string path;
    std::string filename;
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

#endif  // TEST_FIXTURES_HPP
