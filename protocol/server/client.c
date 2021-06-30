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

#include <fractal/utils/mouse.h>
#include <fractal/utils/rwlock.h>
#include "client.h"
#include "server_network.h"

FractalMutex state_lock;
RWLock is_active_rwlock;

Client clients[MAX_NUM_CLIENTS];

int num_controlling_clients = 0;
int num_active_clients = 0;
int host_id = -1;

volatile bool client_exited_nongracefully =
    false;  // set to true after a nongraceful exit (only exit logic can set to false)
volatile clock last_nongraceful_exit;  // start this after every nongraceful exit

/*
============================
Public Functions
============================
*/

int init_clients(void) {
    /*
        Initializes all clients objects in the client buffer.
        Must be called before the client buffer can be used.

        NOTE: Locks shouldn't matter. They are getting created.

        Returns:
            (int): -1 on failure, 0 on success
    */

    state_lock = fractal_create_mutex();
    init_rw_lock(&is_active_rwlock);
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        clients[id].is_active = false;

        clients[id].UDP_port = BASE_UDP_PORT + id;
        clients[id].TCP_port = BASE_TCP_PORT + id;

        memcpy(&(clients[id].mouse.color), &(mouse_colors[id]), sizeof(FractalRGBColor));
    }
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

    fractal_destroy_mutex(state_lock);
    destroy_rw_lock(&is_active_rwlock);
    return 0;
}

int quit_client(int id) {
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

    clients[id].is_active = false;
    clients[id].mouse.is_active = false;
    num_active_clients--;
    if (clients[id].is_controlling) num_controlling_clients--;
    if (disconnect_client(id) != 0) {
        LOG_ERROR("Failed to disconnect client. (ID: %d)", id);
        return -1;
    }
    return 0;
}

int quit_clients(void) {
    /*
        Deactivates all active clients. Disconnects client. Updates count
        of active clients. The associated client objects are not destroyed
        and may be made active in the future.

        NOTE: Needs write is_active_rwlock and (write) state_lock

        Returns:
            (int): -1 on failure, 0 on success
    */

    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            if (quit_client(id) != 0) {
                LOG_ERROR("Failed to quit client. (ID: %d)", id);
                ret = -1;
            }
        }
    }
    return ret;
}

int exists_timed_out_client(double timeout, bool *exists) {
    /*
        Determines if any active client has timed out. Checks the time
        of the last received ping for each active client. If the time
        since the server has received a ping from a client equals or
        exceeds the timeout threshold, the client is considered to have
        timed out.

        NOTE: Needs read is_active_rwlock

        Arguments:
            timeout (double): Duration (in seconds) after which an active
                client is deemed timed out if the server has not received
                a ping from the client.
            exists (bool*): The field pointed to by exists is set to true
                if one or more clients are timed out client and false
                otherwise.

        Returns:
            (int): Returns -1 on failure, 0 on success. Whether or not
                there exists a timed out client does not mean failure.
    */

    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            if (get_timer(clients[id].last_ping) > timeout) {
                *exists = true;
                return 0;
            }
        }
    }
    *exists = false;
    return 0;
}

int reap_timed_out_clients(double timeout) {
    /*
        Quits all timed out clients.

        NOTE: Needs write is_active_rwlock and (write) state_lock

        Arguments:
            timeout (double): Duration (in seconds) after which a
                client is deemed timed out if the server has not received
                a ping from the client.

        Returns:
            (int): Returns -1 on failure, 0 on success.
    */

    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active && get_timer(clients[id].last_ping) > timeout) {
            LOG_INFO("Dropping client ID: %d", id);
            // indicate that a client has exited nongracefully and is being reaped
            client_exited_nongracefully = true;
            start_timer((clock *)&last_nongraceful_exit);
            if (quit_client(id) != 0) {
                LOG_ERROR("Failed to quit client. (ID: %d)", id);
                ret = -1;
            }
        }
    }
    return ret;
}

int try_find_client_id_by_user_id(int user_id, bool *found, int *id) {
    /*
        Finds the client ID of the active client object associated with
        a user_id, if there is one. Only searches currently active clients.

        NOTES: Needs read is_active_rwlock

        Arguments:
            user_id (int): User ID to be searched for.
            found (bool*): Populated with true if an associated client
                ID is found, false otherwise.
            id (int*): Populated with found client ID, if one is found.

        Retrurns:
            (int): Returns -1 on failure, 0 on success. Not finding an
                associated ID does not mean failure.
    */

    *found = false;
    for (int i = 0; i < MAX_NUM_CLIENTS; i++) {
        if (clients[i].is_active && clients[i].user_id == user_id) {
            *id = i;
            *found = true;
            return 0;
        }
    }
    return 0;
}

int get_available_client_id(int *id) {
    /*
        Finds an available client ID. If a client object is inactive,
        that object and the associated client ID are available for
        re-use. Function fails if no client ID is available.

        NOTES: Needs read is_active_rwlock
               Does not set up the client or make it active

        Arguments:
            id (int*): Points to field which function populates
                with available client ID.

        Returns:
            (int): Returns -1 on failure (including no client IDs
                are available), 0 on success.
    */

    for (int i = 0; i < MAX_NUM_CLIENTS; i++) {
        if (!clients[i].is_active) {
            *id = i;
            return 0;
        }
    }
    LOG_WARNING("No available client slots.");
    return -1;
}

int fill_peer_update_messages(PeerUpdateMessage *msgs, size_t *num_msgs) {
    /*
        Fills buffer with status info for every active client. Status
        info includes mouse position, interaction mode, and more.

        NOTE: Needs read is_active_rwlock and (read) state_lock

        Arguments:
            msgs (PeerUpdateMessage*): Buffer to be filled with peer
                update info. Must be at least as large as the number
                of presently active clients times the size of
                each PeerUpdateMessage.
            num_msgs (size_t*): Number of messages filled by function.

        Returns:
            (int): Returns -1 on failure, 0 on success.
    */

    *num_msgs = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active && clients[id].mouse.is_active) {
            msgs->peer_id = id;
            msgs->x = clients[id].mouse.x;
            msgs->y = clients[id].mouse.y;
            msgs->is_controlling = clients[id].is_controlling;
            memcpy(&(msgs->color), &(clients[id].mouse.color), sizeof(FractalRGBColor));
            msgs++;
            (*num_msgs)++;
        }
    }
    return 0;
}
