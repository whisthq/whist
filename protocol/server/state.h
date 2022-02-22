#ifndef WHIST_SERVER_STATE_H
#define WHIST_SERVER_STATE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file state.h
 * @brief This file defines a struct to hold server global state
 */

/*
============================
Includes
============================
*/
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

#include <whist/core/whistgetopt.h>
#include <whist/core/whist.h>
#include <whist/input/input.h>
#include <whist/logging/error_monitor.h>
#include <whist/video/transfercapture.h>
#include <whist/logging/log_statistic.h>
#include "client.h"
#include "network.h"
#include "video.h"
#include "audio.h"

#include <whist/utils/window_info.h>
#include <whist/utils/sysinfo.h>

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
    char dbus_address[DBUS_ADDRESS_MAXLEN + 1];
    WhistSubsystemParams susbsystems_params;
};

typedef struct _whist_server_config whist_server_config;

// NOTE: PLEASE DO NOT ADD MORE TO THIS STRUCT
// See code standards, this counts as "extern global state".
// Instead, make classes with init/destroy and getters/setters.

/** @brief internal state of the whist server */
struct _whist_server_state {
    // TODO: Mutex usage of this state
    whist_server_config* config;

    volatile int connection_id;
    volatile WhistOSType client_os;

    volatile bool exiting;
    volatile bool stop_streaming;
    bool client_joined_after_window_name_broadcast;
    Client client;

    // This `update_encoder` should be set to true,
    // when the above "requested" variables have changed
    volatile bool update_encoder;

    InputDevice* input_device;

    /* iframe */
    volatile bool wants_iframe;

    /* video */
    volatile int client_width;
    volatile int client_height;
    volatile int client_dpi;
    volatile bool update_device;

    bool pending_encoder;
    bool encoder_finished;
    VideoEncoder* encoder_factory_result;

    int encoder_factory_server_w;
    int encoder_factory_server_h;
    int encoder_factory_client_w;
    int encoder_factory_client_h;
    int encoder_factory_bitrate;
    int encoder_factory_vbv_size;
    CodecType encoder_factory_codec_type;
};

typedef struct _whist_server_state whist_server_state;

#endif  // WHIST_SERVER_STATE_H
