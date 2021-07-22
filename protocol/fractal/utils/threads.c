#include "threads.h"
#include <fractal/logging/logging.h>

#ifdef __linux__
// Manual pthread control
#include <pthread.h>
#endif

void fractal_init_multithreading() {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_THREAD_FORCE_REALTIME_TIME_CRITICAL, "1");
    SDL_Init(SDL_INIT_VIDEO);
}

FractalThread fractal_create_thread(FractalThreadFunction thread_function, char *thread_name,
                                    void *data) {
    FractalThread ret = SDL_CreateThread(thread_function, thread_name, data);
    if (ret == NULL) {
        LOG_FATAL("Failure creating thread: %s", SDL_GetError());
    }
    return ret;
}

void fractal_detach_thread(FractalThread thread) { SDL_DetachThread(thread); }

void fractal_wait_thread(FractalThread thread, int *ret) { SDL_WaitThread(thread, ret); }

void fractal_set_thread_priority(FractalThreadPriority priority) {
    // Add in special case handling when trying to set specifically REALTIME on Linux,
    // As this leads to increased performance
#ifdef __linux__
    // Use the highest possible priority for _REALTIME
    // (SCHED_RR being higher than any possible nice value, which is SCHED_OTHER)
    if (priority == FRACTAL_THREAD_PRIORITY_REALTIME) {
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
            fractal_set_thread_priority(FRACTAL_THREAD_PRIORITY_HIGH);
            return;
        }
        // Set the priority to the maximum possible priority
        if (pthread_setschedparam(this_thread, SCHED_RR, &params) != 0) {
            // Try _HIGH instead of _REALTIME on logged error
            LOG_ERROR("Failure calling pthread_setschedparam()");
            fractal_set_thread_priority(FRACTAL_THREAD_PRIORITY_HIGH);
            return;
        }
        return;
    }
#endif
    // This is the general cross-platform way, so that
    // we can set any thread priority on any operating system
    if (SDL_SetThreadPriority((SDL_ThreadPriority)priority) < 0) {
        LOG_ERROR("Failure setting thread priority: %s", SDL_GetError());
        if (priority == FRACTAL_THREAD_PRIORITY_REALTIME) {
            // REALTIME requires sudo/admin on unix/windows.
            // So, if we fail to set the REALTIME priority, we should at least try _HIGH
            fractal_set_thread_priority(FRACTAL_THREAD_PRIORITY_HIGH);
        }
    }
}

void fractal_sleep(uint32_t ms) { SDL_Delay(ms); }

FractalMutex fractal_create_mutex() {
    FractalMutex ret = SDL_CreateMutex();
    if (ret == NULL) {
        LOG_FATAL("Failure creating mutex: %s", SDL_GetError());
    }
    return ret;
}

void fractal_lock_mutex(FractalMutex mutex) {
    if (SDL_LockMutex(mutex) < 0) {
        LOG_FATAL("Failure locking mutex: %s", SDL_GetError());
    }
}

int fractal_try_lock_mutex(FractalMutex mutex) {
    int status = SDL_TryLockMutex(mutex);
    if (status == 0 || status == SDL_MUTEX_TIMEDOUT) {
        return status;
    } else {
        LOG_FATAL("Failure try-locking mutex: %s", SDL_GetError());
    }
}

void fractal_unlock_mutex(FractalMutex mutex) {
    if (SDL_UnlockMutex(mutex) < 0) {
        LOG_FATAL("Failure unlocking mutex: %s", SDL_GetError());
    }
}

void fractal_destroy_mutex(FractalMutex mutex) { SDL_DestroyMutex(mutex); }

FractalCondition fractal_create_cond() {
    FractalCondition ret = SDL_CreateCond();
    if (ret == NULL) {
        LOG_FATAL("Failure creating condition variable: %s", SDL_GetError());
    }
    return ret;
}

void fractal_wait_cond(FractalCondition cond, FractalMutex mutex) {
    if (SDL_CondWait(cond, mutex) < 0) {
        LOG_FATAL("Failure waiting on condition variable: %s", SDL_GetError());
    }
}

void fractal_broadcast_cond(FractalCondition cond) {
    if (SDL_CondBroadcast(cond) < 0) {
        LOG_FATAL("Failure broadcasting condition variable: %s", SDL_GetError());
    }
}

void fractal_destroy_cond(FractalCondition cond) { SDL_DestroyCond(cond); }

FractalSemaphore fractal_create_semaphore(uint32_t initial_value) {
    FractalSemaphore ret = SDL_CreateSemaphore(initial_value);
    if (ret == NULL) {
        LOG_FATAL("Failure creating semaphore: %s", SDL_GetError());
    }
    return ret;
}

void fractal_post_semaphore(FractalSemaphore semaphore) {
    if (SDL_SemPost(semaphore) < 0) {
        LOG_FATAL("Failure posting semaphore: %s", SDL_GetError());
    }
}

void fractal_wait_semaphore(FractalSemaphore semaphore) {
    if (SDL_SemWait(semaphore) < 0) {
        LOG_FATAL("Failure waiting on semaphore: %s", SDL_GetError());
    }
}

void fractal_destroy_semaphore(FractalSemaphore semaphore) { SDL_DestroySemaphore(semaphore); }
