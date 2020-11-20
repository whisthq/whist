#include "../fractal/utils/mouse.h"
#include "../fractal/utils/rwlock.h"
#include "client.h"
#include "network.h"

SDL_mutex *state_lock;
RWLock is_active_rwlock;

Client clients[MAX_NUM_CLIENTS];

int num_controlling_clients = 0;
int num_active_clients = 0;
int host_id = -1;

volatile bool client_exited_nongracefully =
    false;  // set to true after a nongraceful exit (only exit logic can set to false)
volatile clock last_nongraceful_exit;  // start this after every nongraceful exit

// locks shouldn't matter. they are getting created.
int init_clients(void) {
    if ((state_lock = SDL_CreateMutex()) == 0) {
        LOG_ERROR("Failed to create state lock.");
        return -1;
    }
    if (init_rw_lock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to create is active read-write lock.");
        SDL_DestroyMutex(state_lock);
        return -1;
    }
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        clients[id].is_active = false;

        clients[id].UDP_port = BASE_UDP_PORT + id;
        clients[id].TCP_port = BASE_TCP_PORT + id;

        memcpy(&(clients[id].mouse.color), &(mouse_colors[id]), sizeof(FractalRGBColor));
    }
    return 0;
}

// locks shouldn't matter. they are getting trashed.
int destroy_clients(void) {
    SDL_DestroyMutex(state_lock);
    if (destroy_rw_lock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to destroy is active read-write lock.");
        return -1;
    }
    return 0;
}

// Needs write is_active_rwlock and (write) state lock
int quit_client(int id) {
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

// Needs write is_active_rwlock and (write) state lock
int quit_clients(void) {
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

// needs read is_active_rwlock
int exists_timed_out_client(double timeout, bool *exists) {
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

// Needs write is_active_rwlock and (write) state lock
int reap_timed_out_clients(double timeout) {
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

// Needs read is_active_rwlock
// You should quit this guy soon after
// Only searches currently active clients
int try_find_client_id_by_username(int username, bool *found, int *id) {
    *found = false;
    for (int i = 0; i < MAX_NUM_CLIENTS; i++) {
        if (clients[i].is_active && clients[i].username == username) {
            *id = i;
            *found = true;
            return 0;
        }
    }
    return 0;
}

// Needs read is_active_rwlock
// Does not set up the client or make it active
int get_available_client_id(int *id) {
    for (int i = 0; i < MAX_NUM_CLIENTS; i++) {
        if (!clients[i].is_active) {
            *id = i;
            return 0;
        }
    }
    LOG_WARNING("No available client slots.");
    return -1;
}

// needs read-is active lock and (read) mouse/is_controlling lock
int fill_peer_update_messages(PeerUpdateMessage *msgs, size_t *num_msgs) {
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
