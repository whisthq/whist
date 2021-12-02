#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H
/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file main.h
 * @brief This file contains the main code that runs a Whist server on a
 *        Windows or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Whist video streaming server being created and creating
its threads.
*/

/*
============================
Includes
============================
*/
#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <process.h>
#include <shlwapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <time.h>

#include <fractal/core/whistgetopt.h>
#include <fractal/core/fractal.h>
#include <fractal/input/input.h>
#include <fractal/logging/error_monitor.h>
#include <fractal/video/transfercapture.h>
#include <fractal/logging/log_statistic.h>
#include "client.h"
#include "network.h"
#include "video.h"
#include "audio.h"

#include <fractal/utils/window_info.h>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

/** @brief whist server configuration parameters */
struct _whist_server_config {
    char binary_aes_private_key[16];
    char hex_aes_private_key[33];
    char identifier[WHIST_IDENTIFIER_MAXLEN + 1];
    int begin_time_to_exit;
};

typedef struct _whist_server_config whist_server_config;

/** @brief internal state of the whist server */
struct _whist_server_state {
    whist_server_config* config;

    volatile int connection_id;
    volatile WhistOSType client_os;

    volatile bool exiting;
    volatile bool stop_streaming;
    volatile bool update_encoder;
    bool client_joined_after_window_name_broadcast;
    Client client;

    int sample_rate;
    volatile int max_bitrate;
    volatile bool wants_iframe;
    InputDevice* input_device;

    /* video */
    volatile int client_width;
    volatile int client_height;
    volatile int client_dpi;
    volatile CodecType client_codec_type;
    volatile bool update_device;

    bool pending_encoder;
    bool encoder_finished;
    VideoEncoder* encoder_factory_result;

    int encoder_factory_server_w;
    int encoder_factory_server_h;
    int encoder_factory_client_w;
    int encoder_factory_client_h;
    int encoder_factory_bitrate;
    CodecType encoder_factory_codec_type;

    /* long term listen sockets */
    SOCKET discovery_listen;
    SOCKET tcp_listen;
    SOCKET udp_listen;
};

typedef struct _whist_server_state whist_server_state;

#endif  // SERVER_MAIN_H
