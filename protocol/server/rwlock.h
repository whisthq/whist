#ifndef RWLOCK_H
#define RWLOCK_H

#include "../fractal/core/fractal.h"

typedef struct RWLock {
    int num_active_readers;
    SDL_mutex *num_active_readers_lock;
    SDL_cond *no_active_readers_cond;

    int num_writers;
    SDL_mutex *num_writers_lock;
    SDL_cond *no_writers_cond;

    bool is_active_writer;
    SDL_mutex *is_active_writer_lock;
    SDL_cond *no_active_writer_cond;
} RWLock;

int initRWLock(RWLock *rwlock);

int destroyRWLock(RWLock *rwlock);

int writeLock(RWLock *rwlock);

int readLock(RWLock *rwlock);

int writeUnlock(RWLock *rwlock);

int readUnlock(RWLock *rwlock);

#endif  // RWLOCK_H
