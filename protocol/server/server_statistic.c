/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file server_statistic.c
 * @brief This file lists all the server-side metrics that needs to be logged
 */
#include "server_statistic.h"

StatisticInfo server_statistic_info[SERVER_NUM_METRICS];

void whist_init_server_statistics() {
    // (StatisticInfo){"key", is_max_needed, is_min_needed, average_over_time};
    server_statistic_info[AUDIO_ENCODE_TIME] =
        (StatisticInfo){"AUDIO_ENCODE_TIME", true, false, false};
    server_statistic_info[CLIENT_HANDLE_USERINPUT_TIME] =
        (StatisticInfo){"HANDLE_USERINPUT_TIME", true, false, false};
    server_statistic_info[NETWORK_THROTTLED_PACKET_DELAY] =
        (StatisticInfo){"THROTTLED_PACKET_DELAY", true, false, false};
    server_statistic_info[NETWORK_THROTTLED_PACKET_DELAY_RATE] =
        (StatisticInfo){"THROTTLED_PACKET_DELAY_RATE", true, false, false};
    server_statistic_info[NETWORK_THROTTLED_PACKET_DELAY_LOOPS] =
        (StatisticInfo){"THROTTLED_PACKET_DELAY_LOOPS", true, false, false};
    server_statistic_info[VIDEO_CAPTURE_CREATE_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_CREATE_TIME", true, false, false};
    server_statistic_info[VIDEO_CAPTURE_UPDATE_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_UPDATE_TIME", true, false, false};
    server_statistic_info[VIDEO_CAPTURE_SCREEN_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_SCREEN_TIME", true, false, false};
    server_statistic_info[VIDEO_CAPTURE_TRANSFER_TIME] =
        (StatisticInfo){"VIDEO_CAPTURE_TRANSFER_TIME", true, false, false};
    server_statistic_info[VIDEO_ENCODER_UPDATE_TIME] =
        (StatisticInfo){"VIDEO_ENCODER_UPDATE_TIME", true, false, false};
    server_statistic_info[VIDEO_ENCODE_TIME] =
        (StatisticInfo){"VIDEO_ENCODE_TIME", true, false, false};
    server_statistic_info[VIDEO_FPS_SENT] = (StatisticInfo){"VIDEO_FPS_SENT", false, false, true};
    server_statistic_info[VIDEO_FPS_SKIPPED_IN_CAPTURE] =
        (StatisticInfo){"VIDEO_FPS_SKIPPED_IN_CAPTURE", false, false, true};
    server_statistic_info[VIDEO_FRAME_SIZE] =
        (StatisticInfo){"VIDEO_FRAME_SIZE", true, false, false};
    server_statistic_info[VIDEO_FRAME_PROCESSING_TIME] =
        (StatisticInfo){"VIDEO_FRAME_PROCESSING_TIME", true, false, false};
    server_statistic_info[VIDEO_GET_CURSOR_TIME] =
        (StatisticInfo){"GET_CURSOR_TIME", true, false, false};
    server_statistic_info[VIDEO_INTER_FRAME_QP] =
        (StatisticInfo){"VIDEO_INTER_FRAME_QP", true, false, false};
    server_statistic_info[VIDEO_INTRA_FRAME_QP] =
        (StatisticInfo){"VIDEO_INTRA_FRAME_QP", true, false, false};
    server_statistic_info[VIDEO_SEND_TIME] = (StatisticInfo){"VIDEO_SEND_TIME", true, false, false};
    server_statistic_info[DBUS_MSGS_RECEIVED] =
        (StatisticInfo){"DBUS_MSGS_RECEIVED", false, false, true};
}
