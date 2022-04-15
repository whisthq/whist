#include "threads.h"
#include <whist/logging/logging.h>

#ifdef __linux__
// Manual pthread control
#include <pthread.h>
#endif

void whist_init_multithreading(void) {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_THREAD_FORCE_REALTIME_TIME_CRITICAL, "1");
}

WhistThreadID whist_get_thread_id(WhistThread thread) {
    // `thread` == NULL returns the current thread ID
    return SDL_GetThreadID(thread);
}

WhistThread whist_create_thread(WhistThreadFunction thread_function, const char *thread_name,
                                void *data) {
    LOG_INFO("Creating thread \"%s\" from thread %lx", thread_name, whist_get_thread_id(NULL));
    WhistThread ret = SDL_CreateThread(thread_function, thread_name, data);
    if (ret == NULL) {
        LOG_FATAL("Failure creating thread: %s", SDL_GetError());
    }
    LOG_INFO("Created from thread %lx", whist_get_thread_id(NULL));
    return ret;
}

void whist_detach_thread(WhistThread thread) {
    LOG_INFO("Detaching thread \"%s\" from thread %lx", SDL_GetThreadName(thread),
             whist_get_thread_id(NULL));
    SDL_DetachThread(thread);
    LOG_INFO("Detached from thread %lx", whist_get_thread_id(NULL));
}

void whist_wait_thread(WhistThread thread, int *ret) {
    LOG_INFO("Waiting on thread \"%s\" from thread %lx", SDL_GetThreadName(thread),
             whist_get_thread_id(NULL));
    SDL_WaitThread(thread, ret);
    LOG_INFO("Waited from thread %lx", whist_get_thread_id(NULL));
}

void whist_set_thread_priority(WhistThreadPriority priority) {
    // Add in special case handling when trying to set specifically REALTIME on Linux,
    // As this leads to increased performance
#ifdef __linux__
    // Use the highest possible priority for _REALTIME
    // (SCHED_RR being higher than any possible nice value, which is SCHED_OTHER)
    if (priority == WHIST_THREAD_PRIORITY_REALTIME) {
        char err[1024];
        // Get the identifier for the thread that called this function
        pthread_t this_thread = pthread_self();
        struct sched_param params;
        errno = 0;
        // Get the maximum possible priority that exists
        params.sched_priority = sched_get_priority_max(SCHED_RR);
        // Check for error
        if (params.sched_priority == -1 && errno != 0) {
            // Try _HIGH instead of _REALTIME on logged error
            strerror_r(errno, err, sizeof(err));
            LOG_ERROR("Failure calling sched_get_priority_max(): %s", err);
            whist_set_thread_priority(WHIST_THREAD_PRIORITY_HIGH);
            return;
        }
        // Set the priority to the maximum possible priority
        if (pthread_setschedparam(this_thread, SCHED_RR, &params) != 0) {
            // Try _HIGH instead of _REALTIME on logged error
            LOG_ERROR("Failure calling pthread_setschedparam()");
            whist_set_thread_priority(WHIST_THREAD_PRIORITY_HIGH);
            return;
        }
        return;
    }
#endif
    // This is the general cross-platform way, so that
    // we can set any thread priority on any operating system
    if (SDL_SetThreadPriority((SDL_ThreadPriority)priority) < 0) {
        LOG_ERROR("Failure setting thread priority: %s", SDL_GetError());
        if (priority == WHIST_THREAD_PRIORITY_REALTIME) {
            // REALTIME requires sudo/admin on unix/windows.
            // So, if we fail to set the REALTIME priority, we should at least try _HIGH
            whist_set_thread_priority(WHIST_THREAD_PRIORITY_HIGH);
        }
    }
}

WhistThreadLocalStorageKey whist_create_thread_local_storage(void) {
    WhistThreadLocalStorageKey key = SDL_TLSCreate();
    if (key == 0) {
        LOG_FATAL("Failure creating thread local storage: %s", SDL_GetError());
    }
    return key;
}

void whist_set_thread_local_storage(WhistThreadLocalStorageKey key, void *data,
                                    WhistThreadLocalStorageDestructor destructor) {
    if (SDL_TLSSet(key, data, destructor) < 0) {
        LOG_FATAL("Failure setting thread local storage: %s", SDL_GetError());
    }
}

void *whist_get_thread_local_storage(WhistThreadLocalStorageKey key) { return SDL_TLSGet(key); }

void whist_sleep(uint32_t ms) { SDL_Delay(ms); }

void whist_usleep(uint32_t us) {
#ifdef _WIN32
    // No clean usleep() on Windows, so just fall back to SDL_Delay.
    if (us > 0 && us < 1000) {
        // Ensure that short sleeps do something.
        whist_sleep(1);
    } else {
        whist_sleep(us / 1000);
    }
#else
    usleep(us);
#endif  // _WIN32
}

WhistMutex whist_create_mutex(void) {
    WhistMutex ret = SDL_CreateMutex();
    if (ret == NULL) {
        LOG_FATAL("Failure creating mutex: %s", SDL_GetError());
    }
    return ret;
}

void whist_lock_mutex(WhistMutex mutex) {
    if (SDL_LockMutex(mutex) < 0) {
        LOG_FATAL("Failure locking mutex: %s", SDL_GetError());
    }
}

int whist_try_lock_mutex(WhistMutex mutex) {
    int status = SDL_TryLockMutex(mutex);
    if (status == 0 || status == SDL_MUTEX_TIMEDOUT) {
        return status;
    } else {
        LOG_FATAL("Failure try-locking mutex: %s", SDL_GetError());
    }
}

void whist_unlock_mutex(WhistMutex mutex) {
    if (SDL_UnlockMutex(mutex) < 0) {
        LOG_FATAL("Failure unlocking mutex: %s", SDL_GetError());
    }
}

void whist_destroy_mutex(WhistMutex mutex) { SDL_DestroyMutex(mutex); }

WhistCondition whist_create_cond(void) {
    WhistCondition ret = SDL_CreateCond();
    if (ret == NULL) {
        LOG_FATAL("Failure creating condition variable: %s", SDL_GetError());
    }
    return ret;
}

void whist_wait_cond(WhistCondition cond, WhistMutex mutex) {
    if (SDL_CondWait(cond, mutex) < 0) {
        LOG_FATAL("Failure waiting on condition variable: %s", SDL_GetError());
    }
}

void whist_broadcast_cond(WhistCondition cond) {
    // A spurious wake, can cause a wait_cond thread to not actually be waiting, during this
    // broadcast. Thus, the mutex must be locked in order to guarantee a wakeup
    if (SDL_CondBroadcast(cond) < 0) {
        LOG_FATAL("Failure broadcasting condition variable: %s", SDL_GetError());
    }
}

void whist_destroy_cond(WhistCondition cond) { SDL_DestroyCond(cond); }

WhistSemaphore whist_create_semaphore(uint32_t initial_value) {
    WhistSemaphore ret = SDL_CreateSemaphore(initial_value);
    if (ret == NULL) {
        LOG_FATAL("Failure creating semaphore: %s", SDL_GetError());
    }
    return ret;
}

void whist_post_semaphore(WhistSemaphore semaphore) {
    if (SDL_SemPost(semaphore) < 0) {
        LOG_FATAL("Failure posting semaphore: %s", SDL_GetError());
    }
}

void whist_wait_semaphore(WhistSemaphore semaphore) {
    if (SDL_SemWait(semaphore) < 0) {
        LOG_FATAL("Failure waiting on semaphore: %s", SDL_GetError());
    }
}

bool whist_wait_timeout_semaphore(WhistSemaphore semaphore, int timeout_ms) {
    int ret = SDL_SemWaitTimeout(semaphore, timeout_ms);
    if (ret == SDL_MUTEX_TIMEDOUT) {
        return false;
    } else if (ret == 0) {
        return true;
    } else {
        LOG_FATAL("Failure waiting on semaphore: %s", SDL_GetError());
    }
}

uint32_t whist_semaphore_value(WhistSemaphore semaphore) { return SDL_SemValue(semaphore); }

void whist_destroy_semaphore(WhistSemaphore semaphore) { SDL_DestroySemaphore(semaphore); }
