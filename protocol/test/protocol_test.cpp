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
#include <vector>
#include <string.h>

#include <gtest/gtest.h>
#include "fixtures.hpp"

extern "C" {
#include <client/sdl_utils.h>
#include "whist/utils/color.h"
#include <whist/core/whist.h>
#include <whist/utils/os_utils.h>
#include <whist/network/ringbuffer.h>
#include <client/audio.h>

#include <client/native_window_utils.h>

#ifndef __APPLE__
#include "server/state.h"
#include "server/parse_args.h"
#include "server/client.h"
#endif

#include <whist/file/file_synchronizer.h>
#include <whist/logging/log_statistic.h>
#include <whist/utils/aes.h>
#include <whist/utils/png.h>
#include <whist/utils/avpacket_buffer.h>
#include <whist/utils/atomic.h>
#include <whist/utils/linked_list.h>
#include <whist/utils/queue.h>
#include <whist/utils/command_line.h>
#include <whist/utils/linked_list.h>
#include <whist/fec/fec.h>
#include <whist/fec/rs_wrapper.h>

#include "whist/core/error_codes.h"
#include <whist/core/features.h>

extern int output_width;
extern int output_height;
extern WhistMutex window_resize_mutex;
extern volatile SDL_Window* window;
extern volatile char client_hex_aes_private_key[33];
extern unsigned short port_mappings[USHRT_MAX + 1];

bool whist_error_monitor_environment_set(void);
char* get_error_monitor_environment(void);
char* get_error_monitor_session_id(void);
}

int get_debug_console_listen_port();

class ProtocolTest : public CaptureStdoutFixture {};

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
    char icon_filepath[] = PATH_JOIN("assets", "icon_dev.png");

    // These need to be small enough to fit on the screen, but bigger
    // than our set minima from whist/core/whist.h.
    int width = 500;
    int height = 575;
    EXPECT_GE(width, MIN_SCREEN_WIDTH);
    EXPECT_GE(height, MIN_SCREEN_HEIGHT);

    WhistFrontend* frontend = NULL;
    SDL_Window* new_window = init_sdl(width, height, very_long_title, icon_filepath, &frontend);

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

    check_stdout_line(::testing::HasSubstr("SDL: Using renderer: "));
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
    int measured_width, measured_height;
    whist_frontend_get_window_virtual_size(frontend, &measured_width, &measured_height);
    EXPECT_EQ(measured_width, width);
    EXPECT_EQ(measured_height, height);

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

        whist_frontend_get_window_virtual_size(frontend, &measured_width, &measured_height);

        EXPECT_EQ(measured_width, width);
        EXPECT_EQ(measured_height, height);

        whist_frontend_get_window_pixel_size(frontend, &width, &height);

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

        sdl_renderer_resize_window(frontend, width, height);

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
        sdl_update_pending_tasks(frontend);

        sdl_utils_check_private_vars(&pending_resize_message, NULL, NULL, NULL, NULL, NULL, NULL,
                                     NULL);
        EXPECT_FALSE(pending_resize_message);

        // New dimensions should ensure width is a multiple of 8 and height is a even number
        whist_frontend_get_window_pixel_size(frontend, &measured_width, &measured_height);
        EXPECT_EQ(measured_width, adjusted_width);
        EXPECT_EQ(measured_height, adjusted_height);
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

        sdl_update_pending_tasks(frontend);

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

        sdl_update_pending_tasks(frontend);
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
        whist_frontend_get_window_pixel_size(frontend, &width, &height);

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
        whist_frontend_get_window_pixel_size(frontend, &measured_width, &measured_height);
        EXPECT_EQ(measured_width, width);
        EXPECT_EQ(measured_height, height);

        sdl_update_pending_tasks(frontend);

        sdl_utils_check_private_vars(NULL, NULL, NULL, NULL, NULL, NULL, &fullscreen_trigger,
                                     &fullscreen_value);
        EXPECT_FALSE(fullscreen_trigger);

        // TODO: Check that the window is fullscreen by checking the window attributes.
        //       We used to do this by comparing the window size to the screen size,
        //       but this was literally the only place in our codebase where we needed
        //       to compute the screen size, so I opted to remove that function.
    }

    destroy_sdl(new_window, frontend);
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
    expect_thread_logs("Logger Thread");
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

    expect_thread_logs("Logger Thread");
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
    whist_init_logger();
    flush_logs();
    expect_thread_logs("Logger Thread");
    check_stdout_line(::testing::HasSubstr("Logging initialized!"));

    log_double_statistic(VIDEO_E2E_LATENCY, 10.0);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("all_statistics is NULL"));

    whist_init_statistic_logger(2);
    log_double_statistic(NUM_METRICS, 10.0);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("index is out of bounds"));
    log_double_statistic(NUM_METRICS + 1, 10.0);
    flush_logs();
    check_stdout_line(::testing::HasSubstr("index is out of bounds"));
    log_double_statistic(VIDEO_E2E_LATENCY, 10.0);
    log_double_statistic(VIDEO_E2E_LATENCY, 21.5);
    log_double_statistic(VIDEO_FPS_SENT, 30.0);
    log_double_statistic(VIDEO_FPS_SENT, 20.0);
    whist_sleep(2010);
    log_double_statistic(VIDEO_FPS_SENT, 60.0);
    flush_logs();
    // Log format is: "NAME": count,sum,sum_of_squares,sum_with_offset where the offset is the first
    // value, sum_with_offset is just the sum of differences between each value and the offset, and
    // the sum of squares is the sum of all the squares: (val - offset)^2
    check_stdout_line(::testing::HasSubstr("\"VIDEO_FPS_SENT\" : 55.0, \"COUNT\": 2"));
    check_stdout_line(::testing::HasSubstr("\"VIDEO_END_TO_END_LATENCY\" : 15.8, \"COUNT\": 2"));
    check_stdout_line(::testing::HasSubstr("\"MAX_VIDEO_END_TO_END_LATENCY\" : 21.50"));
    check_stdout_line(::testing::HasSubstr("\"MIN_VIDEO_END_TO_END_LATENCY\" : 10.00"));

    destroy_statistic_logger();
    destroy_logger();
}

// Test threaded logging.
// This logs in a tight loop on four threads simultaneously for a second.

static int log_test_thread(void* arg) {
    int thr = (intptr_t)arg;
    uint64_t k;

    WhistTimer timer;
    start_timer(&timer);
    for (k = 0;; k++) {
        LOG_INFO("Thread %d line %" PRIu64 "!", thr, k);
        if (get_timer(&timer) >= 1.0) break;
    }

    return thr;
}

TEST_F(ProtocolTest, LogThreadTest) {
    whist_init_logger();

    WhistThread threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] = whist_create_thread(&log_test_thread, "Log Test Thread", (void*)(intptr_t)i);
        EXPECT_TRUE(threads[i]);
    }

    for (int i = 0; i < 4; i++) {
        int ret;
        whist_wait_thread(threads[i], &ret);
        EXPECT_EQ(ret, i);
    }

    destroy_logger();
}

TEST_F(ProtocolTest, LogRateLimitTest) {
    whist_init_logger();

    for (int i = 0; i <= 30; i++) {
        if (i == 10 || i == 30) {
            whist_sleep(1000);
        }
        LOG_INFO_RATE_LIMITED(1.0, 3, "Test %d", i);
    }

    destroy_logger();

    expect_thread_logs("Logger Thread");

    check_stdout_line(::testing::HasSubstr("Logging initialized!"));
    check_stdout_line(::testing::EndsWith("Test 0"));
    check_stdout_line(::testing::EndsWith("Test 1"));
    check_stdout_line(::testing::EndsWith("Test 2"));
    check_stdout_line(::testing::EndsWith("Test 10"));
    check_stdout_line(::testing::HasSubstr("7 messages suppressed since"));
    check_stdout_line(::testing::EndsWith("Test 11"));
    check_stdout_line(::testing::EndsWith("Test 12"));
    check_stdout_line(::testing::EndsWith("Test 30"));
    check_stdout_line(::testing::HasSubstr("17 messages suppressed since"));
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
 * file/file_synchronizer.c
 **/

// Information regarding the test files we use
#define LARGE_IMAGE_SIZE 3512956

// Based on chunk size, the number of steps it takes to transfer the large image file
constexpr int transfer_steps_large_image() { return LARGE_IMAGE_SIZE / CHUNK_SIZE + 1; }

// Returns true if passed in file paths both exist and hold the same
// contents, false otherwise.
static bool files_are_equal(const char* file_name, const char* file_name_other) {
    FILE* f1 = fopen(file_name, "rb");
    if (f1 == NULL) {
        return false;
    }

    FILE* f2 = fopen(file_name_other, "rb");
    if (f2 == NULL) {
        fclose(f1);
        return false;
    }

    int c1, c2;

    // Loop through byte by byte and check equality
    do {
        c1 = fgetc(f1);
        c2 = fgetc(f2);
        if (c1 != c2) {
            break;
        }
    } while (c1 != EOF && c2 != EOF);

    fclose(f1);
    fclose(f2);

    return c1 == EOF && c2 == EOF;
}

// Does a single file transfer to test the following - whether file synchronizer can initialize
// properly, whether file metadata is set correctly, if linked list is maintained properly, and if
// file read and write happens successfully.
TEST_F(ProtocolTest, FileSynchronizerTestSingleTransfer) {
    // Init file synchronizer with noop so we write the file locally to the working directory
    init_file_synchronizer(FILE_TRANSFER_DEFAULT);

    // Transferring files linked list should be initialized and empty
    LinkedList* transferring_files_list = file_synchronizer_get_transferring_files();
    EXPECT_EQ(linked_list_size(transferring_files_list), 0);

    // Read in a file to begin transfer - this is called when a transfer is initiated.
    const char* read_file_name = PATH_JOIN("assets", "large_image.png");
    file_synchronizer_set_file_reading_basic_metadata(read_file_name, FILE_TRANSFER_DEFAULT, NULL);
    EXPECT_EQ(linked_list_size(transferring_files_list), 1);

    TransferringFile* active_file = (TransferringFile*)linked_list_head(transferring_files_list);
    // Rather than sending them over tcp, we save our file chunks in this array for this test
    FileData* file_chunks[transfer_steps_large_image()];
    FileMetadata* file_metadata;
    FileData* file_close_chunk;

    // Before opening make sure that the chunk cannot be read
    file_synchronizer_read_next_file_chunk(active_file, &file_chunks[0]);
    EXPECT_TRUE(file_chunks[0] == NULL);

    // Begin read and verify that file metadata is correct
    file_synchronizer_open_file_for_reading(active_file, &file_metadata);
    EXPECT_STREQ(file_metadata->filename, "large_image.png");
    EXPECT_EQ(file_metadata->global_file_id, 0);
    EXPECT_EQ(file_metadata->transfer_type, FILE_TRANSFER_DEFAULT);
    EXPECT_EQ(file_metadata->filename_len, strlen("large_image.png"));
    EXPECT_EQ(file_metadata->file_size, LARGE_IMAGE_SIZE);

    // Read chunks from file and verify transferring files list is empty after
    for (int i = 0; i < transfer_steps_large_image(); i++) {
        file_synchronizer_read_next_file_chunk(active_file, &file_chunks[i]);
    }
    // Final read necessary to evict transferring file since we keep it until an empty read
    file_synchronizer_read_next_file_chunk(active_file, &file_close_chunk);
    EXPECT_EQ(linked_list_size(transferring_files_list), 0);

    // Begin write process on this file synchronizer - mirrors that of server/client relation
    file_synchronizer_open_file_for_writing(file_metadata);
    EXPECT_EQ(linked_list_size(transferring_files_list), 1);

    // Write the file chunks - these are usually sent over a tcp channel
    for (int i = 0; i < transfer_steps_large_image(); i++) {
        file_synchronizer_write_file_chunk(file_chunks[i]);
        deallocate_region(file_chunks[i]);
    }
    // Final write necessary to register file close chunk
    file_synchronizer_write_file_chunk(file_close_chunk);
    EXPECT_EQ(linked_list_size(transferring_files_list), 0);

    // Since the FILE_TRANSFER_DEFAULT flag is set - the file is written to the current directory
    const char* write_file_name = PATH_JOIN(".", "large_image.png");

    // Verify that both files are equal
    EXPECT_TRUE(files_are_equal(read_file_name, write_file_name));
    remove(write_file_name);

    destroy_file_synchronizer();
}

// More than enough file chunks for testing many files - 1GB if filled
#define FILE_CHUNK_LIMIT 1000

// Allow the file synchronizer to run multiple files through. Replicate the action seen on
// both client and server ends and ensure that the transferring files list is correct at all
// times and that the files are transferred correctly by checking their equality.
TEST_F(ProtocolTest, FileSynchronizerMultipleFileTransfer) {
    // Since FILE_TRANSFER_DEFAULT is on, writes are saved to '.'
    const int num_files = 3;
    const char* read_files[] = {
        PATH_JOIN("assets", "large_image.png"),
        PATH_JOIN("assets", "image.png"),
        PATH_JOIN("assets", "100-frames-h264.mp4"),
    };
    const char* write_files[] = {
        PATH_JOIN(".", "large_image.png"),
        PATH_JOIN(".", "image.png"),
        PATH_JOIN(".", "100-frames-h264.mp4"),
    };

    init_file_synchronizer(FILE_TRANSFER_DEFAULT);

    // Activate target files for reading
    for (int i = 0; i < num_files; i++) {
        file_synchronizer_set_file_reading_basic_metadata(read_files[i], FILE_TRANSFER_DEFAULT,
                                                          NULL);
    }
    LinkedList* transferring_files_list = file_synchronizer_get_transferring_files();
    EXPECT_EQ(linked_list_size(transferring_files_list), num_files);

    FileData* current_file_chunk;
    // Do not need an exact count of file chunk pointers, just more than enough. Rather than a tcp
    // channel we buffer them here in this test and feed them back to the synchronizer afterwards.
    FileData* file_chunks[FILE_CHUNK_LIMIT];
    FileMetadata* current_file_metadata;
    FileMetadata* file_metadatas[num_files];

    int file_chunk_counter = 0;
    int file_metadata_counter = 0;
    // Read and "send" the files the same way client/server do in their respective tcp threads
    while (linked_list_size(transferring_files_list)) {
        linked_list_for_each(transferring_files_list, TransferringFile, transferring_file) {
            file_synchronizer_read_next_file_chunk(transferring_file, &current_file_chunk);
            if (current_file_chunk == NULL) {
                // If chunk cannot be read, then try opening the file
                file_synchronizer_open_file_for_reading(transferring_file, &current_file_metadata);
                // Ensure that our files are read properly
                EXPECT_TRUE(current_file_metadata != NULL);
                file_metadatas[file_metadata_counter] = current_file_metadata;
                file_metadata_counter++;
            } else {
                file_chunks[file_chunk_counter] = current_file_chunk;
                file_chunk_counter++;
            }
        }
    }

    // Begin writing files - these usually arrive as triggers as a tcp message
    for (int i = 0; i < num_files; i++) {
        file_synchronizer_open_file_for_writing(file_metadatas[i]);
        deallocate_region(file_metadatas[i]);
    }

    // Read in chunks - file synchronizer should handle matching correct chunks to files
    for (int i = 0; i < file_chunk_counter; i++) {
        file_synchronizer_write_file_chunk(file_chunks[i]);
        deallocate_region(file_chunks[i]);
    }

    // Verify our files our equal
    for (int i = 0; i < num_files; i++) {
        EXPECT_TRUE(files_are_equal(read_files[i], write_files[i]));
        remove(write_files[i]);
    }

    destroy_file_synchronizer();
}

// Tests the reset all method - should completely clear transferring files even in the middle of
// transfers
TEST_F(ProtocolTest, FileSynchronizerResetAll) {
    // Init file synchronizer with noop so we write the file locally to the working directory
    init_file_synchronizer(FILE_TRANSFER_DEFAULT);

    LinkedList* transferring_files_list = file_synchronizer_get_transferring_files();
    FileMetadata* file_metadata;

    // Begin reading file
    file_synchronizer_set_file_reading_basic_metadata(PATH_JOIN("assets", "large_image.png"),
                                                      FILE_TRANSFER_DEFAULT, NULL);
    TransferringFile* active_file = (TransferringFile*)linked_list_head(transferring_files_list);
    file_synchronizer_open_file_for_reading(active_file, &file_metadata);

    // Only read half of the available chunks
    FileData* file_chunks[transfer_steps_large_image()];
    for (int i = 0; i < max(transfer_steps_large_image() / 2, 1); i++) {
        file_synchronizer_read_next_file_chunk(active_file, &file_chunks[i]);
    }

    // Reset all files and check that list is cleared
    EXPECT_EQ(linked_list_size(transferring_files_list), 1);
    reset_all_transferring_files();
    EXPECT_EQ(linked_list_size(transferring_files_list), 0);

    destroy_file_synchronizer();
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
    std::ifstream png_image(PATH_JOIN("assets", "image.png"), std::ios::binary);

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
    std::ifstream bmp_image(PATH_JOIN("assets", "image.bmp"), std::ios::binary);

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
    write_avpackets_to_buffer(1, &packets, (uint8_t*)buffer);

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

#ifndef __APPLE__
int client_test_thread(void* raw_client) {
    Client* client = (Client*)raw_client;

    for (int i = 0; i < 10000; i++) {
        // May block
        ClientLock* client_lock = client_active_lock(client);
        if (client_lock == NULL) {
            // Exit when done
            break;
        }
        EXPECT_TRUE(client->is_active);
        if (i % 1700 == 0) {
            // Sometimes start deactivating it
            start_deactivating_client(client);
            EXPECT_TRUE(client->is_deactivating);
        }
        client_active_unlock(client_lock);

        // Should happen quickly
        client_lock = client_active_trylock(client);
        if (client_lock != NULL) {
            EXPECT_TRUE(client->is_active);
            if (i % 2300 == 0) {
                // Sometimes start deactivating it
                start_deactivating_client(client);
                EXPECT_TRUE(client->is_deactivating);
            }
            client_active_unlock(client_lock);
        }
    }

    return 0;
}

TEST_F(ProtocolTest, ClientStruct) {
    // Client has assertions inside,
    // in order to ensure everything is happening correctly

    Client* client = init_client();

    const int num_threads = 4;
    WhistThread threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        threads[i] = whist_create_thread(&client_test_thread, "Client Test Thread", (void*)client);
        EXPECT_FALSE(threads[i] == NULL);
    }

    // Run for 100ms
    for (int i = 0; i < 1000; i++) {
        if (!client->is_active) {
            // Activate the client, if it's not active
            activate_client(client);
            continue;
        }

        if (client->is_deactivating) {
            // Wait for the client to deactivate (Which waits for every clientlock to be unlocked)
            EXPECT_TRUE(client->is_active);
            deactivate_client(client);
            EXPECT_FALSE(client->is_active);
            continue;
        }

        // If nothing interesting happened, check again in 0.1ms
        whist_usleep(0.1 * US_IN_MS);
    }

    if (client->is_active) {
        start_deactivating_client(client);
        deactivate_client(client);
        EXPECT_FALSE(client->is_active);
    }

    permanently_deactivate_client(client);

    // The threads should exit since the client was permanently deactivated
    for (int i = 0; i < num_threads; i++) {
        int ret;
        whist_wait_thread(threads[i], &ret);
        EXPECT_EQ(ret, 0);
    }

    destroy_client(client);
}
#endif

// Dummy nack and stream reset functions

void dummy_nack(SocketContext* socket_context, WhistPacketType frame_type, int id, int index) {
    // Eventually I guess this should record a nack for testing purposes
    return;
}

void dummy_stream_reset(SocketContext* socket_context, WhistPacketType frame_type,
                        int last_failed_id) {
    return;
}

TEST_F(ProtocolTest, RingBufferTest) {
    int size = 275;
    // Initialize a ring buffer
    RingBuffer* video_buffer = init_ring_buffer(PACKET_VIDEO, LARGEST_VIDEOFRAME_SIZE, size, NULL,
                                                dummy_nack, dummy_stream_reset);

    EXPECT_FALSE(video_buffer == NULL);
    EXPECT_EQ(video_buffer->ring_buffer_size, size);
    EXPECT_EQ(video_buffer->type, PACKET_VIDEO);
    EXPECT_EQ(video_buffer->largest_frame_size,
              sizeof(WhistPacket) - MAX_PAYLOAD_SIZE + LARGEST_VIDEOFRAME_SIZE);
    EXPECT_FALSE(video_buffer->receiving_frames == NULL);
    EXPECT_EQ(video_buffer->frames_received, 0);

    int min_id = size + 1;
    int max_id = size + 5;
    int num_frames = max_id - min_id + 1;
    // Let's produce some sample segments starting from 100
    for (int id = min_id; id < max_id + 1; id++) {
        int num_indices = id - (min_id - 1);
        for (int index = 0; index < num_indices; index++) {
            WhistSegment sample_segment = {};
            sample_segment.id = id;
            sample_segment.index = index;
            sample_segment.num_indices = num_indices;
            sample_segment.num_fec_indices = 0;
            sample_segment.segment_size = num_indices;
            sample_segment.is_a_nack = false;
            memset(sample_segment.segment_data, index, sample_segment.segment_size);

            ring_buffer_receive_segment(video_buffer, &sample_segment);
        }
    }

    // Check correct max and min IDs
    EXPECT_EQ(video_buffer->max_id, max_id);
    EXPECT_EQ(video_buffer->min_id, min_id);
    // Check packets and frames received
    EXPECT_EQ(video_buffer->num_packets_received, (num_frames * (num_frames + 1)) / 2);
    EXPECT_EQ(video_buffer->frames_received, num_frames);
    for (int id = min_id; id < max_id + 1; id++) {
        int expected_indices = id - (min_id - 1);
        // check that all frames are ready
        EXPECT_TRUE(is_ready_to_render(video_buffer, id));
        FrameData* frame_data = get_frame_at_id(video_buffer, id);
        // check that frames have been concatenated properly
        EXPECT_EQ(frame_data->frame_buffer_size, expected_indices * expected_indices);
    }

    // send an old frame, check ring buffer doesn't change
    WhistSegment old_segment = {};
    old_segment.id = min_id - size;
    old_segment.index = 0;
    old_segment.num_indices = 1;
    old_segment.segment_size = 1;
    ring_buffer_receive_segment(video_buffer, &old_segment);

    EXPECT_EQ(video_buffer->min_id, min_id);
    EXPECT_EQ(get_frame_at_id(video_buffer, min_id)->id, min_id);

    // render a frame
    int render_id = min_id;
    FrameData* frame_to_render = set_rendering(video_buffer, min_id);
    // check that last_rendered_id is correct
    EXPECT_EQ(video_buffer->last_rendered_id, render_id);
    EXPECT_EQ(video_buffer->currently_rendering_id, render_id);
    EXPECT_EQ(frame_to_render->id, render_id);
    // check that the old frame has been reset
    EXPECT_EQ(get_frame_at_id(video_buffer, render_id)->packet_buffer, (char*)NULL);
    // Check that the id and other info is not cleared up.
    EXPECT_EQ(get_frame_at_id(video_buffer, render_id)->id, render_id);
    EXPECT_NE(get_frame_at_id(video_buffer, render_id)->original_packets_received, 0);

    // reset a stream
    reset_stream(video_buffer, max_id + 1);
    EXPECT_EQ(video_buffer->last_rendered_id, max_id);

    // stale reset - should be a noop
    reset_stream(video_buffer, min_id);
    EXPECT_EQ(video_buffer->last_rendered_id, max_id);

    destroy_ring_buffer(video_buffer);
}

TEST_F(ProtocolTest, FECTest) {
#define NUM_FEC_PACKETS 4

#define NUM_ORIGINAL_PACKETS 4

#define NUM_TOTAL_PACKETS (NUM_ORIGINAL_PACKETS + NUM_FEC_PACKETS)

// make the last packet shorter than others
#define TAIL_SHORTER_SIZE 20

#define BUFFER_SIZE (NUM_ORIGINAL_PACKETS * (MAX_PACKET_SIZE - FEC_HEADER_SIZE) - TAIL_SHORTER_SIZE)

    // the indices remaining after some packets are considered lost
    std::vector<int> remaining_indices = {1, 5};       // an insufficent subset for recovery
    std::vector<int> more_remaining_indices = {7, 6};  // makes a sufficient subset with above

    // Initialize FEC
    // Bypass the test, if it's the case hardware not support
    // which happened on Mac CI
    int fec_init_result = init_fec();
    EXPECT_EQ(fec_init_result, 0);

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

    // Register a not sufficient large subset of the encoded packets
    for (int i = 0; i < (int)remaining_indices.size(); i++) {
        int idx = remaining_indices[i];
        fec_decoder_register_buffer(fec_decoder, idx, encoded_buffers[idx],
                                    encoded_buffer_sizes[idx]);
    }
    // It's not possible to reconstruct 2 packets, only being given 1 FEC packet
    EXPECT_EQ(fec_get_decoded_buffer(fec_decoder, NULL), -1);
    // Given the FEC packets, it should be possible to reconstruct packet #2

    // feed more packets, so that we get a sufficent subset for decodeing
    for (int i = 0; i < (int)more_remaining_indices.size(); i++) {
        int idx = more_remaining_indices[i];
        fec_decoder_register_buffer(fec_decoder, idx, encoded_buffers[idx],
                                    encoded_buffer_sizes[idx]);
    }

    // Decode the buffer using FEC
    char decoded_buffer[BUFFER_SIZE];
    int decoded_size = fec_get_decoded_buffer(fec_decoder, decoded_buffer);

    // Confirm that we correctly reconstructed the original data
    EXPECT_EQ(decoded_size, BUFFER_SIZE);
    EXPECT_EQ(memcmp(decoded_buffer, original_buffer, BUFFER_SIZE), 0);

    // Destroy the fec decoder when we're done looking at decoded_buffer
    destroy_fec_decoder(fec_decoder);
}

TEST_F(ProtocolTest, FECTest2) {
    WhistTimer timer;
    WhistTimer timer2;
    const int verbose_print = 0;

    // Initialize FEC
    // Bypass the test, if it's the case hardware not support
    // which happened on Mac CI
    int fec_init_result = init_fec();
    EXPECT_EQ(fec_init_result, 0);

    // better random generator than rand()
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> dis(1, 1000 * 1000 * 1000);

    // size of each segment of data
    const int segment_size = 1280;
    // how many segmetns of data
    const int num_segments = 256;
    // the size large buffer that is going to feed into fec encoder
    const int large_buffer_size = num_segments * (segment_size - FEC_HEADER_SIZE) - 10;

    // try to decode after filled per this many packets, for robustness of lib
    // with a num >1, we don't try decode imediately, instead we overfill some buffers than
    // necessary for decode.
    const int try_decode_per_fill_num = 3;

    // test all combination of below parameters
    std::vector<double> fec_ratios = {0.1, 0.5};
    std::vector<int> max_group_sizes = {64, 256};
    std::vector<double> max_group_overheads = {5, 10, 20, 40, 999999};

    // save old parameters, so that this test doesn't break other test possibly involving fec
    double saved_max_group_size = rs_wrapper_set_max_group_size(-1);
    double saved_max_group_overhead = rs_wrapper_set_max_group_overhead(-1);

    // enable verbose print of rs_wrapper
    if (verbose_print) {
        rs_wrapper_set_verbose_log(1);
    }

    // for each fec ratio
    for (int idx1 = 0; idx1 < (int)fec_ratios.size(); idx1++) {
        double fec_ratio = fec_ratios[idx1];
        // for each max_group_size
        for (int idx2 = 0; idx2 < (int)max_group_sizes.size(); idx2++) {
            int max_group_size = max_group_sizes[idx2];
            if (verbose_print) {
                fprintf(stderr, "==== current fec_ratio=%f current max_group_size=%d ====\n",
                        fec_ratio, max_group_size);
            }
            // for each max_group_overhead
            for (int idx3 = 0; idx3 < (int)max_group_overheads.size(); idx3++) {
                double max_group_overhead = max_group_overheads[idx3];
                rs_wrapper_set_max_group_overhead(max_group_overhead);

                if (verbose_print) {
                    fprintf(stderr, "----current max_group_overhead=%f----\n", max_group_overhead);
                }

                // generate a buffer of random data
                char buf[large_buffer_size];
                for (int i = 0; i < large_buffer_size; i++) {
                    buf[i] = (signed char)dis(g);
                }

                int num_real_buffers = num_segments;
                int num_fec_buffers = get_num_fec_packets(num_real_buffers, fec_ratio);
                int num_total_buffers = num_real_buffers + num_fec_buffers;

                if (verbose_print) {
                    fprintf(stderr, "real=%d,fec=%d\n", num_real_buffers, num_fec_buffers);
                }

                // create fec encode
                FECEncoder* fec_encoder =
                    create_fec_encoder(num_real_buffers, num_fec_buffers, segment_size);

                // feed buf into encoder
                fec_encoder_register_buffer(fec_encoder, buf, sizeof(buf));

                // allocate buffers for hold the encoded data
                void** encoded_buffers = (void**)malloc(sizeof(void*) * num_total_buffers);
                int* encoded_buffer_sizes = (int*)malloc(sizeof(int) * num_total_buffers);
                int* indices = (int*)malloc(sizeof(int) * num_total_buffers);

                start_timer(&timer);
                // do encoding
                fec_get_encoded_buffers(fec_encoder, encoded_buffers, encoded_buffer_sizes);
                double t = get_timer(&timer);
                if (verbose_print) {
                    fprintf(stderr, "encode used=%f\n", t);
                }

                // create fec decoder
                FECDecoder* fec_decoder =
                    create_fec_decoder(num_real_buffers, num_fec_buffers, segment_size);
                int decoded_size = -1;

                // a buffer for holding decoded data
                char decoded_buffer[large_buffer_size + segment_size];

                // an index array to simulate packet loss and out of order delivery
                for (int i = 0; i < num_total_buffers; i++) {
                    indices[i] = i;
                }

                // shuffle the index array
                for (int i = 0; i < num_total_buffers; i++) {
                    int j = dis(g) % num_total_buffers;
                    int tmp = indices[i];
                    indices[i] = indices[j];
                    indices[j] = tmp;
                }

                // feed buffer into the decoder according to the shuffle, this simulates packet loss
                for (int i = 0; i < num_total_buffers; i++) {
                    int index = indices[i];
                    fec_decoder_register_buffer(fec_decoder, index, encoded_buffers[index],
                                                encoded_buffer_sizes[index]);

                    // try decoder
                    if (i % try_decode_per_fill_num == 0) {
                        start_timer(&timer2);
                        decoded_size = fec_get_decoded_buffer(fec_decoder, decoded_buffer);
                        double t2 = get_timer(&timer2);
                        if (decoded_size != -1) {
                            if (verbose_print) {
                                fprintf(stderr,
                                        "was able to decode after feeding %d buffers, t2=%f\n",
                                        i + 1, t2);
                            }
                            // if decode success, jump out
                            break;
                        }
                    }
                }

                // check if the decoded data is same as original
                EXPECT_EQ(decoded_size, large_buffer_size);
                EXPECT_EQ(memcmp(decoded_buffer, buf, large_buffer_size), 0);

                // destory encoder and decoder
                destroy_fec_decoder(fec_decoder);
                destroy_fec_encoder(fec_encoder);

                // free intermediate buffers
                free(encoded_buffers);
                free(encoded_buffer_sizes);
                free(indices);

                if (verbose_print) {
                    fprintf(stderr, "\n");
                }
            }
            if (verbose_print) {
                fprintf(stderr, "\n");
            }
        }
    }
    // restore the saved value
    rs_wrapper_set_max_group_size(saved_max_group_size);
    rs_wrapper_set_max_group_overhead(saved_max_group_overhead);
}

typedef struct {
    LINKED_LIST_HEADER;
    int id;
} TestItem;

static int test_linked_list_compare(const void* a, const void* b) {
    return ((const TestItem*)a)->id - ((const TestItem*)b)->id;
}

TEST_F(ProtocolTest, LinkedListTest) {
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

    // Construct list with no order.
    for (int i = 0; i < 16; i++) {
        if (i % 2)
            linked_list_add_tail(&list, &items[i]);
        else
            linked_list_add_head(&list, &items[i]);
    }

    // Sort and verify that list is now sorted.
    linked_list_sort(&list, &test_linked_list_compare);
    for (int i = 0; i < 16; i++) {
        TestItem* test;
        test = (TestItem*)linked_list_extract_head(&list);
        EXPECT_EQ(test->id, i);
    }
}

TEST_F(ProtocolTest, QueueTest) {
    int item;
    QueueContext* fifo_queue = fifo_queue_create(sizeof(int), 5);
    EXPECT_TRUE(fifo_queue != NULL);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), -1);
    item = 1;
    EXPECT_EQ(fifo_queue_enqueue_item(fifo_queue, &item), 0);
    item = 2;
    EXPECT_EQ(fifo_queue_enqueue_item(fifo_queue, &item), 0);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), 0);
    EXPECT_EQ(item, 1);
    item = 3;
    EXPECT_EQ(fifo_queue_enqueue_item(fifo_queue, &item), 0);
    item = 4;
    EXPECT_EQ(fifo_queue_enqueue_item(fifo_queue, &item), 0);
    item = 5;
    EXPECT_EQ(fifo_queue_enqueue_item(fifo_queue, &item), 0);
    item = 6;
    EXPECT_EQ(fifo_queue_enqueue_item(fifo_queue, &item), 0);
    item = 7;
    EXPECT_EQ(fifo_queue_enqueue_item(fifo_queue, &item), -1);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), 0);
    EXPECT_EQ(item, 2);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), 0);
    EXPECT_EQ(item, 3);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), 0);
    EXPECT_EQ(item, 4);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), 0);
    EXPECT_EQ(item, 5);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), 0);
    EXPECT_EQ(item, 6);
    EXPECT_EQ(fifo_queue_dequeue_item(fifo_queue, &item), -1);
    EXPECT_EQ(item, 6);
    fifo_queue_destroy(fifo_queue);
    EXPECT_EQ(fifo_queue_dequeue_item(NULL, &item), -1);
    EXPECT_EQ(fifo_queue_enqueue_item(NULL, &item), -1);
}

TEST_F(ProtocolTest, WCCTest) {
    output_width = 1920;
    output_height = 1080;
    network_algo_set_dpi(192);
    whist_set_feature(WHIST_FEATURE_WHIST_CONGESTION_CONTROL, true);
    NetworkSettings network_settings = get_starting_network_settings();
    int expected_video_bitrate = output_width * output_height * STARTING_BITRATE_PER_PIXEL;
    EXPECT_EQ(network_settings.video_bitrate, expected_video_bitrate);
    GroupStats curr_group_stats = {0, 0, 0};
    GroupStats prev_group_stats = {0, 0, 0};
    int incoming_bitrate = expected_video_bitrate * 0.97;
    double packet_loss_ratio = 0.0;

    EXPECT_EQ(network_settings.saturate_bandwidth, true);
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    // No change in bitrates as timers would have just initialized in the first call.
    EXPECT_EQ(network_settings.video_bitrate, expected_video_bitrate);
    EXPECT_EQ(network_settings.saturate_bandwidth, true);

    // Wait for little more than NEW_BITRATE_DURATION_IN_SEC, for WCC to react
    whist_sleep((uint32_t)(NEW_BITRATE_DURATION_IN_SEC * 1.1 * MS_IN_SECOND));
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    expected_video_bitrate *= (1.0 + MAX_INCREASE_PERCENTAGE / 100.0);
    EXPECT_EQ(network_settings.video_bitrate, expected_video_bitrate);
    EXPECT_EQ(network_settings.burst_bitrate, network_settings.video_bitrate);
    EXPECT_EQ(network_settings.saturate_bandwidth, true);

    // Cause congestion to see if WCC reacts.
    incoming_bitrate = 4000000;
    packet_loss_ratio = 0.11;
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    // No change in bitrate as congestion should be present for atleast
    // OVERUSE_TIME_THRESHOLD_IN_SEC
    EXPECT_EQ(network_settings.video_bitrate, expected_video_bitrate);
    whist_sleep(OVERUSE_TIME_THRESHOLD_IN_SEC * 1.1 * MS_IN_SECOND);
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    // Now bitrate should have dropped to something lesser than incoming_bitrate
    EXPECT_LT(network_settings.video_bitrate, incoming_bitrate);
    EXPECT_EQ(network_settings.saturate_bandwidth, true);

    // Once bitrate is decreased due to congestion, next bitrate decrease will happen only after
    // NEW_BITRATE_DURATION_IN_SEC
    incoming_bitrate = 2000000;
    packet_loss_ratio = 0.11;
    expected_video_bitrate = network_settings.video_bitrate;
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    whist_sleep(OVERUSE_TIME_THRESHOLD_IN_SEC * 1.1 * MS_IN_SECOND);
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    EXPECT_EQ(network_settings.video_bitrate, expected_video_bitrate);
    whist_sleep((uint32_t)(NEW_BITRATE_DURATION_IN_SEC * 1.1 * MS_IN_SECOND));
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    EXPECT_LT(network_settings.video_bitrate, incoming_bitrate);
    EXPECT_EQ(network_settings.saturate_bandwidth, true);

    // Make sure min bitrate is capped
    incoming_bitrate = 1000000;
    packet_loss_ratio = 0.11;
    expected_video_bitrate = output_width * output_height * MINIMUM_BITRATE_PER_PIXEL;
    whist_sleep((uint32_t)(NEW_BITRATE_DURATION_IN_SEC * 1.1 * MS_IN_SECOND));
    whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                packet_loss_ratio, &network_settings);
    EXPECT_EQ(network_settings.video_bitrate, expected_video_bitrate);
    EXPECT_EQ(network_settings.burst_bitrate, network_settings.video_bitrate);
    EXPECT_EQ(network_settings.saturate_bandwidth, true);

    // Once congestion clears up WCC should reach max bitrate quickly
    for (int i = 0; i < 13; i++) {
        incoming_bitrate = network_settings.video_bitrate;
        packet_loss_ratio = 0.0;
        expected_video_bitrate = output_width * output_height * MINIMUM_BITRATE_PER_PIXEL;
        whist_sleep((uint32_t)(NEW_BITRATE_DURATION_IN_SEC * 1.1 * MS_IN_SECOND));
        whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                    packet_loss_ratio, &network_settings);
    }
    expected_video_bitrate = output_width * output_height * MAXIMUM_BITRATE_PER_PIXEL;
    EXPECT_EQ(network_settings.video_bitrate, expected_video_bitrate);
    // Make sure the burst bitrate is higher than video bitrate
    EXPECT_GT(network_settings.burst_bitrate, network_settings.video_bitrate);
    EXPECT_EQ(network_settings.saturate_bandwidth, false);

    // Now there is prolonged congestion and WCC should converge to the available bandwidth quickly
    int available_bandwidth = 3000000;
    for (int i = 0; i < 80; i++) {
        if (network_settings.video_bitrate > available_bandwidth) {
            incoming_bitrate = available_bandwidth;
            packet_loss_ratio = 0.11;
        } else {
            incoming_bitrate = network_settings.video_bitrate;
            packet_loss_ratio = 0.0;
        }
        whist_sleep((uint32_t)(NEW_BITRATE_DURATION_IN_SEC * 0.51 * MS_IN_SECOND));
        whist_congestion_controller(&curr_group_stats, &prev_group_stats, incoming_bitrate,
                                    packet_loss_ratio, &network_settings);
    }
    EXPECT_LT(network_settings.video_bitrate, available_bandwidth);
    EXPECT_GT(network_settings.video_bitrate, available_bandwidth * CONVERGENCE_THRESHOLD_LOW);
    EXPECT_EQ(network_settings.saturate_bandwidth, false);
}

TEST_F(ProtocolTest, UnOrderedPacketTest) {
    UnOrderedPacketInfo unordered_info;
    unordered_info.max_unordered_packets = 0.0;
    unordered_info.prev_frame_id = 0;
    unordered_info.prev_packet_index = 0;
    update_max_unordered_packets(&unordered_info, 1, 0);
    EXPECT_EQ(unordered_info.max_unordered_packets, 0.0);
    update_max_unordered_packets(&unordered_info, 1, 1);
    update_max_unordered_packets(&unordered_info, 1, 2);
    EXPECT_EQ(unordered_info.max_unordered_packets, 0.0);
    update_max_unordered_packets(&unordered_info, 2, 0);
    update_max_unordered_packets(&unordered_info, 2, 1);
    EXPECT_EQ(unordered_info.max_unordered_packets, 0.0);
    update_max_unordered_packets(&unordered_info, 2, 3);
    EXPECT_EQ(unordered_info.max_unordered_packets, 0.0);
    update_max_unordered_packets(&unordered_info, 2, 2);
    EXPECT_EQ(unordered_info.max_unordered_packets, 1.0);
    update_max_unordered_packets(&unordered_info, 2, 4);
    EXPECT_TRUE(unordered_info.max_unordered_packets > 0.98 &&
                unordered_info.max_unordered_packets < 1.0);
    update_max_unordered_packets(&unordered_info, 2, 10);
    update_max_unordered_packets(&unordered_info, 2, 5);
    EXPECT_EQ(unordered_info.max_unordered_packets, 5.0);
    update_max_unordered_packets(&unordered_info, 3, 0);
    EXPECT_TRUE(unordered_info.max_unordered_packets > 4.94 &&
                unordered_info.max_unordered_packets < 4.96);
    for (int i = 1; i < 100; i++) {
        update_max_unordered_packets(&unordered_info, 3, i);
    }
    EXPECT_TRUE(unordered_info.max_unordered_packets > 1.8 &&
                unordered_info.max_unordered_packets < 1.9);
    for (int i = 0; i < 100; i++) {
        update_max_unordered_packets(&unordered_info, 4, i);
    }
    EXPECT_TRUE(unordered_info.max_unordered_packets > 0.6 &&
                unordered_info.max_unordered_packets < 0.7);
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

TEST_F(ProtocolTest, AudioTest) {
    // TODO: I've disabled this test because `WhistFrontend*` is not yet in
    //       a state where we can easily initialize it here without spinning
    //       up a window as well.

    // AudioContext* audio_context = init_audio();
    // // no frames buffered: underflowing, need more frames
    // EXPECT_FALSE(audio_ready_for_frame(audio_context, 0));
    // // lots of frames buffered (maxed out ring buffer): ready to render
    // EXPECT_TRUE(audio_ready_for_frame(audio_context, 15));
    // destroy_audio(audio_context);
}

static int test_option_int;
static bool test_option_bool_1, test_option_bool_2, test_option_bool_3;
static const char* test_option_string;
static const char* test_option_callback;
static int test_operand_position;
static const char* test_operand_value;

static WhistStatus test_option_callback_function(const WhistCommandLineOption* opt,
                                                 const char* value) {
    test_option_callback = value;
    return WHIST_SUCCESS;
}

static WhistStatus test_operand_callback_function(int pos, const char* value) {
    test_operand_position = pos;
    test_operand_value = value;
    return WHIST_SUCCESS;
}

// These option names must not conflict with options anywhere else.
COMMAND_LINE_INT_OPTION(test_option_int, '4', "test-int", INT_MIN, INT_MAX,
                        "Integer option for testing.")
COMMAND_LINE_BOOL_OPTION(test_option_bool_1, '5', "test-bool-1", "Boolean option for testing.")
COMMAND_LINE_BOOL_OPTION(test_option_bool_2, '6', "test-bool-2",
                         "Second boolean option for testing.")
COMMAND_LINE_BOOL_OPTION(test_option_bool_3, '7', "test-bool-3",
                         "Third boolean option for testing.")
COMMAND_LINE_STRING_OPTION(test_option_string, '8', "test-string", 16, "String option for testing.")
COMMAND_LINE_CALLBACK_OPTION(test_option_callback_function, '9', "test-callback",
                             WHIST_OPTION_REQUIRED_ARGUMENT, "Callback option for testing.")

TEST_F(ProtocolTest, CommandLineTest) {
    const char* argv[10] = {"command-line-test"};

    // Empty command-line.
    EXPECT_SUCCESS(whist_parse_command_line(1, argv, NULL));

    // Single operand.
    argv[1] = "foo";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, &test_operand_callback_function));
    EXPECT_EQ(test_operand_position, 1);
    EXPECT_STREQ(test_operand_value, "foo");

    // Single integer option in various forms.
    argv[1] = "-417";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_int, 17);
    argv[1] = "--test-int=29";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_int, 29);
    argv[1] = "-4";
    argv[2] = "42";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_EQ(test_option_int, 42);
    argv[1] = "--test-int";
    argv[2] = "53";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_EQ(test_option_int, 53);

    // Also check retrieving the value again.
    int int_value;
    EXPECT_SUCCESS(whist_option_get_int_value("test-int", &int_value));
    EXPECT_EQ(int_value, 53);

    // Single boolean in various forms
    argv[1] = "-5";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_bool_1, true);
    argv[1] = "-5off";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_bool_1, false);
    argv[1] = "-5";
    argv[2] = "TRUE";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_EQ(test_option_bool_1, true);
    argv[1] = "-5";
    argv[2] = "No";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_EQ(test_option_bool_1, false);
    argv[1] = "--test-bool-1";
    argv[2] = "-442";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_EQ(test_option_bool_1, true);
    EXPECT_EQ(test_option_int, 42);
    argv[1] = "--test-bool-1";
    argv[2] = "0";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_EQ(test_option_bool_1, false);

    // Retrieving a boolean value.
    bool bool_value;
    EXPECT_SUCCESS(whist_option_get_bool_value("5", &bool_value));
    EXPECT_EQ(bool_value, false);

    // Multiple boolean flags.
    argv[1] = "-5";
    argv[2] = "-6";
    argv[3] = "-7";
    EXPECT_SUCCESS(whist_parse_command_line(4, argv, NULL));
    EXPECT_EQ(test_option_bool_1, true);
    EXPECT_EQ(test_option_bool_2, true);
    EXPECT_EQ(test_option_bool_3, true);

    // Single string option.
    argv[1] = "-8foo";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_STREQ(test_option_string, "foo");
    argv[1] = "--test-string";
    argv[2] = "bar";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_STREQ(test_option_string, "bar");

    // Retrieving a string value.
    const char* string_value;
    EXPECT_SUCCESS(whist_option_get_string_value("test-string", &string_value));
    EXPECT_STREQ(string_value, "bar");
    EXPECT_EQ(string_value, test_option_string);

    // Callback function.
    argv[1] = "-9hello";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_STREQ(test_option_callback, "hello");
    argv[1] = "--test-callback";
    argv[2] = "world";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_STREQ(test_option_callback, "world");

    // Abbreviated names.
    argv[1] = "--test-str=test";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_STREQ(test_option_string, "test");
    argv[1] = "--test-call";
    argv[2] = "foo";
    EXPECT_SUCCESS(whist_parse_command_line(3, argv, NULL));
    EXPECT_STREQ(test_option_callback, "foo");

    // Different integer formats.
    argv[1] = "--test-int=-1";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_int, -1);
    argv[1] = "--test-int=0x1234";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_int, 0x1234);

    // Out-of-range integers.
    argv[1] = "--test-int=2000000000";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_int, 2000000000);
    argv[1] = "--test-int=3000000000";
    EXPECT_EQ(whist_parse_command_line(2, argv, NULL), WHIST_ERROR_OUT_OF_RANGE);
    argv[1] = "--test-int=-2000000000";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_int, -2000000000);
    argv[1] = "--test-int=-3000000000";
    EXPECT_EQ(whist_parse_command_line(2, argv, NULL), WHIST_ERROR_OUT_OF_RANGE);
    argv[1] = "--test-int=0x7fffffff";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_EQ(test_option_int, 2147483647);
    argv[1] = "--test-int=0x80000000";
    EXPECT_EQ(whist_parse_command_line(2, argv, NULL), WHIST_ERROR_OUT_OF_RANGE);

    // Too-long strings.
    argv[1] = "--test-string=1234567890123456";
    EXPECT_SUCCESS(whist_parse_command_line(2, argv, NULL));
    EXPECT_STREQ(test_option_string, "1234567890123456");
    argv[1] = "--test-string=12345678901234567";
    EXPECT_EQ(whist_parse_command_line(2, argv, NULL), WHIST_ERROR_TOO_LONG);

    // Retrieving a nonexistent value.
    int missing_value;
    int* missing_pointer = &missing_value;
    EXPECT_EQ(whist_option_get_int_value("nonexistent", &missing_value), WHIST_ERROR_NOT_FOUND);
    EXPECT_EQ(missing_pointer, &missing_value);

    // Everything together.
    argv[1] = "--test-int=-17";
    argv[2] = "-6";
    argv[3] = "-7";
    argv[4] = "--test-bool-1";
    argv[5] = "off";
    argv[6] = "--test-str";
    argv[7] = "foo";
    argv[8] = "bar";
    EXPECT_SUCCESS(whist_parse_command_line(9, argv, &test_operand_callback_function));
    EXPECT_EQ(test_option_int, -17);
    EXPECT_EQ(test_option_bool_1, false);
    EXPECT_EQ(test_option_bool_2, true);
    EXPECT_EQ(test_option_bool_3, true);
    EXPECT_STREQ(test_option_string, "foo");
    EXPECT_EQ(test_operand_position, 8);
    EXPECT_STREQ(test_operand_value, "bar");
}

TEST_F(ProtocolTest, ClientParseEmpty) {
    const char* argv[] = {"./wclient"};
    int argc = ARRAY_LENGTH(argv);
    int ret_val = client_parse_args(argc, argv);
    EXPECT_EQ(ret_val, 0);
}

TEST_F(ProtocolTest, ClientParseHelp) {
    const char* argv[] = {"./wclient", "--help"};
    int argc = ARRAY_LENGTH(argv);
    EXPECT_EXIT(client_parse_args(argc, argv), testing::ExitedWithCode(0), "");
    check_stdout_line(::testing::StartsWith("Usage:"));
}

TEST_F(ProtocolTest, ClientParseArgs) {
    const char* argv[] = {
        "./wclient",
        // Required urls: IP, port mappings, AES key
        "16.72.29.190",
        "-p32262:7415.32263:66182.32273:12774",
        "-k",
        "9d3ff73c663e13bce0780d1b95c89582",
        // Enable the debug console on a port
        "--debug-console",
        "9090",
        // Disable catching segfaults on server (developer mode)
        "-D",
        // Enable features
        "--enable-features",
        "packet encryption,long-term reference frames",
        // Set environment to
        "-e",
        "staging",
        // Set width and height
        "-w",
        "500",
        "-h",
        "750",
        // Set the icon (using a 64x64 png file)
        "-i",
        "assets/icon_dev.png",
        // Set the protocol window title
        "-n",
        "Test Title 1234567890 ?!",
        // Pass URL to open up in new tab
        "-x",
        "https://www.nytimes.com/",
        // Add additional port mappings
        "-p",
        "8787:6969.1234:7248",
        // Set the session id
        "-d",
        "2rhfnq384",
        // Request to launch protocol without an icon in the taskbar
        "-s",
        // Pass the user's email
        "-u",
        "user@whist.com",
    };
    int argc = ARRAY_LENGTH(argv);
    int ret_val = client_parse_args(argc, argv);
    EXPECT_EQ(ret_val, 0);

    // Check that the server ip was saved properly.
    const char* server_ip;
    EXPECT_SUCCESS(whist_option_get_string_value("server-ip", &server_ip));
    EXPECT_TRUE(server_ip != NULL);
    EXPECT_STREQ(server_ip, argv[1]);

    // Check the port mappings
    EXPECT_EQ(port_mappings[32262], 7415);
    EXPECT_EQ(port_mappings[32263], 646);
    EXPECT_EQ(port_mappings[32273], 12774);
    check_stdout_line(::testing::HasSubstr("Mapping port: origin=32262, destination=7415"));
    check_stdout_line(::testing::HasSubstr("Mapping port: origin=32263, destination=646"));
    check_stdout_line(::testing::HasSubstr("Mapping port: origin=32273, destination=12774"));

    // Check that the AES key was saved properly. Use a loop (instead of strcmp) because
    // client_hex_aes_private_key is volatile.
    size_t aes_key_len = 0, argv4_len = strlen(argv[4]);
    while (client_hex_aes_private_key[aes_key_len] != '\0') {
        EXPECT_TRUE(aes_key_len <= argv4_len);
        EXPECT_EQ(tolower(client_hex_aes_private_key[aes_key_len]), argv[4][aes_key_len]);
        aes_key_len++;
    }
    EXPECT_EQ(aes_key_len, argv4_len);

    // Check that the debugging console was turned on at the desired port
    int debug_console_port = atoi(argv[6]);
    EXPECT_EQ(get_debug_console_listen_port(), debug_console_port);

    // Check that we are not catching seg faults, given that developer mode is on.
    bool developer_flag;
    EXPECT_SUCCESS(whist_option_get_bool_value("developer-mode", &developer_flag));
    EXPECT_TRUE(developer_flag);
    whist_init_features();

    // Check the environment
    EXPECT_TRUE(whist_error_monitor_environment_set());
    char* environment_copy = get_error_monitor_environment();
    EXPECT_TRUE(environment_copy != NULL);
    EXPECT_STREQ(environment_copy, argv[11]);
    free(environment_copy);

    // Check the width and the height
    int width = atoi(argv[13]);
    int height = atoi(argv[15]);
    EXPECT_EQ(width, output_width);
    EXPECT_EQ(height, output_height);

    // Check that the icon filename was set correctly
    const char* png_icon_filename;
    EXPECT_SUCCESS(whist_option_get_string_value("icon", &png_icon_filename));
    EXPECT_TRUE(png_icon_filename != NULL);
    EXPECT_STREQ(png_icon_filename, argv[17]);

    // Check the window title
    const char* program_name;
    EXPECT_SUCCESS(whist_option_get_string_value("name", &program_name));
    EXPECT_TRUE(program_name != NULL);
    EXPECT_STREQ(program_name, argv[19]);

    // Check that the url was saved properly
    const char* new_tab_url;
    EXPECT_SUCCESS(whist_option_get_string_value("new-tab-url", &new_tab_url));
    EXPECT_TRUE(new_tab_url != NULL);
    EXPECT_STREQ(new_tab_url, argv[21]);

    // Check the additional port mappings
    EXPECT_EQ(port_mappings[8787], 6969);
    EXPECT_EQ(port_mappings[1234], 7248);
    check_stdout_line(::testing::HasSubstr("Mapping port: origin=8787, destination=6969"));
    check_stdout_line(::testing::HasSubstr("Mapping port: origin=1234, destination=7248"));

    // Check that features are enabled
    EXPECT_TRUE(whist_check_feature(WHIST_FEATURE_PACKET_ENCRYPTION));
    EXPECT_TRUE(whist_check_feature(WHIST_FEATURE_LONG_TERM_REFERENCE_FRAMES));
    check_stdout_line(
        ::testing::HasSubstr("Feature packet encryption is enabled from the command line."));
    check_stdout_line(::testing::HasSubstr(
        "Feature long-term reference frames is enabled from the command line."));

    // Check session id
    char* session_id_copy = get_error_monitor_session_id();
    EXPECT_TRUE(session_id_copy != NULL);
    EXPECT_EQ(strlen(session_id_copy), strlen(argv[25]));
    EXPECT_EQ(strcmp(session_id_copy, argv[25]), 0);
    free(session_id_copy);

    // Check skip taskbar
    EXPECT_TRUE(get_skip_taskbar());

    // Check user email
    const char* user_email;
    EXPECT_SUCCESS(whist_option_get_string_value("user", &user_email));
    EXPECT_TRUE(user_email != NULL);
    EXPECT_STREQ(user_email, argv[28]);

    // Undo global option settings which can break other tests.
    whist_set_single_option("skip-taskbar", "off");
}

TEST_F(ProtocolTest, ErrorCodes) {
    // Success / no-error is zero.
    EXPECT_EQ(WHIST_SUCCESS, 0);

    // Unknown errors are minus-one, for compatibility with functions
    // before error codes were added.
    EXPECT_EQ(WHIST_ERROR_UNKNOWN, -1);

    // Other errors are negative.
    EXPECT_LT(WHIST_ERROR_INVALID_ARGUMENT, -1);

    // Error string function should return a string.
    EXPECT_STREQ(whist_error_string(WHIST_ERROR_NOT_FOUND), "not found");

    // Invalid error codes should also return a string.
    EXPECT_STREQ(whist_error_string((WhistStatus)9001), "invalid error code");

    // Error string function should return something for all values.
    for (int e = 10; e > -100; e--) {
        const char* str = whist_error_string((WhistStatus)e);
        EXPECT_TRUE(str != NULL);
    }
}

TEST_F(ProtocolTest, GitRevision) {
    const char* rev = whist_git_revision();
    size_t len = strlen(rev);

    EXPECT_EQ(len, 15);
    for (unsigned int i = 0; i < len; i++) {
        EXPECT_TRUE(isxdigit(rev[i]));
    }
}

/*
============================
Run Tests
============================
*/

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::GTEST_FLAG(death_test_style) = "threadsafe";
    return RUN_ALL_TESTS();
}
