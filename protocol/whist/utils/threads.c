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

WhistThread whist_create_thread(WhistThreadFunction thread_function, const char *thread_name,
                                void *data) {
    WhistThread ret = SDL_CreateThread(thread_function, thread_name, data);
    if (ret == NULL) {
        LOG_FATAL("Failure creating thread: %s", SDL_GetError());
    }
    return ret;
}

WhistThreadID whist_get_thread_id(WhistThread thread) {
    // `thread` == NULL returns the current thread ID
    return SDL_GetThreadID(thread);
}

void whist_detach_thread(WhistThread thread) { SDL_DetachThread(thread); }

void whist_wait_thread(WhistThread thread, int *ret) { SDL_WaitThread(thread, ret); }

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

void whist_sleep(uint32_t ms) { SDL_Delay(ms); }

void whist_usleep(uint32_t us) {
#ifdef _WIN32
    // Not sure if this is implemented on Windows, so just fall back to SDL_Delay.
    whist_sleep(us / 1000);
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

uint32_t whist_value_semapore(WhistSemaphore semaphore) { return SDL_SemValue(semaphore); }

void whist_destroy_semaphore(WhistSemaphore semaphore) { SDL_DestroySemaphore(semaphore); }
