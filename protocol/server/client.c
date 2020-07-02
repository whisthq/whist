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

// locks shouldn't matter. they are getting created.
int initClients(void) {
    if ((state_lock = SDL_CreateMutex()) == 0) {
        LOG_ERROR("Failed to create state lock.");
        return -1;
    }
    if (initRWLock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to create is active read-write lock.");
        SDL_DestroyMutex(state_lock);
        return -1;
    }
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        clients[id].is_active = false;

        clients[id].UDP_port = BASE_UDP_PORT + id;
        clients[id].TCP_port = BASE_TCP_PORT + id;

        memcpy(&(clients[id].mouse.color), &(MOUSE_COLORS[id]),
               sizeof(RGB_Color));
    }
    return 0;
}

// locks shouldn't matter. they are getting trashed.
int destroyClients(void) {
    SDL_DestroyMutex(state_lock);
    if (destroyRWLock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to destroy is active read-write lock.");
        return -1;
    }
    return 0;
}

// Needs write is_active_rwlock and (write) state lock
int quitClient(int id) {
    clients[id].is_active = false;
    clients[id].mouse.is_active = false;
    num_active_clients--;
    if (clients[id].is_controlling)
        num_controlling_clients--;
    if (disconnectClient(id) != 0) {
        LOG_ERROR("Failed to disconnect client. (ID: %d)", id);
        return -1;
    }
    return 0;
}

// Needs write is_active_rwlock and (write) state lock
int quitClients(void) {
    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            if (quitClient(id) != 0) {
                LOG_ERROR("Failed to quit client. (ID: %d)", id);
                ret = -1;
            }
        }
    }
    return ret;
}

// needs read is_active_rwlock
int existsTimedOutClient(double timeout, bool *exists) {
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            if (GetTimer(clients[id].last_ping) > timeout) {
                *exists = true;
                return 0;
            }
        }
    }
    *exists = false;
    return 0;
}

// Needs write is_active_rwlock and (write) state lock
int reapTimedOutClients(double timeout) {
    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active &&
            GetTimer(clients[id].last_ping) > timeout) {
            LOG_INFO("Dropping client ID: %d", id);
            if (quitClient(id) != 0) {
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
int tryFindClientIdByUsername(int username, bool *found, int *id) {
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
int getAvailableClientID(int *id) {
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
int fillPeerUpdateMessages(PeerUpdateMessage *msgs, size_t *num_msgs) {
    *num_msgs = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active && clients[id].mouse.is_active) {
            msgs->peer_id = id;
            msgs->x = clients[id].mouse.x;
            msgs->y = clients[id].mouse.y;
            msgs->is_controlling = clients[id].is_controlling;
            memcpy(&(msgs->color), &(clients[id].mouse.color),
                   sizeof(RGB_Color));
            msgs++;
            (*num_msgs)++;
        }
    }
    return 0;
}
