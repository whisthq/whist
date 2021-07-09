#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file client.h
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

#include <fractal/core/fractal.h>
#include <fractal/utils/clock.h>
#include <fractal/utils/rwlock.h>

/*
============================
Custom types
============================
*/

typedef struct Client {
    /* ACTIVE */
    bool is_active;       // protected by global is_active_rwlock
    bool is_controlling;  // protected by state lock
    bool is_host;         // protected by state lock

    /* USER INFO */
    int user_id;  // not lock protected

    /* NETWORK */
    SocketContext UDP_context;  // protected by global is_active_rwlock
    SocketContext TCP_context;  // protected by global is_active_rwlock
    int UDP_port;               // protected by global is_active_rwlock
    int TCP_port;               // protected by global is_active_rwlock

    /* MOUSE */
    struct {
        int x;
        int y;
        struct {
            int r;
            int g;
            int b;
        } color;
        bool is_active;
    } mouse;  // protected by state lock

    /* PING */
    clock last_ping;  // not lock protected

} Client;

extern FractalMutex state_lock;
extern RWLock is_active_rwlock;

extern Client clients[MAX_NUM_CLIENTS];

extern int num_controlling_clients;
extern int num_active_clients;
extern int host_id;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initializes all clients objects in the client
 *                                 buffer.
 *
 * @details                        Must be called before the client buffer
 *                                 can be used.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int init_clients(void);

/**
 * @brief                          De-initializes all clients objects in the
 *                                 client buffer.
 *
 * @details                        Should be called after initClients() and
 *                                 before program exit. Does not disconnect any
 *                                 connected clients.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int destroy_clients(void);

/**
* @brief                          Deactivates active client.
*
* @details                        Disconnects client. Updates count of active
*                                 clients. May only be called on an active
*                                 client. The associated client object is
*                                 not destroyed and may be made active in the
*                                 future.

* @param id                       Client ID of active client to deactivate
*
* @returns                        Returns -1 on failure, 0 on success
*/
int quit_client(int id);

/**
 * @brief                          Deactivates all active clients.
 *
 * @details                        Disconnects client. Updates count of active
 *                                 clients. The associated client objects are
 *                                 not destroyed and may be made active in the
 *                                 future.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int quit_clients(void);

/**
 * @brief                          Determines if any active client has timed out.
 *
 * @details                        Checks the time of the last received ping for
 *                                 each active client. If the time since the
 *                                 server has received a ping from a client
 *                                 equals or exceeds the timeout threshold, the
 *                                 client is considered to have timed out.
 *
 * @param timeout                  Duration (in seconds) after which an active
 *                                 client is deemed timed out if the server has
 *                                 not received a ping from the client.
 *
 * @param exists                   The field pointed to by exists is set to true
 *                                 if one or more clients are timed out client
 *                                 and false otherwise.
 *
 * @returns                        Returns -1 on failure, 0 on success. Whether
 *                                 or not there exists a timed out client does
 *                                 not mean failure.
 */
int exists_timed_out_client(double timeout, bool *exists);

/**
 * @brief                          Quits all timed out clients.
 *
 * @param timeout                  Duration (in seconds) after which a client
 *                                 is deemed timed out if the server has not
 *                                 received a ping from the client.
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int reap_timed_out_clients(double timeout);

/**
 * @brief                          Finds the client ID of the active client
 *                                 object associated with a user ID, if there is
 *                                 one.
 *
 * @param user_id                  User ID to be searched for.
 *
 * @param found                    Populated with true if an associated client ID
 *                                 is found, false otherwise.
 *
 * @param id                       Populated with found client ID, if one is
 *                                 found.
 *
 * @returns                        Returns -1 on failure, 0 on success. Not
 *                                 finding an associated ID does not mean
 *                                 failure.
 */
int try_find_client_id_by_user_id(int user_id, bool *found, int *id);

/**
 * @brief                          Finds an available client ID.
 *
 * @details                        If a client object is inactive, that object
 *                                 and the associated client ID are
 *                                 available for re-use. Function fails if no
 *                                 client ID is available.
 *
 * @param id                       Points to field which function populates
 *                                 with available client ID.
 *
 * @returns                        Returns -1 on failure (including no
 *                                 client IDs are available), 0 on success.
 */
int get_available_client_id(int *id);

/**
 * @brief                          Fills buffer with status info for every
 *                                 active client.
 *
 * @details                        Status info includes mouse position,
 *                                 interaction mode, and more.
 *
 * @param msgs                     Buffer to be filled with peer update info.
 *                                 Must be at least as large as the number
 *                                 of presently active clients times the size of
 *                                 each PeerUpdateMessage.
 *
 * @param num_msgs                 Number of messages filled by function.
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int fill_peer_update_messages(PeerUpdateMessage *msgs, size_t *num_msgs);

#endif  // SERVER_CLIENT_H
