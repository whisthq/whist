#ifndef RWLOCK_H
#define RWLOCK_H
/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file rwlock.h
 * @brief This file contains a write-preferred read-write lock.
============================
Usage
============================

RWLocks must be initialized before use and should be destroyed before program
exit. RWLocks are not allocated or freed by init_rw_lock or destroyRWLock,
respectively.

Only one thread may write-hold the RWLock at once. Arbitrarily many threads may
read-hold the RWLock at once.

The RWLock is write-preferred. If a writer is waiting for the lock and a reader
requests the lock, the writer will always be granted the lock before the reader.
If multiple writers wait for the lock at once, the lock will be granted to the
writers, one at a time, in the order of their requesting the lock.

The functions for acquiring the RWLock are blocking and will block indefinitely.
*/

/*
============================
Includes
============================
*/

#include <fractal/core/whist.h>

/*
============================
Custom types
============================
*/

typedef struct RWLock {
    int num_active_readers;
    WhistMutex num_active_readers_lock;
    WhistCondition no_active_readers_cond;

    int num_writers;
    WhistMutex num_writers_lock;
    WhistCondition no_writers_cond;

    bool is_active_writer;
    WhistMutex is_active_writer_lock;
    WhistCondition no_active_writer_cond;
} RWLock;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initializes read-write lock.
 *
 * @param rwlock                   Lock to be initialized.
 */
void init_rw_lock(RWLock *rwlock);

/**
 * @brief                          Destroys read-write lock.
 *
 * @details                        Does not free the lock itself, though this
 *                                 function frees any additional memory allocated
 *                                 in init_rw_lock.
 *
 * @param rwlock                   Lock to be destroyed.
 */
void destroy_rw_lock(RWLock *rwlock);

/**
 * @brief                          Write-acquires read-write lock.
 *
 * @details                        Blocks until the lock is acquired.
 *
 * @param rwlock                   Lock to be acquired.
 */
void write_lock(RWLock *rwlock);

/**
 * @brief                          Read-acquires read-write lock.
 *
 * @details                        Blocks until the lock is acquired.
 *
 * @param rwlock                   Lock to be acquired.
 */
void read_lock(RWLock *rwlock);

/**
 * @brief                          Write-releases read-write lock.
 *
 * @param rwlock                   Lock to be released.
 */
void write_unlock(RWLock *rwlock);

/**
 * @brief                          Read-releases read-write lock.
 *
 * @param rwlock                   Lock to be released.
 */
void read_unlock(RWLock *rwlock);

#endif  // RWLOCK_H
