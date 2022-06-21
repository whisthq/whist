/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file whist_cursor_test.cpp
 * @brief This file contains unit tests for the files in protocol/whist/cursor
 */

#include <gtest/gtest.h>
#include "fixtures.hpp"

extern "C" {
#include <whist/core/whist.h>
#include <whist/core/whist_frame.h>
#include <whist/cursor/cursor_internal.h>
}

class WhistCursorTest : public CaptureStdoutFixture {};

// Death tests are tests which run in a separate process or thread until
// exit. GTest wants us to run death tests before regular tests, so we
// append DeathTest to the test name to register this behavior with GTest.
using WhistCursorDeathTest = WhistCursorTest;

TEST_F(WhistCursorTest, CursorSizeID) {
    WhistCursorInfo* info = whist_cursor_info_from_id(WHIST_CURSOR_CROSSHAIR, CURSOR_STATE_VISIBLE);
    EXPECT_EQ(sizeof(WhistCursorInfo), whist_cursor_info_get_size(info));
    free(info);

    info = whist_cursor_info_from_id(WHIST_CURSOR_ARROW, CURSOR_STATE_HIDDEN);
    EXPECT_EQ(sizeof(WhistCursorInfo), whist_cursor_info_get_size(info));
    free(info);
}

// the citadel logo in rgba form (25 x 11)
#define UINT32_PACK_RGBA(hex)                                                             \
    (((hex & 0xFF000000) >> 24) | ((hex & 0x00FF0000) >> 8) | ((hex & 0x0000FF00) << 8) | \
     ((hex & 0x000000FF) << 24))
static const uint32_t b = UINT32_PACK_RGBA(0x2d5dccff);
static const uint32_t w = UINT32_PACK_RGBA(0xffffffff);
static const uint32_t citadel_cursor[] = {
    b, b, b, b, b, w, w, w, w, w, b, b, b, b, b, w, w, w, w, w, b, b, b, b, b, b, b, b, b, b, w,
    w, w, w, w, b, b, b, b, b, w, w, w, w, w, b, b, b, b, b, b, b, b, b, b, w, w, w, w, w, b, b,
    b, b, b, w, w, w, w, w, b, b, b, b, b, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w,
    w, w, w, w, w, w, w, b, b, b, b, b, b, b, b, b, b, b, b, w, b, b, b, b, b, b, b, b, b, b, b,
    b, b, b, b, b, b, b, b, b, b, b, b, b, w, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,
    b, b, b, b, b, b, b, w, b, b, b, b, b, b, b, b, b, b, b, b, w, w, w, w, w, w, w, w, w, w, w,
    w, w, w, w, w, w, w, w, w, w, w, w, w, w, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,
    b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,
    b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,
};
static const size_t citadel_cursor_width = 25;
static const size_t citadel_cursor_height = 11;

TEST_F(WhistCursorTest, CursorSizePNG) {
    // Print out what's in the buf in a 25 x 11 hex grid
    WhistCursorInfo* info = whist_cursor_info_from_rgba(
        citadel_cursor, citadel_cursor_width, citadel_cursor_height, 5, 5, CURSOR_STATE_VISIBLE);
    EXPECT_TRUE(info != NULL);

    const size_t hardcoded_png_size = 112;
    EXPECT_EQ(sizeof(WhistCursorInfo) + hardcoded_png_size, whist_cursor_info_get_size(info));

    free(info);
}

TEST_F(WhistCursorDeathTest, CursorSizeNull) {
    EXPECT_DEATH(whist_cursor_info_get_size(NULL), ".*");
}

TEST_F(WhistCursorTest, PNGIdentity) {
    WhistCursorInfo* info = whist_cursor_info_from_rgba(
        citadel_cursor, citadel_cursor_width, citadel_cursor_height, 5, 5, CURSOR_STATE_VISIBLE);

    EXPECT_TRUE(info != NULL);

    uint32_t* decoded_cursor = (uint32_t*)whist_cursor_info_to_rgba(info);
    free(info);

    for (size_t i = 0; i < citadel_cursor_width * citadel_cursor_height; i++) {
        EXPECT_EQ(citadel_cursor[i], decoded_cursor[i]) << "i = " << i;
    }
    free(decoded_cursor);
}
