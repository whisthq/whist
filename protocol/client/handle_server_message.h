#ifndef SERVER_MESSAGE_HANDLER_H
#define SERVER_MESSAGE_HANDLER_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file handle_server_message.h
 * @brief This file contains all the code for client-side processing of messages
 *        received from the server
============================
Usage
============================

handleServerMessage() must be called on any received message from the server.
Any action trigged a server message must be initiated in network.c.
*/

/*
============================
Includes
============================
*/

#include <stddef.h>

#include <fractal/core/fractal.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Handles message received by client from
 *                                 server, on client-side
 *
 * @details                        Updates client state, replies to server,
 *                                 or performs whatever side-effects appropriate
 *                                 in handling message from server. Logs errors.
 *
 * @param fsmsg                     Message from server
 * @param fsmsg_size                The size (in bytes) of the message, including
 *                                 any buffer considered part of the message
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int handle_server_message(FractalServerMessage *fsmsg, size_t fsmsg_size);

#endif  // SERVER_MESSAGE_HANDLER_H
