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
        remove(filename.c_str());
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

        size_t matcher_idx = 0;
        bool output_to_stderr = ::testing::Test::HasFailure();

        std::string line;
        while (std::getline(file, line)) {
            if (output_to_stderr) {
                fprintf(stderr, "%s\n", line.c_str());
            }

            bool async_matcher_hit = false;
            for (size_t async_idx = 0; async_idx < async_line_matchers.size(); async_idx++) {
                if (Matches(async_line_matchers[async_idx])(line)) {
                    // One of our async matches succeeded, so remove it from the list
                    async_line_matchers.erase(async_line_matchers.begin() + async_idx);
                    async_matcher_hit = true;
                    break;
                }
            }
            if (async_matcher_hit) {
                continue;
            }

            if (matcher_idx < line_matchers.size()) {
                EXPECT_THAT(line, line_matchers[matcher_idx]);
                ++matcher_idx;
            }
        }

        // Expect that no sync matchers are left
        while (matcher_idx < line_matchers.size()) {
            std::ostringstream ss;
            line_matchers[matcher_idx].DescribeTo(&ss);
            ADD_FAILURE() << "Line matcher: no line " << ss.str();
            ++matcher_idx;
        }

        // Expect that no async matchers are left
        for (::testing::Matcher<std::string> async_matcher : async_line_matchers) {
            std::ostringstream ss;
            async_matcher.DescribeTo(&ss);
            ADD_FAILURE() << "Async line matcher: no line " << ss.str();
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

    void expect_stdout_line(::testing::Matcher<std::string> matcher) {
        /*
            This function registers a matcher to be used to compare the output
            of the current test to the expected output, as described in the
            first paragraph of `check_stdout_line()`.

            The difference here is that the matcher registered here will stick
            around until it is satisfied, taking precedence over any matchers
            registered with `check_stdout_line()`.

            This is useful when a log is expected but the order of the logs is
            potentially undefined. For example, if we expect to see the strings
            "1", "2", "3" in that order, but the log "hello" at some
            random point within that list, we would register.

            expect_stdout_line(::testing::HasSubstr("hello"))
            check_stdout_line(::testing::HasSubstr("1"))
            check_stdout_line(::testing::HasSubstr("2"))
            check_stdout_line(::testing::HasSubstr("3"))

            On "1", "2", "hello", "3" in that order, we would first check
            "hello" against "1" and see a failure. Then we would check "1"
            against "1" and see a success. We repeat for "2". Finally, "hello"
            matches and we remove it from the async checks list. We then check
            "3" against "3" and see a success.
        */
        async_line_matchers.push_back(matcher);
    }

    void expect_thread_logs(std::string thread_name) {
        /*
            This function is a convenience function. When our code spins up threads, this can
            generate logs that are potentially out-of-order relating to thread creation and
            destruction. This function registers the appropriate matchers to catch these logs.
        */
        check_stdout_line(
            ::testing::HasSubstr("Creating thread \"" + thread_name + "\" from thread"));
        expect_stdout_line(
            ::testing::HasSubstr("Waiting on thread \"" + thread_name + "\" from thread"));
        expect_stdout_line(::testing::HasSubstr("Waited from thread"));
        check_stdout_line(::testing::HasSubstr("Created from thread"));
    }

    int old_stdout;
    int fd;
    std::string path;
    std::string filename;
    std::vector<::testing::Matcher<std::string>> line_matchers;
    std::vector<::testing::Matcher<std::string>> async_line_matchers;
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
 * Extra macro to expect a success return code.
 */
#define EXPECT_SUCCESS(val) \
    EXPECT_PRED_FORMAT2(::testing::internal::EqHelper::Compare, val, WHIST_SUCCESS)

#endif  // TEST_FIXTURES_HPP
