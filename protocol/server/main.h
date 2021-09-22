#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file main.h
 * @brief This file contains the main code that runs a Fractal server on a
 *        Windows or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Fractal video streaming server being created and creating
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

#include <fractal/utils/window_name.h>
#include <fractal/core/fractalgetopt.h>
#include <fractal/core/fractal.h>
#include <fractal/input/input.h>
#include <fractal/logging/error_monitor.h>
#include <fractal/video/transfercapture.h>
#include <fractal/logging/log_statistic.h>
#include "client.h"
#include "handle_client_message.h"
#include "parse_args.h"
#include "network.h"
#include "video.h"
#include "audio.h"

#ifdef _WIN32
#include <fractal/utils/windows_utils.h>
#endif

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

extern Client clients[MAX_NUM_CLIENTS];

volatile FractalOSType client_os;
char binary_aes_private_key[16];
char hex_aes_private_key[33];

// This variables should stay as arrays - we call sizeof() on them
char identifier[FRACTAL_IDENTIFIER_MAXLEN + 1];

volatile int connection_id;
volatile bool exiting;

int sample_rate = -1;

volatile int max_bitrate = STARTING_BITRATE;
volatile int max_burst_bitrate = STARTING_BURST_BITRATE;
InputDevice* input_device = NULL;

volatile bool stop_streaming;
volatile bool wants_iframe;
volatile bool update_encoder;

bool client_joined_after_window_name_broadcast = false;
// This variable should always be an array - we call sizeof()

#endif  // SERVER_MAIN_H
