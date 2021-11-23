/**
 * Copyright 2021 Whist Technologies, Inc.
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

#include <fractal/utils/rwlock.h>
#include "client.h"
#include "network.h"

volatile int threads_needing_active = 0;  // Threads dependent on client being active
volatile int threads_holding_active = 0;  // Threads currently assuming client is active
FractalMutex active_holding_write_mutex;  // Protects writes to the above two variables
                                          //     (protecting reads not necessary)

Client client;

/*
============================
Public Functions
============================
*/

int init_client(void) {
    /*
        Initializes the client object.
        Must be called before the client object can be used.

        NOTE: Locks shouldn't matter. They are getting created.

        Returns:
            (int): -1 on failure, 0 on success
    */

    client.is_active = false;
    client.udp_port = BASE_UDP_PORT;
    client.tcp_port = BASE_TCP_PORT;
    init_rw_lock(&client.tcp_rwlock);
    active_holding_write_mutex = fractal_create_mutex();

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
    return 0;
}

int start_quitting_client() {
    /*
        Begins deactivating client, but does not clean up its
        resources yet. Must be called before `quit_client`.
    */

    client.is_deactivating = true;
    return 0;
}

int quit_client() {
    /*
        Deactivates active client. Disconnects client. Updates count of active
        clients. Only does anything on an active client. The associated client
        object is not destroyed and may be made active in the future.

        Arguments:
            id (int): Client ID of active client to deactivate

        Returns:
            (int): -1 on failure, 0 on success
    */

    // If client is not active, just return
    if (!client.is_active) {
        return 0;
    }

    client.is_active = false;
    if (disconnect_client() != 0) {
        LOG_ERROR("Failed to disconnect client.");
        return -1;
    }
    client.is_deactivating = false;
    return 0;
}

int reap_timed_out_client(double timeout) {
    /*
        Sets client to quit if timed out.

        Arguments:
            timeout (double): Duration (in seconds) after which a
                client is deemed timed out if the server has not received
                a ping from the client.

        Returns:
            (int): Returns -1 on failure, 0 on success.
    */

    if (client.is_active && get_timer(client.last_ping) > timeout) {
        LOG_INFO("Dropping timed out client");
        if (start_quitting_client() != 0) {
            LOG_ERROR("Failed to start quitting client.");
            return -1;
        }
    }
    return 0;
}

void add_thread_to_client_active_dependents() {
    /*
        Add thread to count of those dependent on client being active
    */

    fractal_lock_mutex(active_holding_write_mutex);
    threads_needing_active++;
    fractal_unlock_mutex(active_holding_write_mutex);
}

void remove_thread_from_holding_active_count() {
    /*
        Remove thread from those currently assuming that client is active
    */

    fractal_lock_mutex(active_holding_write_mutex);
    threads_holding_active--;
    fractal_unlock_mutex(active_holding_write_mutex);
}

void reset_threads_holding_active_count() {
    /*
        Set the thread count regarding a client as active to the full
        dependent thread count again. This is needed when a new client
        is made active after the previous one has been deactivated
        and quit.

        NOTE: Should only be called from `multithreaded_manage_client`
    */

    fractal_lock_mutex(active_holding_write_mutex);
    threads_holding_active = threads_needing_active;
    client.is_deactivating = false;
    fractal_unlock_mutex(active_holding_write_mutex);
}

void update_client_active_status(bool* is_thread_assuming_active) {
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

    if (client.is_deactivating) {
        if (*is_thread_assuming_active) {
            *is_thread_assuming_active = false;
            remove_thread_from_holding_active_count();
        }
    } else if (client.is_active && !*is_thread_assuming_active) {
        *is_thread_assuming_active = true;
    }
}

bool threads_still_holding_active() {
    /*
        Whether there remain any threads that are assuming that
        the client is active.

        Returns:
            (bool): whether any threads need the client to still be active
    */

    return threads_holding_active > 0;
}
