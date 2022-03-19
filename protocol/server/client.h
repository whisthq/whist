#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file client.h
 * @brief This file contains the code for interacting with the client buffer.

Client is a wrapper around the client state, and the client connection information.

All client functions in this header file must be called on a single thread (The "client management
thread"). The only exception are functions explicitly marked as thread-safe.

This information inside of the client struct can only be accessed on other threads if the client is
_active_. The client is only guaranteed to be active, if a client activation lock is actively held.
The client will not deactivate while a client activation lock is actively held.

The client's data is guaranteed to be valid while the client is active, but should still only be
used in a thread-safe way. Each unique activation, will be assigned its own connection_id. This to
identify if a new connection has occured.

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

typedef struct ClientPrivateData ClientPrivateData;

typedef struct {
    // PUBLIC

    /* ACTIVE */
    // This is public data for the client management thread
    // This data should only be accessed on the client management thread
    bool is_active;        // Whether or not a client is active
    bool is_deactivating;  // Whether a client is in the process of deactivating

    // The publc data below this line is only valid on other threads when a client activity lock is
    // held Note that multiple threads may access the below data, so it must be thread-safe and/or
    // locked appropriately

    /* NETWORK */
    // Unique identifier for this connection
    int connection_id;
    // Network contexts for communication to/from the client
    SocketContext udp_context;
    SocketContext tcp_context;

    // PRIVATE

    // Private data, do not access
    ClientPrivateData* private_data;
} Client;

typedef struct ClientLock ClientLock;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initializes client object.
 *
 * @returns                        Returns the new client
 */
Client* init_client(void);

/**
 * @brief                          De-initializes all client data.
 *
 * @details                        Should be called after init_client() and
 *                                 before program exit. Does not disconnect any
 *                                 connected clients.
 *
 * @param client                   The client context to destroy
 *
 * @note                           This function is not thread-safe with any other function,
 *                                 since it destroys the locks
 */
void destroy_client(Client* client);

/**
 * @brief                          Marks the client as active
 *
 * @param client                   The client to activate
 */
void activate_client(Client* client);

/**
 * @brief                          Marks a client for deactivation, but does not deactivate it yet.
 *
 * @param client                   The client to mark for deactivation
 *
 * @note                           client->is_deactivating will show whether or not an active client
 *                                 has been marked for deactivation.
 *
 * @note                           This function can be thread-safe relative to the other client
 *                                 management functions, but it is only thread-safe if a client
 *                                 activation lock is held in that thread.
 */
void start_deactivating_client(Client* client);

/**
 * @brief                          Deactivates an active client.
 *                                 Blocks until all ClientLock's are unlocked.
 *
 * @param client                   The client to deactivate
 */
void deactivate_client(Client* client);

/**
 * @brief                          Marks a client as permanently deactivated.
 *                                 This will cause all attempts to lock the client to return NULL.
 *
 * @param client                   The client to permanently deactivate
 */
void permanently_deactivate_client(Client* client);

/**
 * @brief                          Gets a lock on an active client.
 *                                 This will block until the client is both active
 *                                 and not pending deactivation.
 *                                 This lock must be released often.
 *
 * @param client                   The client to wait for activation on
 *
 * @returns                        The lock, or NULL if the client was permanently deactivated
 *
 * @note                           This function is thread-safe relative to
 *                                 the other client management functions
 *
 * @note                           Failing to release this lock promptly (Within 1 second),
 *                                 may forcefully shutdown WhistServer.
 */
ClientLock* client_active_lock(Client* client);

// Same as client_active_lock,
// but quickly returns NULL if the client is not active or is pending deactivation.
// This function is guaranteed to return promptly.
ClientLock* client_active_trylock(Client* client);

/**
 * @brief                          Releases the lock on an active client.
 *
 * @param client_lock              The lock to unlock
 *
 * @note                           This function is thread-safe relative to
 *                                 the other client management functions
 */
void client_active_unlock(ClientLock* client_lock);

#endif  // SERVER_CLIENT_H
