#include "threads.h"
#include <fractal/utils/logging.h>

void fractal_init_multithreading() {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);
}

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

FractalCondition fractal_create_cond() {
    FractalCondition ret = SDL_CreateCond();
    if (ret == NULL) {
        LOG_FATAL("Failure creating condition variable: %s", SDL_GetError());
    }
    return ret;
}

void fractal_destroy_cond(FractalCondition cond) {
    SDL_DestroyCond(cond);
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

void fractal_destroy_mutex(FractalMutex mutex) {
    SDL_DestroyMutex(mutex);
}

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

FractalThread fractal_create_thread(FractalThreadFunction thread_function, char *thread_name, void *data) {
    FractalThread ret = SDL_CreateThread(thread_function, thread_name, data);
    if (ret == NULL) {
        LOG_FATAL("Failure creating thread: %s",
        SDL_GetError());
    }
    return ret;
}

void fractal_detach_thread(FractalThread thread) {
    SDL_DetachThread(thread);
}
void fractal_wait_thread(FractalThread thread, int *ret) {
    SDL_WaitThread(thread, ret);
}
void fractal_sleep(uint32_t ms) {
    SDL_Delay(ms);
}

void fractal_set_thread_priority(FractalThreadPriority priority) {
    if (SDL_SetThreadPriority((SDL_ThreadPriority) priority) < 0) {
        LOG_FATAL("Failure setting thread priority: %s", SDL_GetError());
    }
}
