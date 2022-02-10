#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file client.h
 * @brief This file contains the code for interacting with the client buffer.

See
https://www.notion.so/whisthq/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=755123dad01244f3a0a3ced5139228a3
for the specification for multi-client code
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include <whist/utils/clock.h>
#include <whist/utils/rwlock.h>

/*
============================
Custom types
============================
*/

typedef struct Client {
    /* ACTIVE */
    bool is_active;        // "protected" by `is_deactivating`
    bool is_deactivating;  // whether a client is in the process of deactivating

    /* USER INFO */
    int user_id;  // not lock protected

    /* NETWORK */
    SocketContext udp_context;  // "protected" by global `is_deactivating`
    SocketContext tcp_context;  // "protected" by global `is_deactivating`
    int udp_port;               // "protected" by global `is_deactivating`
    int tcp_port;               // "protected" by global `is_deactivating`
    RWLock tcp_rwlock;          // protects tcp_context for synchrony-sensitive sends and recvs
} Client;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initializes client object.
 *
 * @details                        Must be called before the client object
 *                                 can be used.
 *
 * @param client				   the client context to initialize
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int init_client(Client* client);

/**
 * @brief                          De-initializes all clients objects in the
 *                                 client buffer.
 *
 * @details                        Should be called after initClients() and
 *                                 before program exit. Does not disconnect any
 *                                 connected clients.
 *
 * @param client				   the client context to destroy
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int destroy_clients(Client* client);

/**
 * @brief                          Begins deactivating client, but does not clean up
 *                                 its resources yet. Must be called before `quit_client`.
 *
 * @param client				   Target client
 */
int start_quitting_client(Client* client);

/**
 * @brief                          Deactivates active client.
 *
 * @details                        Disconnects client. Updates count of active
 *                                 clients. May only be called on an active
 *                                 client. The associated client object is
 *                                 not destroyed and may be made active in the
 *                                 future.
 *
 * @param client				   Target client
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int quit_client(Client* client);

/**
 * @brief                          Add thread to count of those dependent on
 *                                 client being active.
 *
 * @details                        This function only needs to be called if the
 *                                 thread depends on any members of `Client`. For
 *                                 example, if a thread uses Client.udp_context,
 *                                 we don't want the resource to be destroyed while
 *                                 the thread continues to think that the client is
 *                                 active. This also means that the thread has to
 *                                 call `remove_thread_from_holding_active_count`
 *                                 eventually to release its hold on the active
 *                                 client. This is typically done through a call to
 *                                 `update_client_active_status` in the thread loop.
 */
void add_thread_to_client_active_dependents(void);

/**
 * @brief                          Set the thread count regarding a client as
 *                                 active to the full dependent thread count
 *                                 again. This is needed when a new client is
 *                                 made active after the previous one has been
 *                                 deactivated and quit.
 */
void reset_threads_holding_active_count(Client* client);

/**
 * @brief                          Allows a thread to update its status on
 *                                 whether it believes the client is active or
 *                                 not. If a client is deactivating, we want the
 *                                 thread to stop believing that the client is
 *                                 active, but otherwise will leave the status as is.
 *
 * @param client				    Target client
 * @param is_thread_assuming_active Pointer to the boolean that the thread is using
 *                                  to indicate whether it believes the client is
 *                                  currently active
 */
void update_client_active_status(Client* client, bool* is_thread_assuming_active);

/**
 * @brief                          Whether there remain any threads that are assuming
 *                                 that the client is active.
 *
 * @returns                        Whether any threads need the client to still be
 *                                 active
 */
bool threads_still_holding_active(void);

#endif  // SERVER_CLIENT_H
