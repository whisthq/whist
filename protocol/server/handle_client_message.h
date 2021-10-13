#ifndef HANDLE_CLIENT_MESSAGE_H
#define HANDLE_CLIENT_MESSAGE_H
/**
 * Copyright Fractal Computers, Inc. 2020
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

/**
 * @brief                          Handles message received by the server from
 *                                 a client, on the server-side
 *
 * @details                        Replays keyboard and mouse input, replies
 *                                 to pings, or performs whatever side-effects
 *                                 appropriate in handling message from client.
 *                                 Logs errors.
 *
 * @param fcmsg                     Message from client
 * @param client_id                Client ID of client from whom message was
 *                                 received
 * @param is_controlling           True is the client from whom the message was
 *                                 received is controlling, not spectating
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int handle_client_message(FractalClientMessage *fcmsg, int client_id, bool is_controlling);

#endif  // HANDLE_CLIENT_MESSAGE_H
