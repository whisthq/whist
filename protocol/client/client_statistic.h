#ifndef CLIENT_STATISTIC_H
#define CLIENT_STATISTIC_H
/**
 * Copyright (c) 2019-2022 Whist Technologies, Inc.
 * @file client_statistic.h
 * @brief This file lists all the client-side metrics that needs to be logged
 */
#include <whist/logging/log_statistic.h>

/*
============================
Public enums
============================
*/
typedef enum {
    AUDIO_RECEIVE_TIME,
    AUDIO_UPDATE_TIME,
    AUDIO_FPS_SKIPPED_FLUSH,
    AUDIO_FPS_SKIPPED_CATCHUP,
    AUDIO_FPS_SKIPPED_RECEIVE,
    NETWORK_READ_PACKET_TCP,
    NETWORK_READ_PACKET_UDP,
    SERVER_HANDLE_MESSAGE_TCP,
    SERVER_HANDLE_MESSAGE_UDP,
    VIDEO_AVCODEC_RECEIVE_TIME,
    VIDEO_AV_HWFRAME_TRANSFER_TIME,
    VIDEO_CURSOR_UPDATE_TIME,
    VIDEO_DECODE_SEND_PACKET_TIME,
    VIDEO_DECODE_GET_FRAME_TIME,
    VIDEO_E2E_LATENCY,
    VIDEO_RECEIVE_TIME,
    VIDEO_RENDER_TIME,
    VIDEO_SDL_WRITE_TIME,
    VIDEO_TIME_BETWEEN_FRAMES,
    VIDEO_UPDATE_TIME,
    CLIENT_NUM_METRICS
} ClientMetric;

/*
============================
External Variables
============================
*/
extern StatisticInfo client_statistic_info[CLIENT_NUM_METRICS];

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize "client_statistic_info" array.
 */
void init_client_statistics();

#endif  // CLIENT_STATISTIC_H
