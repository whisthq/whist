

/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file protocol_test.cpp
 * @brief This file contains most of the unit tests for the /protocol codebase
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

#include <algorithm>
#include <iterator>
#include <random>
#include <string.h>

#include <gtest/gtest.h>
#include "fixtures.hpp"

extern "C" {
#include <client/sdl_utils.h>
#include "client/client_utils.h"
#include "server/notifications.c"
#include "whist/utils/color.h"
#include <whist/core/whist.h>
#include <whist/utils/os_utils.h>
#include <whist/network/ringbuffer.h>

#include <client/native_window_utils.h>

#ifndef __APPLE__
#include "server/state.h"
#include "server/parse_args.h"
#endif

#include <whist/logging/log_statistic.h>
#include <whist/utils/aes.h>
#include <whist/utils/png.h>
#include <whist/utils/avpacket_buffer.h>
#include <whist/utils/atomic.h>
#include <whist/utils/fec.h>
#include <whist/utils/linked_list.h>

extern WhistMutex window_resize_mutex;
extern volatile SDL_Window* window;
}

class ProtocolTest : public CaptureStdoutFixture {};

/*
============================
Example Test
============================
*/

// Example of a test using a function from the client module
TEST_F(ProtocolTest, ClientParseArgsEmpty) {
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

// Helper function returning a pointer to a newly allocated string of alphanumeric characters, with
// length equal to the length parameter. If length<=0, the function returns NULL
char* generate_random_string(size_t length) {
    const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    if (length <= 0) {
        return NULL;
    }
    char* str = (char*)calloc(length + 1, sizeof(char));
    for (size_t i = 0; i < length; i++) {
        int k = rand() % (int)(sizeof(characters) - 1);
        str[i] = characters[k];
    }
    return str;
}

/**
 * client/sdl_utils.c
 **/

TEST_F(ProtocolTest, InitSDL) {
    char* very_long_title = generate_random_string(2000);
    size_t title_len = strlen(very_long_title);
    EXPECT_EQ(title_len, 2000);
    char icon_filepath[] = "../../../frontend/client-applications/public/icon_dev.png";

    int width = 500;
    int height = 375;

    SDL_Window* new_window = init_sdl(width, height, very_long_title, icon_filepath);

    if (new_window == NULL) {
        // Check if there is no device available to test SDL (e.g. on Ubuntu CI)
        const char* err = SDL_GetError();
        int res = strcmp(err, "No available video device");
        if (res == 0) {
            check_stdout_line(
                ::testing::HasSubstr("Could not initialize SDL - No available video device"));
            free(very_long_title);
            return;
        }
    }

    EXPECT_TRUE(new_window != NULL);

    check_stdout_line(::testing::HasSubstr("all_statistics is NULL"));
    check_stdout_line(::testing::HasSubstr("all_statistics is NULL"));

#ifdef _WIN32
    check_stdout_line(::testing::HasSubstr("Not implemented on Windows."));
#elif defined(__linux__)
    check_stdout_line(::testing::HasSubstr("Not implemented on X11."));
#endif

    // Check that the initial title was set appropriately
    const char* title = SDL_GetWindowTitle(new_window);
    EXPECT_EQ(strcmp(title, very_long_title), 0);
    free(very_long_title);

    // Check that the screensaver option was enabled
    bool screen_saver_check = SDL_IsScreenSaverEnabled();
    EXPECT_TRUE(screen_saver_check);

    // Ensure that the flags below were successfully set at SDL initialization time
    const uint32_t desired_sdl_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    uint32_t actual_sdl_flags = SDL_WasInit(desired_sdl_flags);
    EXPECT_EQ(actual_sdl_flags, desired_sdl_flags);

    // Check that the dimensions are the desired ones
    int actual_width = get_window_virtual_width(new_window);
    int actual_height = get_window_virtual_height(new_window);

    EXPECT_EQ(actual_width, width);
    EXPECT_EQ(actual_height, height);

    char* very_short_title = generate_random_string(1);
    title_len = strlen(very_short_title);
    EXPECT_EQ(title_len, 1);
    SDL_SetWindowTitle(new_window, very_short_title);

    const char* new_title = SDL_GetWindowTitle(new_window);
    EXPECT_EQ(strcmp(new_title, very_short_title), 0);

    free(very_short_title);

    // Check the update_pending_task_functioning

    window_resize_mutex = whist_create_mutex();
    window = new_window;

    // Window resize
    {
        // Swap height and width (pixel form)
        int temp = width;
        width = height;
        height = temp;

        // Apply window dimension change to SDL window
        SDL_SetWindowSize(new_window, width, height);

        actual_width = get_window_virtual_width(new_window);
        actual_height = get_window_virtual_height(new_window);
        EXPECT_EQ(actual_width, width);
        EXPECT_EQ(actual_height, height);

        width = get_window_pixel_width(new_window);
        height = get_window_pixel_height(new_window);

#ifndef __linux__
        int adjusted_width = width - (width % 8);
        int adjusted_height = height - (height % 2);
#else
        int adjusted_width = width;
        int adjusted_height = height;
#endif

        // Check Whist resize procedure (rounding)
        bool pending_resize_message;
        sdl_utils_check_private_vars(&pending_resize_message, NULL, NULL, NULL, NULL, NULL, NULL,
                                     NULL);
        EXPECT_FALSE(pending_resize_message);

        sdl_renderer_resize_window(width, height);

        char buffer[1000];
        memset(buffer, 0, 1000);
        sprintf(buffer, "Received resize event for %dx%d, currently %dx%d", width, height, width,
                height);
        check_stdout_line(::testing::HasSubstr(buffer));
#ifndef __linux__
        memset(buffer, 0, 1000);
        sprintf(buffer, "Forcing a resize from %dx%d to %dx%d", width, height, adjusted_width,
                adjusted_height);
        check_stdout_line(::testing::HasSubstr(buffer));
#endif
        memset(buffer, 0, 1000);
        sprintf(buffer, "Window resized to %dx%d (Actual %dx%d)", width, height, adjusted_width,
                adjusted_height);
        check_stdout_line(::testing::HasSubstr(buffer));

        memset(buffer, 0, 1000);
        sprintf(buffer, "Sending MESSAGE_DIMENSIONS: output=%dx%d", adjusted_width,
                adjusted_height);
        check_stdout_line(::testing::HasSubstr(buffer));

        sdl_utils_check_private_vars(&pending_resize_message, NULL, NULL, NULL, NULL, NULL, NULL,
                                     NULL);
        EXPECT_TRUE(pending_resize_message);
        sdl_update_pending_tasks();

        sdl_utils_check_private_vars(&pending_resize_message, NULL, NULL, NULL, NULL, NULL, NULL,
                                     NULL);
        EXPECT_FALSE(pending_resize_message);

        // New dimensions should ensure width is a multiple of 8 and height is a even number
        actual_width = get_window_pixel_width(new_window);
        actual_height = get_window_pixel_height(new_window);
        EXPECT_EQ(actual_width, adjusted_width);
        EXPECT_EQ(actual_height, adjusted_height);
    }

    //  Titlebar color change
    {
        std::ranlux48 gen;
        std::uniform_int_distribution<unsigned int> uniform_0_255(0, 255);

        WhistRGBColor c;
        c.red = (uint8_t)uniform_0_255(gen);
        c.green = (uint8_t)uniform_0_255(gen);
        c.blue = (uint8_t)uniform_0_255(gen);

        bool native_window_color_update;
        sdl_utils_check_private_vars(NULL, NULL, NULL, &native_window_color_update, NULL, NULL,
                                     NULL, NULL);

        EXPECT_FALSE(native_window_color_update);
        sdl_render_window_titlebar_color(c);

        WhistRGBColor new_color;
        bool native_window_color_is_null;
        sdl_utils_check_private_vars(NULL, &native_window_color_is_null, &new_color,
                                     &native_window_color_update, NULL, NULL, NULL, NULL);

        EXPECT_FALSE(native_window_color_is_null);
        EXPECT_TRUE(native_window_color_update);
        EXPECT_TRUE(new_color.red == c.red);
        EXPECT_TRUE(new_color.blue == c.blue);
        EXPECT_TRUE(new_color.green == c.green);

        set_native_window_color(new_window, c);

#ifdef _WIN32
        check_stdout_line(::testing::HasSubstr("Not implemented on Windows."));
        check_stdout_line(::testing::HasSubstr("Not implemented on Windows."));
#elif defined(__linux__)
        check_stdout_line(::testing::HasSubstr("Not implemented on X11."));
        check_stdout_line(::testing::HasSubstr("Not implemented on X11."));
#endif

        sdl_update_pending_tasks();

        sdl_utils_check_private_vars(NULL, NULL, NULL, &native_window_color_update, NULL, NULL,
                                     NULL, NULL);

        EXPECT_FALSE(native_window_color_update);
    }

    //  Window title
    {
        char* changed_title = generate_random_string(150);
        title_len = strlen(changed_title);
        EXPECT_EQ(title_len, 150);
        bool should_update_window_title;

        sdl_utils_check_private_vars(NULL, NULL, NULL, NULL, NULL, &should_update_window_title,
                                     NULL, NULL);
        EXPECT_FALSE(should_update_window_title);

        sdl_set_window_title(changed_title);
        char* window_title = (char*)calloc(2048, sizeof(char));
        sdl_utils_check_private_vars(NULL, NULL, NULL, NULL, window_title,
                                     &should_update_window_title, NULL, NULL);
        EXPECT_TRUE(should_update_window_title);
        EXPECT_EQ(strcmp(changed_title, window_title), 0);

        const char* old_title = SDL_GetWindowTitle(new_window);
        EXPECT_FALSE(strcmp(old_title, changed_title) == 0);

        sdl_update_pending_tasks();
        sdl_utils_check_private_vars(NULL, NULL, NULL, NULL, NULL, &should_update_window_title,
                                     NULL, NULL);

        EXPECT_FALSE(should_update_window_title);
        const char* changed_title2 = SDL_GetWindowTitle(new_window);
        EXPECT_EQ(strcmp(changed_title, changed_title2), 0);
        free(window_title);
        free(changed_title);
    }

    // Set fullscreen
    {
        width = get_window_pixel_width(new_window);
        height = get_window_pixel_height(new_window);

        bool fullscreen_trigger, fullscreen_value;
        sdl_utils_check_private_vars(NULL, NULL, NULL, NULL, NULL, NULL, &fullscreen_trigger,
                                     &fullscreen_value);
        EXPECT_FALSE(fullscreen_value);
        sdl_set_fullscreen(true);

        sdl_utils_check_private_vars(NULL, NULL, NULL, NULL, NULL, NULL, &fullscreen_trigger,
                                     &fullscreen_value);

        EXPECT_TRUE(fullscreen_value);
        EXPECT_TRUE(fullscreen_trigger);

        // nothing changed yet
        actual_width = get_window_pixel_width(new_window);
        actual_height = get_window_pixel_height(new_window);
        EXPECT_EQ(actual_width, width);
        EXPECT_EQ(actual_height, height);

        sdl_update_pending_tasks();

        sdl_utils_check_private_vars(NULL, NULL, NULL, NULL, NULL, NULL, &fullscreen_trigger,
                                     &fullscreen_value);
        EXPECT_FALSE(fullscreen_trigger);

        actual_width = get_window_virtual_width(new_window);
        actual_height = get_window_virtual_height(new_window);

        int full_width = get_virtual_screen_width();
        int full_height = get_virtual_screen_height();

        EXPECT_EQ(actual_width, full_width);
        EXPECT_EQ(actual_height, full_height);
    }

    destroy_sdl(new_window);
    whist_destroy_mutex(window_resize_mutex);

    check_stdout_line(::testing::HasSubstr("Destroying SDL"));
}

// Test network calls ignoring EINTR.

// Not relevant on Windows, and we need pthread_kill() for the test.
// This should run on macOS, but the CI instances do not run with
// sufficiently consistent timing for the test to always pass.
#if !defined(_WIN32) && !defined(__APPLE__)

typedef struct {
    WhistSemaphore semaphore;
    bool write;
    int fd;
    bool kill;
    pthread_t kill_target;
    int kill_signal;
    bool finish;
} IntrThread;

static int test_intr_thread(void* arg) {
    IntrThread* intr = (IntrThread*)arg;

    while (1) {
        whist_wait_semaphore(intr->semaphore);
        if (intr->finish) break;

        // Wait a short time before performing the requested operation,
        // because the parent thread has to trigger this before it
        // actually enters the state where we want something to happen.
        if (intr->kill) {
            whist_sleep(250);
            pthread_kill(intr->kill_target, intr->kill_signal);
            intr->kill = false;
        }
        if (intr->write) {
            whist_sleep(250);
            char tmp = 42;
            write(intr->fd, &tmp, 1);
            intr->write = false;
        }
    }

    return 0;
}

static atomic_int recv_intr_count = ATOMIC_VAR_INIT(0);
static void test_intr_handler(int signal) { atomic_fetch_add(&recv_intr_count, 1); }

TEST_F(ProtocolTest, RecvNoIntr) {
    int ret, err;
    int socks[2];
    char buf[2];
    WhistTimer timer;
    double elapsed;

    // Set the signal action to a trivial handler so can see when an
    // interrupt happens.
    struct sigaction sa = {0}, old_sa;
    sa.sa_handler = &test_intr_handler;
    ret = sigaction(SIGHUP, &sa, &old_sa);
    EXPECT_EQ(ret, 0);

    // Use a pair of local sockets.  These are used as a pipe, reading
    // from socks[0] and writing to socks[1].
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socks);
    EXPECT_EQ(ret, 0);

    // A separate thread to do the interrupting.  We can't trigger the
    // events from our own thread because we will be calling recv() at
    // the time, so we need another thread to do it after a short delay
    // to allow the recv() to start.
    IntrThread intr = {0};
    intr.semaphore = whist_create_semaphore(0);
    WhistThread thr;
    thr = whist_create_thread(&test_intr_thread, "Intr Test Thread", &intr);
    EXPECT_TRUE(thr);

    // Test recv() working normally.
    intr.write = true;
    intr.fd = socks[1];
    whist_post_semaphore(intr.semaphore);
    ret = recv(socks[0], buf, 1, 0);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(atomic_load(&recv_intr_count), 0);

    // Test that EINTR is happening as expected.
    intr.kill = true;
    intr.kill_target = pthread_self();
    intr.kill_signal = SIGHUP;
    whist_post_semaphore(intr.semaphore);
    ret = recv(socks[0], buf, 1, 0);
    err = errno;
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(err, EINTR);
    EXPECT_EQ(atomic_load(&recv_intr_count), 1);

    // Test that EINTR doesn't happen when we don't want it to.
    intr.kill = true;
    intr.kill_target = pthread_self();
    intr.kill_signal = SIGHUP;
    intr.write = true;
    intr.fd = socks[1];
    whist_post_semaphore(intr.semaphore);
    ret = recv_no_intr(socks[0], buf, 1, 0);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(atomic_load(&recv_intr_count), 2);

    // Same test with recvfrom() this time.
    intr.kill = true;
    intr.kill_target = pthread_self();
    intr.kill_signal = SIGHUP;
    intr.write = true;
    intr.fd = socks[1];
    whist_post_semaphore(intr.semaphore);
    struct sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    ret = recvfrom_no_intr(socks[0], buf, 1, 0, &addr, &addr_len);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(atomic_load(&recv_intr_count), 3);

    // Test that EINTR on a socket with a timeout respects the timeout.
    set_timeout(socks[0], 500);
    start_timer(&timer);
    intr.kill = true;
    intr.kill_target = pthread_self();
    intr.kill_signal = SIGHUP;
    whist_post_semaphore(intr.semaphore);
    ret = recv_no_intr(socks[0], buf, 1, 0);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EAGAIN);
    EXPECT_EQ(atomic_load(&recv_intr_count), 4);
    elapsed = get_timer(&timer);
    EXPECT_GE(elapsed, 0.5);
    EXPECT_LT(elapsed, 1.0);

    // Test that receive with timeout works after EINTR.
    set_timeout(socks[0], 1000);
    start_timer(&timer);
    intr.kill = true;
    intr.kill_target = pthread_self();
    intr.kill_signal = SIGHUP;
    intr.write = true;
    intr.fd = socks[1];
    whist_post_semaphore(intr.semaphore);
    ret = recv_no_intr(socks[0], buf, 1, 0);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(atomic_load(&recv_intr_count), 5);
    elapsed = get_timer(&timer);
    EXPECT_GE(elapsed, 0.5);
    EXPECT_LT(elapsed, 1.0);

    // Test that EINTR does not reset the timeout.
    set_timeout(socks[0], 300);
    start_timer(&timer);
    intr.kill = true;
    intr.kill_target = pthread_self();
    intr.kill_signal = SIGHUP;
    intr.write = true;
    intr.fd = socks[1];
    whist_post_semaphore(intr.semaphore);
    ret = recv_no_intr(socks[0], buf, 1, 0);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EAGAIN);
    EXPECT_EQ(atomic_load(&recv_intr_count), 6);
    elapsed = get_timer(&timer);
    EXPECT_GE(elapsed, 0.3);
    EXPECT_LT(elapsed, 0.5);

    // Clean up thread.
    intr.finish = true;
    whist_post_semaphore(intr.semaphore);
    whist_wait_thread(thr, &ret);
    EXPECT_EQ(ret, 0);

    whist_destroy_semaphore(intr.semaphore);
    close(socks[0]);
    close(socks[1]);

    // Restore the old signal action, since other tests might want it.
    sigaction(SIGHUP, &old_sa, NULL);
}
#endif

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
TEST_F(ProtocolTest, ServerParseArgsUsage) {
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
TEST_F(ProtocolTest, LoggerTest) {
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

    // Check that very long lines are truncated.
    char very_long_line[LOGGER_BUF_SIZE * 2];
    memset(very_long_line, 'X', sizeof(very_long_line));
    very_long_line[sizeof(very_long_line) - 1] = '\0';
    LOG_INFO("%s", very_long_line);

    // Check that line continuations work as expected.
    LOG_INFO("First\nSecond\nThird");

    // Check escape codes.
    LOG_INFO("B\b F\f R\r T\t Q\" B\\");

    // Check non-ASCII.
    LOG_INFO("T\xc3\xa9st");
    LOG_INFO("\xf0\x9f\x98\xb1");

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
    check_stdout_line(::testing::EndsWith("XXX..."));
    check_stdout_line(::testing::EndsWith("| First"));
    check_stdout_line(::testing::EndsWith("|    Second"));
    check_stdout_line(::testing::EndsWith("|    Third"));
    check_stdout_line(::testing::EndsWith("B\\b F\\f R\\r T\\t Q\" B\\"));
    check_stdout_line(::testing::EndsWith("T\xc3\xa9st"));
    check_stdout_line(::testing::EndsWith("\xf0\x9f\x98\xb1"));
}

// Test for log overflow.
TEST_F(ProtocolTest, LoggerOverflowTest) {
    whist_init_logger();

    test_set_pause_state_on_logger_thread(true);

    flush_logs();

    // Log twice as many lines as the queue has.
    for (int i = 0; i < 2 * LOGGER_QUEUE_SIZE; i++) {
        LOG_INFO("Test %d", i);
    }

    test_set_pause_state_on_logger_thread(false);

    destroy_logger();

    check_stdout_line(::testing::HasSubstr("Logging initialized!"));

    // We should have the last LOGGER_QUEUE_SIZE messages along with the
    // overflow notifications (the older messages are discarded).
    for (int i = 0; i < LOGGER_QUEUE_SIZE; i++) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "Test %d", i + LOGGER_QUEUE_SIZE);
        check_stdout_line(::testing::EndsWith(tmp));
        check_stdout_line(::testing::EndsWith("Log buffer overflowing!"));
    }
}

/**
 * logging/log_statistic.c
 **/
TEST_F(ProtocolTest, LogStatistic) {
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

/**
 * utils/color.c
 **/

TEST_F(ProtocolTest, WhistColorTest) {
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

TEST_F(ProtocolTest, TimersTest) {
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
TEST_F(ProtocolTest, EncryptAndDecrypt) {
    const char* data = "testing...testing";
    size_t len = strlen(data);

    // Construct test packet
    WhistPacket original_packet = {};

    // Contruct packet metadata
    original_packet.id = -1;
    original_packet.type = PACKET_MESSAGE;
    original_packet.payload_size = (int)len;

    // Copy packet data
    memcpy(original_packet.data, data, len);
    int original_len = get_packet_size(&original_packet);

    // The encryption data
    AESMetadata aes_metadata;
    char encrypted_data[sizeof(original_packet) + MAX_ENCRYPTION_SIZE_INCREASE];

    // Encrypt
    int encrypted_len = encrypt_packet(encrypted_data, &aes_metadata, &original_packet,
                                       original_len, DEFAULT_BINARY_PRIVATE_KEY);

    // The packet after being decrypted
    WhistPacket decrypted_packet;

    // Decrypt, using the encryption data
    int decrypted_len = decrypt_packet(&decrypted_packet, sizeof(decrypted_packet), aes_metadata,
                                       encrypted_data, encrypted_len, DEFAULT_BINARY_PRIVATE_KEY);

    // Compare the original and decrypted size and data
    EXPECT_EQ(decrypted_len, original_len);
    EXPECT_EQ(memcmp(&decrypted_packet, &original_packet, original_len), 0);
}

// This test encrypts a packet with one key, then attempts to decrypt it with a differing
// key, confirms that it returns -1
TEST_F(ProtocolTest, BadDecrypt) {
#define SECOND_BINARY_PRIVATE_KEY \
    ((const void*)"\xED\xED\xED\xED\xD7\x28\xD1\x7D\xB8\x06\x45\x81\x42\x8D\xED\xED")

    const char* data = "testing...testing";
    size_t len = strlen(data);

    // Construct test packet
    WhistPacket original_packet = {};

    // Contruct packet metadata
    original_packet.id = -1;
    original_packet.type = PACKET_MESSAGE;
    original_packet.payload_size = (int)len;

    // Copy packet data
    memcpy(original_packet.data, data, len);
    int original_len = get_packet_size(&original_packet);

    // The encryption data
    AESMetadata aes_metadata;
    char encrypted_data[sizeof(original_packet) + MAX_ENCRYPTION_SIZE_INCREASE];

    // Encrypt
    int encrypted_len = encrypt_packet(encrypted_data, &aes_metadata, &original_packet,
                                       original_len, DEFAULT_BINARY_PRIVATE_KEY);

    // The packet after being decrypted
    WhistPacket decrypted_packet;

    // Decrypt, using the encryption data
    int decrypted_len = decrypt_packet(&decrypted_packet, sizeof(decrypted_packet), aes_metadata,
                                       encrypted_data, encrypted_len, SECOND_BINARY_PRIVATE_KEY);

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
TEST_F(ProtocolTest, PngToBmpToPng) {
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
TEST_F(ProtocolTest, BmpToPngToBmp) {
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
TEST_F(ProtocolTest, PacketsToBuffer) {
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

TEST_F(ProtocolTest, BitArrayMemCpyTest) {
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

        char* ba_raw = (char*)safe_malloc(BITS_TO_CHARS(test_size));
        memcpy(ba_raw, bit_array_get_bits(bit_arr), BITS_TO_CHARS(test_size));
        bit_array_free(bit_arr);

        BitArray* bit_arr_recovered = bit_array_create(test_size);
        EXPECT_TRUE(bit_arr_recovered);

        EXPECT_TRUE(bit_arr_recovered->array);
        EXPECT_TRUE(bit_arr_recovered->array != NULL);
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
        free(ba_raw);
    }
}

#ifndef _WIN32
// This test is disabled on Windows for the time being, since UTF-8
// seems to behave differently in MSVC, which causes indefinite hanging
// in our CI. See the implementation of `trim_utf8_string` for a bit
// more context.
TEST_F(ProtocolTest, Utf8Truncation) {
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
        val = (uint32_t)val * 7 + 1 + thread;
        atomic_fetch_xor(&atomic_test_xor, val);
    }

    return thread;
}

TEST_F(ProtocolTest, Atomics) {
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

TEST_F(ProtocolTest, FECTest) {
#define NUM_FEC_PACKETS 2

#define NUM_ORIGINAL_PACKETS 2

#define NUM_TOTAL_PACKETS (NUM_ORIGINAL_PACKETS + NUM_FEC_PACKETS)

#define BUFFER_SIZE (NUM_ORIGINAL_PACKETS * MAX_PACKET_SIZE)

    // Initialize FEC
    init_fec();

    // Initialize a buffer that's NUM_ORIGINAL_PACKETS packets large
    char original_buffer[BUFFER_SIZE] = {0};
    original_buffer[0] = 92;
    original_buffer[BUFFER_SIZE - 1] = 31;

    // ** ENCODING **

    FECEncoder* fec_encoder =
        create_fec_encoder(NUM_ORIGINAL_PACKETS, NUM_FEC_PACKETS, MAX_PACKET_SIZE);

    // Register the original buffer
    fec_encoder_register_buffer(fec_encoder, original_buffer, sizeof(original_buffer));

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
    char decoded_buffer[BUFFER_SIZE];
    int decoded_size = fec_get_decoded_buffer(fec_decoder, decoded_buffer);

    // Confirm that we correctly reconstructed the original data
    EXPECT_EQ(decoded_size, BUFFER_SIZE);
    EXPECT_EQ(memcmp(decoded_buffer, original_buffer, BUFFER_SIZE), 0);

    // Destroy the fec decoder when we're done looking at decoded_buffer
    destroy_fec_decoder(fec_decoder);
}

TEST_F(ProtocolTest, LinkedListTest) {
    typedef struct {
        LINKED_LIST_HEADER;
        int id;
    } TestItem;

    LinkedList list;
    linked_list_init(&list);
    EXPECT_EQ(linked_list_size(&list), 0);

    TestItem items[16];
    for (int i = 0; i < 16; i++) items[i].id = i;

    // Build a short list with the various add functions.
    // [ 1 ]
    linked_list_add_head(&list, &items[1]);
    // [ 1, 3 ]
    linked_list_add_tail(&list, &items[3]);
    // [ 0, 1, 3 ]
    linked_list_add_before(&list, &items[1], &items[0]);
    // [ 0, 1, 2, 3 ]
    linked_list_add_after(&list, &items[1], &items[2]);

    // Verify that the list is in the right order and each element has
    // the correct next and prev pointers.
    linked_list_for_each(&list, const TestItem, iter) {
        const TestItem* test;
        // This is clumsy in C++ because it requires an explicit cast.
        test = (const TestItem*)linked_list_prev(iter);
        if (iter->id == 0)
            EXPECT_TRUE(test == NULL);
        else
            EXPECT_EQ(test->id, iter->id - 1);
        test = (const TestItem*)linked_list_next(iter);
        if (iter->id == 3)
            EXPECT_TRUE(test == NULL);
        else
            EXPECT_EQ(test->id, iter->id + 1);
    }
    EXPECT_EQ(linked_list_size(&list), 4);

    // Empty list and create it again with many items.
    linked_list_init(&list);

    for (int i = 0; i < 16; i++) {
        linked_list_add_tail(&list, &items[i]);
    }
    EXPECT_EQ(linked_list_size(&list), 16);

    // Remove odd-numbered items from the list while iterating.
    linked_list_for_each(&list, TestItem, iter) {
        if (iter->id % 2 == 1) linked_list_remove(&list, iter);
    }

    // Verify that the list contains only the even items.
    linked_list_for_each(&list, const TestItem, iter) EXPECT_TRUE(iter->id % 2 == 0);
    EXPECT_EQ(linked_list_size(&list), 8);

    // Remove all other items and make sure the list is empty.
    for (int i = 0; i < 4; i++) {
        TestItem* test;
        test = (TestItem*)linked_list_extract_head(&list);
        EXPECT_EQ(test->id, 2 * i);
        test = (TestItem*)linked_list_extract_tail(&list);
        EXPECT_EQ(test->id, 14 - 2 * i);
    }
    EXPECT_EQ(linked_list_size(&list), 0);
    EXPECT_TRUE(linked_list_extract_head(&list) == NULL);
    EXPECT_TRUE(linked_list_extract_tail(&list) == NULL);
}

// Test notification packager (from string to WhistNotification).
// Ensures no malformed strings, future OOB memory access, etc.
TEST_F(ProtocolTest, PackageNotificationTest) {
    const int add_chars = 30;
    char title[MAX_NOTIF_TITLE_LEN + add_chars] = {0};
    char msg[MAX_NOTIF_MSG_LEN + add_chars] = {0};
    for (int i = 0; i < MAX_NOTIF_TITLE_LEN + add_chars - 1; i++) title[i] = 'A' + rand() % 50;
    for (int i = 0; i < MAX_NOTIF_MSG_LEN + add_chars - 1; i++) msg[i] = 'A' + rand() % 50;

    WhistNotification notif;
    package_notification(&notif, title, msg);

    // Check that the notif is a valid string
    EXPECT_TRUE(strlen(notif.title) < MAX_NOTIF_TITLE_LEN);
    EXPECT_TRUE(strlen(notif.message) < MAX_NOTIF_MSG_LEN);

    std::cerr << strlen(notif.title) << ' ' << MAX_NOTIF_TITLE_LEN << std::endl;
    std::cerr << strlen(notif.message) << ' ' << MAX_NOTIF_MSG_LEN << std::endl;
}

// Test notification display
TEST_F(ProtocolTest, NotificationDisplayTest) {
    WhistNotification notif;
    safe_strncpy(notif.title, "Title of Notification Here", MAX_NOTIF_TITLE_LEN);
    safe_strncpy(notif.message, "Message of Notification Here!", MAX_NOTIF_MSG_LEN);
    int result = display_notification(notif);

#ifdef __APPLE__
    EXPECT_EQ(result, 0);
#else
    EXPECT_EQ(result, -1);
#endif
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
