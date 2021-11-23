#ifndef HANDLE_CLIENT_MESSAGE_H
#define HANDLE_CLIENT_MESSAGE_H
/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file handle_client_message.h
 * @brief This file contains all the code for server-side processing of messages
 *        received from a client
============================
Usage
============================

handle_client_message() must be called on any received message from a client.
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>

/*
============================
Public Functions
============================
*/

typedef struct _whist_server_state whist_server_state;

/**
 * @brief                          Handles message received by the server from
 *                                 a client, on the server-side
 *
 * @details                        Replays keyboard and mouse input, replies
 *                                 to pings, or performs whatever side-effects
 *                                 appropriate in handling message from client.
 *                                 Logs errors.
 *
 * @param state					   Server's state
 * @param fcmsg                    Message from client
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int handle_client_message(whist_server_state *state, FractalClientMessage *fcmsg);

#endif  // HANDLE_CLIENT_MESSAGE_H
