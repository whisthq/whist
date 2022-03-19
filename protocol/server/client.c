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

/*
============================
Defines
============================
*/

// The maximum amount of time for a lock to be held, in seconds
#define MAX_DEACTIVATION_TIME_SEC 10.0

struct ClientPrivateData {
    bool is_permanently_deactivated;  // Whether or not the client has permanently deactivated
    // Mutex protecting the cond
    WhistMutex cond_mutex;
    // Cond that gets signaled on activation or permanent deactivation
    WhistCondition cond;
    // Read lock is held by anyone holding a client active lock
    RWLock activation_rwlock;
};

// Represents an actively held activation lock on the client
struct ClientLock {
    // The Client that this ClientLock refers to
    Client* client;
};

/*
============================
Public Functions
============================
*/

Client* init_client() {
    Client* client = safe_malloc(sizeof(Client));

    client->is_active = false;
    client->is_deactivating = false;

    client->private_data = safe_malloc(sizeof(ClientPrivateData));
    client->private_data->is_permanently_deactivated = false;
    client->private_data->cond_mutex = whist_create_mutex();
    client->private_data->cond = whist_create_cond();
    init_rw_lock(&client->private_data->activation_rwlock);

    return client;
}

void destroy_client(Client* client) {
    // Destroy everything
    destroy_rw_lock(&client->private_data->activation_rwlock);
    whist_destroy_cond(client->private_data->cond);
    whist_destroy_mutex(client->private_data->cond_mutex);
    free(client->private_data);
    free(client);
}

void activate_client(Client* client) {
    whist_lock_mutex(client->private_data->cond_mutex);

    // Mark as active
    LOG_INFO("Client Active");
    client->is_active = true;
    // Wake up everyone waiting on client activation
    whist_broadcast_cond(client->private_data->cond);

    whist_unlock_mutex(client->private_data->cond_mutex);
}

void start_deactivating_client(Client* client) {
    whist_lock_mutex(client->private_data->cond_mutex);

    // Verify that this is an active client
    FATAL_ASSERT(client->is_active);

    // Mark as deactivating, if not already marked as such
    client->is_deactivating = true;

    whist_unlock_mutex(client->private_data->cond_mutex);
}

static int verify_deactivation(void* p_deactivation_finished_raw) {
    WhistSemaphore verify_deactivation_semaphore = (WhistSemaphore)(p_deactivation_finished_raw);

    // Wait for the semaphore, but die if too much time has passed
    if (!whist_wait_timeout_semaphore(verify_deactivation_semaphore,
                                      MAX_DEACTIVATION_TIME_SEC * MS_IN_SECOND)) {
        // Forcefully exit Whist if the locks aren't being unlocked as they should
        // This is for protection against thread crashes or lock misuse
        LOG_FATAL("Failed to deactivate after %f seconds!", MAX_DEACTIVATION_TIME_SEC);
    }

    whist_destroy_semaphore(verify_deactivation_semaphore);

    return 0;
}

void deactivate_client(Client* client) {
    // Verify that this is a client being deactivated
    FATAL_ASSERT(client->is_active);
    FATAL_ASSERT(client->is_deactivating);

    // Allow this thread to cleanup a failed deactivation
    // This to prevent lock starvation from causing the mandelbox to hang
    WhistSemaphore verify_deactivation_semaphore = whist_create_semaphore(0);
    WhistThread verify_deactivation_thread = whist_create_thread(
        verify_deactivation, "Deactivation Thread", (void*)verify_deactivation_semaphore);

    // Wait for no one to hold an activation lock anymore
    // (No new locks will be held after this point, since is_deactivating == true)
    write_lock(&client->private_data->activation_rwlock);
    write_unlock(&client->private_data->activation_rwlock);

    // Mark deactivation as finished
    whist_post_semaphore(verify_deactivation_semaphore);
    whist_wait_thread(verify_deactivation_thread, NULL);

    // Update the client state is inactive
    whist_lock_mutex(client->private_data->cond_mutex);

    client->is_deactivating = false;
    LOG_INFO("Client Inactive");
    client->is_active = false;

    whist_unlock_mutex(client->private_data->cond_mutex);
}

void permanently_deactivate_client(Client* client) {
    whist_lock_mutex(client->private_data->cond_mutex);

    // Verify that the client isn't active
    FATAL_ASSERT(!client->is_active);

    // Mark as permanently inactive
    client->private_data->is_permanently_deactivated = true;
    // Wake up everyone waiting on client activation, since the client will never activate
    whist_broadcast_cond(client->private_data->cond);

    whist_unlock_mutex(client->private_data->cond_mutex);
}

ClientLock* client_active_lock(Client* client) {
    whist_lock_mutex(client->private_data->cond_mutex);

    ClientLock* client_lock = safe_malloc(sizeof(ClientLock));
    client_lock->client = client;

    while (true) {
        // If it's already active and we're not trying to deactivate, we're done
        if (client->is_active && !client->is_deactivating) {
            // Grab an activation lock
            read_lock(&client->private_data->activation_rwlock);
            break;
        } else if (client->private_data->is_permanently_deactivated) {
            // Note status as permanently deactivated and exit
            free(client_lock);
            client_lock = NULL;
            break;
        }

        // Otherwise, wait on the next new connection
        // (Or, a permanently deactivated client)
        whist_wait_cond(client->private_data->cond, client->private_data->cond_mutex);
    }

    whist_unlock_mutex(client->private_data->cond_mutex);

    return client_lock;
}

ClientLock* client_active_trylock(Client* client) {
    whist_lock_mutex(client->private_data->cond_mutex);

    ClientLock* client_lock = safe_malloc(sizeof(ClientLock));
    client_lock->client = client;

    // If it's already active and we're not trying to deactivate, we're done
    if (client->is_active && !client->is_deactivating) {
        // Grab an activation lock
        read_lock(&client->private_data->activation_rwlock);
    } else {
        // Could not grab the lock
        free(client_lock);
        client_lock = NULL;
    }

    whist_unlock_mutex(client->private_data->cond_mutex);

    return client_lock;
}

void client_active_unlock(ClientLock* client_lock) {
    // Verify the lock is valid, and that the client is active
    FATAL_ASSERT(client_lock != NULL);
    FATAL_ASSERT(client_lock->client->is_active);

    // Let go of the activation lock
    read_unlock(&client_lock->client->private_data->activation_rwlock);
    free(client_lock);
}
