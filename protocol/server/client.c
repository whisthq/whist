/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file client.c
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

#include <whist/utils/atomic.h>
#include <whist/utils/rwlock.h>
#include "client.h"
#include "network.h"

// Threads dependent on client being active
static atomic_int threads_needing_active = ATOMIC_VAR_INIT(0);
// Threads currently assuming client is active
static atomic_int threads_holding_active = ATOMIC_VAR_INIT(0);

/*
============================
Public Functions
============================
*/

int init_client(Client *client) {
    /*
        Initializes the client object.
        Must be called before the client object can be used.

        NOTE: Locks shouldn't matter. They are getting created.

        Returns:
            (int): -1 on failure, 0 on success
    */

    client->is_active = false;
    client->udp_port = BASE_UDP_PORT;
    client->tcp_port = BASE_TCP_PORT;
    init_rw_lock(&client->tcp_rwlock);

    return 0;
}

int destroy_clients(Client *client) {
    /*
        De-initializes all clients objects in the client buffer.
        Should be called after initClients() and before program exit.
        Does not disconnect any connected clients.

        NOTE: Locks shouldn't matter. They are getting trashed.

        Returns:
            (int): -1 on failure, 0 on success
    */
    destroy_rw_lock(&client->tcp_rwlock);
    return 0;
}

int start_quitting_client(Client *client) {
    /*
        Begins deactivating client, but does not clean up its
        resources yet. Must be called before `quit_client`.
    */

    client->is_deactivating = true;
    return 0;
}

int quit_client(Client *client) {
    /*
        Deactivates active client. Disconnects client. Updates count of active
        clients. Only does anything on an active client. The associated client
        object is not destroyed and may be made active in the future.

        Arguments:
            client (Client*): target client
            id (int): Client ID of active client to deactivate

        Returns:
            (int): -1 on failure, 0 on success
    */

    // If client is not active, just return
    if (!client->is_active) {
        return 0;
    }

    // Client and server share file transfer indexes when sending files, so
    //     when a client disconnects, we need to reset the transferring files
    //     to make sure that if a client reconnects that the indices are fresh.
    reset_all_transferring_files();

    client->is_active = false;
    if (disconnect_client(client) != 0) {
        LOG_ERROR("Failed to disconnect client.");
        return -1;
    }
    client->is_deactivating = false;
    return 0;
}

void add_thread_to_client_active_dependents(void) {
    /*
        Add thread to count of those dependent on client being active
    */

    atomic_fetch_add(&threads_needing_active, 1);
}

static void remove_thread_from_holding_active_count(void) {
    /*
        Remove thread from those currently assuming that client is active
    */

    atomic_fetch_sub(&threads_holding_active, 1);
}

void reset_threads_holding_active_count(Client *client) {
    /*
        Set the thread count regarding a client as active to the full
        dependent thread count again. This is needed when a new client
        is made active after the previous one has been deactivated
        and quit.

        NOTE: Should only be called from `multithreaded_manage_client`
    */

    atomic_store(&threads_holding_active, atomic_load(&threads_needing_active));
    client->is_deactivating = false;
}

void update_client_active_status(Client *client, bool *is_thread_assuming_active) {
    /*
        Allows a thread to update its status on whether it believes
        the client is active or not. If a client is deactivating,
        we want the thread to stop believing that the client is active,
        but otherwise will leave the status as is.

        Arguments:
            is_thread_assuming_active (bool*): pointer to the boolean
                that the thread is using to indicate whether it believes
                the client is currently active.
                >> If the client should change its belief, this pointer
                is populated with the new value.
    */

    if (client->is_deactivating) {
        if (*is_thread_assuming_active) {
            *is_thread_assuming_active = false;
            remove_thread_from_holding_active_count();
        }
    } else if (client->is_active && !*is_thread_assuming_active) {
        *is_thread_assuming_active = true;
    }
}

bool threads_still_holding_active(void) {
    /*
        Whether there remain any threads that are assuming that
        the client is active.

        Returns:
            (bool): whether any threads need the client to still be active
    */

    return atomic_load(&threads_holding_active) > 0;
}
