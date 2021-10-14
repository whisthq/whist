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
    bool is_active;       // "protected" by global `client_deactivating`

    /* USER INFO */
    int user_id;  // not lock protected

    /* NETWORK */
    SocketContext udp_context;  // "protected" by global `client_deactivating`
    SocketContext tcp_context;  // "protected" by global `client_deactivating`
    int udp_port;               // "protected" by global `client_deactivating`
    int tcp_port;               // "protected" by global `client_deactivating`
    RWLock tcp_rwlock;          // protects tcp_context for synchrony-sensitive sends and recvs

    /* PING */
    clock last_ping;      // not lock protected
    clock last_tcp_ping;  // not lock protected

} Client;

extern volatile bool client_deactivating;

extern Client client;

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
 * @returns                        Returns -1 on failure, 0 on success
 */
int init_client(void);

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

*
* @returns                        Returns -1 on failure, 0 on success
*/
int quit_client();

/**
 * @brief                          Quit client if timed out.
 *
 * @param timeout                  Duration (in seconds) after which a client
 *                                 is deemed timed out if the server has not
 *                                 received a ping from the client.
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int reap_timed_out_client(double timeout);

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
 * @returns                        Returns -1 on failure, 0 on success. Not
 *                                 finding an associated ID does not mean
 *                                 failure.
 */
int does_client_match_user_id(int user_id, bool *found);

void add_thread_to_client_active_dependents();
void reset_threads_holding_active_count();
void update_client_active_status(bool* thread_holding_active);
int start_quitting_client();

#endif  // SERVER_CLIENT_H
