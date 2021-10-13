/**
 * Copyright Fractal Computers, Inc. 2020
 * @file client.c
 * @brief This file contains the code for interacting with the client buffer.

See
https://www.notion.so/tryfractal/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=755123dad01244f3a0a3ced5139228a3
for the specification for multi-client code
*/

/*
============================
Includes
============================
*/

#include <fractal/utils/rwlock.h>
#include "client.h"
#include "network.h"

FractalMutex state_lock;
RWLock is_active_rwlock;

Client client;

int num_controlling_clients = 0;
int num_active_clients = 0;
int host_id = -1;

/*
============================
Public Functions
============================
*/

int init_client(void) {
    /*
        Initializes the client objects in the client buffer.
        Must be called before the client buffer can be used.

        NOTE: Locks shouldn't matter. They are getting created.

        Returns:
            (int): -1 on failure, 0 on success
    */

    state_lock = fractal_create_mutex();
    init_rw_lock(&is_active_rwlock);

    client.is_active = false;
    client.udp_port = BASE_UDP_PORT;
    client.tcp_port = BASE_TCP_PORT;
    init_rw_lock(&client.tcp_rwlock);

    return 0;
}

int destroy_clients(void) {
    /*
        De-initializes all clients objects in the client buffer.
        Should be called after initClients() and before program exit.
        Does not disconnect any connected clients.

        NOTE: Locks shouldn't matter. They are getting trashed.

        Returns:
            (int): -1 on failure, 0 on success
    */

    destroy_rw_lock(&client.tcp_rwlock);
    fractal_destroy_mutex(state_lock);
    destroy_rw_lock(&is_active_rwlock);
    return 0;
}

int quit_client() {
    /*
        Deactivates active client. Disconnects client. Updates count of active
        clients. May only be called on an active client. The associated client
        object is not destroyed and may be made active in the future.

        NOTE: Needs write lock is_active_rwlock and (write) state_lock

        Arguments:
            id (int): Client ID of active client to deactivate

        Returns:
            (int): -1 on failure, 0 on success
    */

    client.is_active = false;
    if (disconnect_client() != 0) {
        LOG_ERROR("Failed to disconnect client.");
        return -1;
    }
    return 0;
}

int reap_timed_out_client(double timeout) {
    /*
        Quits client if timed out.

        NOTE: Needs write is_active_rwlock and (write) state_lock

        Arguments:
            timeout (double): Duration (in seconds) after which a
                client is deemed timed out if the server has not received
                a ping from the client.

        Returns:
            (int): Returns -1 on failure, 0 on success.
    */

    if (client.is_active && get_timer(client.last_ping) > timeout) {
        LOG_INFO("Dropping timed out client");
        if (quit_client() != 0) {
            LOG_ERROR("Failed to quit client.");
            return -1;
        }
    }
    return 0;
}

int does_client_match_user_id(int user_id, bool *found) {
    /*
        Finds out whether the client is associated with the `user_id`.

        NOTES: Needs read is_active_rwlock

        Arguments:
            user_id (int): User ID to be searched for.
            found (bool*): Populated with true if the active client
                matches the user_id

        Retrurns:
            (int): Returns -1 on failure, 0 on success. Not matching
                the client does not mean failure.
    */

    *found = false;
    if (client.is_active && client.user_id == user_id) {
        *found = true;
        return 0;
    }
    return 0;
}
