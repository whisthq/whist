/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file client_statistic.c
 * @brief This file lists all the client-side metrics that needs to be logged
 */
#include "client_statistic.h"

StatisticInfo client_statistic_info[CLIENT_NUM_METRICS];

void init_client_statistics() {
    // (StatisticInfo){"key", is_max_needed, is_min_needed, average_over_time};
    client_statistic_info[AUDIO_RECEIVE_TIME] =
        (StatisticInfo){"AUDIO_RECEIVE_TIME", true, false, false};
    client_statistic_info[AUDIO_UPDATE_TIME] =
        (StatisticInfo){"AUDIO_UPDATE_TIME", true, false, false};
    client_statistic_info[AUDIO_FPS_SKIPPED_FLUSH] =
        (StatisticInfo){"AUDIO_FPS_SKIPPED_FLUSH", false, false, true};
    client_statistic_info[AUDIO_FPS_SKIPPED_CATCHUP] =
        (StatisticInfo){"AUDIO_FPS_SKIPPED_CATCHUP", true, false, true};
    client_statistic_info[AUDIO_FPS_SKIPPED_RECEIVE] =
        (StatisticInfo){"AUDIO_FPS_SKIPPED_RECEIVE", true, false, true};
    client_statistic_info[NETWORK_READ_PACKET_TCP] =
        (StatisticInfo){"READ_PACKET_TIME_TCP", true, false, false};
    client_statistic_info[NETWORK_READ_PACKET_UDP] =
        (StatisticInfo){"READ_PACKET_TIME_UDP", true, false, false};
    client_statistic_info[SERVER_HANDLE_MESSAGE_TCP] =
        (StatisticInfo){"HANDLE_SERVER_MESSAGE_TIME_TCP", true, false, false};
    client_statistic_info[SERVER_HANDLE_MESSAGE_UDP] =
        (StatisticInfo){"HANDLE_SERVER_MESSAGE_TIME_UDP", true, false, false};
    client_statistic_info[VIDEO_AVCODEC_RECEIVE_TIME] =
        (StatisticInfo){"AVCODEC_RECEIVE_TIME", true, false, false};
    client_statistic_info[VIDEO_AV_HWFRAME_TRANSFER_TIME] =
        (StatisticInfo){"AV_HWFRAME_TRANSFER_TIME", true, false, false};
    client_statistic_info[VIDEO_CURSOR_UPDATE_TIME] =
        (StatisticInfo){"CURSOR_UPDATE_TIME", true, false, false};
    client_statistic_info[VIDEO_DECODE_SEND_PACKET_TIME] =
        (StatisticInfo){"VIDEO_DECODE_SEND_PACKET_TIME", true, false, false};
    client_statistic_info[VIDEO_DECODE_GET_FRAME_TIME] =
        (StatisticInfo){"VIDEO_DECODE_GET_FRAME_TIME", true, false, false};
    client_statistic_info[VIDEO_RECEIVE_TIME] =
        (StatisticInfo){"VIDEO_RECEIVE_TIME", true, false, false};
    client_statistic_info[VIDEO_RENDER_TIME] =
        (StatisticInfo){"VIDEO_RENDER_TIME", true, false, false};
    client_statistic_info[VIDEO_SDL_WRITE_TIME] =
        (StatisticInfo){"VIDEO_SDL_WRITE_TIME", true, false, false};
    client_statistic_info[VIDEO_TIME_BETWEEN_FRAMES] =
        (StatisticInfo){"VIDEO_TIME_BETWEEN_FRAMES", true, false, false};
    client_statistic_info[VIDEO_UPDATE_TIME] =
        (StatisticInfo){"VIDEO_UPDATE_TIME", true, false, false};
}
