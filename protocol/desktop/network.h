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

establishConnections() must be called for the non-spectator client to send and
receive UDP and TCP packets from the server. establishSpectatorConnections()
must be called to do the same for spectator clients. If establishConnections()
or establishSpectatorConnections() succeeds, closeConnections() or
closeSpectatorConnections(), respectively, must be called before the application
exits for a safe exit. If establishConnections() or
establishSpectatorConnections() fails, either function may be called again to
safely re-attempt connecting.
*/

/**
 * @brief                          Establishes TCP and UDP connections between
 *                                 server and non-spectator client, on client-
 *                                 side.
 *
 * @details                        Makes global variables PacketSendContext,
 *                                 PacketReceiveContext, and PacketTCPContext
 *                                 contexts available for communication, on
 *                                 success. Global variable server_ip must be
 *                                 set. Logs errors.
 *
 * @returns                        Returns -1 on failure to connect or error,
 *                                 0 on success
 */
int establishConnections(void);

/**
 * @brief                          Closes TCP and UDP connections between
 *                                 server and non-spectator client, on client-
 *                                 side.
 *
 * @details                        Should only be called after
 *                                 establishConnections(). Destroys
 *                                 PacketSendContext, PacketReceiveContext,
 *                                 and PacketTCPContext.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int closeConnections(void);

/**
 * @brief                          Establishes UDP connection between
 *                                 server and spectator client, from client-
 *                                 side.
 *
 * @details                        Makes global variables PacketSendContext and
 *                                 PacketReceiveContext contexts available for
 *                                 communication, on success. Global variable
 *                                 server_ip must be set. Logs errors.
 *
 * @returns                        Returns -1 on failure to connect or error,
 *                                 0 on success
 */
int establishSpectatorConnections(void);

/**
 * @brief                          Closes UDP connection between server and
 *                                 spectator client, on client-side
 *
 * @details                        Should only be called after
 *                                 establishSpectatorConnections(). Destroys
 *                                 PacketSendContext and PacketReceiveContext.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int closeSpectatorConnections(void);

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

/**
 * @brief                          Waits until client receives init message from
 *                                 server
 *
 * @details                        Waits for global variable
 *                                 receieved_server_init_message to be true.
 *                                 Sleeps between checking the variable.
 *                                 Returns -1 if not true after timeout many
 *                                 milliseconds.
 *
 * @param timeout                  Maximum time (in milliseconds) to wait before
 *                                 returning
 *
 * @returns                        Returns -1 on timeout or error, 0 on success
 */
int waitForServerInitMessage(int timeout);

#endif  // DESKTOP_NETWORK_H
