#ifndef SERVER_STATISTIC_H
#define SERVER_STATISTIC_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file server_statistic.h
 * @brief This file lists all the server-side metrics that needs to be logged
 */
#include <whist/logging/log_statistic.h>

/*
============================
Public enums
============================
*/
enum {
    AUDIO_ENCODE_TIME,
    CLIENT_HANDLE_USERINPUT_TIME,
    NETWORK_THROTTLED_PACKET_DELAY,
    NETWORK_THROTTLED_PACKET_DELAY_RATE,
    NETWORK_THROTTLED_PACKET_DELAY_LOOPS,
    VIDEO_CAPTURE_CREATE_TIME,
    VIDEO_CAPTURE_UPDATE_TIME,
    VIDEO_CAPTURE_SCREEN_TIME,
    VIDEO_CAPTURE_TRANSFER_TIME,
    VIDEO_ENCODER_UPDATE_TIME,
    VIDEO_ENCODE_TIME,
    VIDEO_FPS_SENT,
    VIDEO_FPS_SKIPPED_IN_CAPTURE,
    VIDEO_FRAME_SIZE,
    VIDEO_FRAME_PROCESSING_TIME,
    VIDEO_GET_CURSOR_TIME,
    VIDEO_INTER_FRAME_QP,
    VIDEO_INTRA_FRAME_QP,
    VIDEO_SEND_TIME,
    DBUS_MSGS_RECEIVED,
    SERVER_NUM_METRICS
};

/*
============================
External Variables
============================
*/
extern StatisticInfo server_statistic_info[SERVER_NUM_METRICS];

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize "server_statistic_info" array.
 */
void whist_init_server_statistics(void);

#endif  // SERVER_STATISTIC_H
