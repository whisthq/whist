/* A writer-preferred read-writer lock */

/*
 * num_writers includes any waiting writers and any active writer
 */
#include "rwlock.h"
#include <fractal/core/fractal.h>

void init_rw_lock(RWLock *rwlock) {
    rwlock->num_active_readers = 0;
    rwlock->num_writers = 0;
    rwlock->is_active_writer = false;

    rwlock->num_active_readers_lock = fractal_create_mutex();
    rwlock->num_writers_lock = fractal_create_mutex();
    rwlock->is_active_writer_lock = fractal_create_mutex();

    rwlock->no_active_readers_cond = fractal_create_cond();
    rwlock->no_writers_cond = fractal_create_cond();
    rwlock->no_active_writer_cond = fractal_create_cond();
}

void destroy_rw_lock(RWLock *rwlock) {
    fractal_destroy_mutex(rwlock->num_active_readers_lock);
    fractal_destroy_mutex(rwlock->num_writers_lock);
    fractal_destroy_mutex(rwlock->is_active_writer_lock);

    fractal_destroy_cond(rwlock->no_active_readers_cond);
    fractal_destroy_cond(rwlock->no_writers_cond);
    fractal_destroy_cond(rwlock->no_active_writer_cond);
}

void write_lock(RWLock *rwlock) {
    // Increment number of writers
    fractal_lock_mutex(rwlock->num_writers_lock);
    rwlock->num_writers++;
    fractal_unlock_mutex(rwlock->num_writers_lock);

    // Wait for all of the active readers to go home
    fractal_lock_mutex(rwlock->num_active_readers_lock);
    while (rwlock->num_active_readers > 0) {
        fractal_wait_cond(rwlock->no_active_readers_cond, rwlock->num_active_readers_lock);
    }
    fractal_unlock_mutex(rwlock->num_active_readers_lock);

    // Wait for the active writer to go home
    fractal_lock_mutex(rwlock->is_active_writer_lock);
    while (rwlock->is_active_writer) {
        fractal_wait_cond(rwlock->no_active_writer_cond, rwlock->is_active_writer_lock);
    }

    // Take the only active writer spot
    rwlock->is_active_writer = true;
    fractal_unlock_mutex(rwlock->is_active_writer_lock);
}

void read_lock(RWLock *rwlock) {
    // Wait for all of the writers to go home
    fractal_lock_mutex(rwlock->num_writers_lock);
    while (rwlock->num_writers > 0) {
        fractal_wait_cond(rwlock->no_writers_cond, rwlock->num_writers_lock);
    }
    // Increment the number of active readers
    fractal_lock_mutex(rwlock->num_active_readers_lock);
    rwlock->num_active_readers++;
    fractal_unlock_mutex(rwlock->num_active_readers_lock);

    fractal_unlock_mutex(rwlock->num_writers_lock);
}

void write_unlock(RWLock *rwlock) {
    // Set is active writer to false
    fractal_lock_mutex(rwlock->is_active_writer_lock);
    rwlock->is_active_writer = false;
    fractal_unlock_mutex(rwlock->is_active_writer_lock);
    // Wake up anyone waiting for a change in is active writer
    fractal_broadcast_cond(rwlock->no_active_writer_cond);

    // Decrement the number of writers
    fractal_lock_mutex(rwlock->num_writers_lock);
    rwlock->num_writers--;
    if (rwlock->num_writers == 0) {
        fractal_broadcast_cond(rwlock->no_writers_cond);
    }
    fractal_unlock_mutex(rwlock->num_writers_lock);
}

void read_unlock(RWLock *rwlock) {
    // Decrement the number of active readers
    fractal_lock_mutex(rwlock->num_active_readers_lock);
    rwlock->num_active_readers--;
    // If there are no more active readers, wake up anyone waiting for there
    // to be no active readers
    if (rwlock->num_active_readers == 0) {
        fractal_broadcast_cond(rwlock->no_active_readers_cond);
    }
    fractal_unlock_mutex(rwlock->num_active_readers_lock);
}
