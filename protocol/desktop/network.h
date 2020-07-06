#ifndef DESKTOP_NETWORK_H
#define DESKTOP_NETWORK_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.h
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

TODO
*/

// must be called before connectToServer()
int discoverPorts(void);

// must be called after discoverPorts()
int connectToServer(void);

int closeConnections(void);

int waitForServerInitMessage(int timeout);

/**
 * @brief                          Sends quit messages to the server
 *
 * @details                        Sends num_messages many quit messages to the
 *                                 server. Sleeps for 50 milliseconds before
 *                                 each message.
 *
 * @param num_messages             Number of quit messages to send
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int sendServerQuitMessages(int num_messages);

#endif  // DESKTOP_NETWORK_H
