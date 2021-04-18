#ifndef FRACTAL_THREADS_H
#define FRACTAL_THREADS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

typedef SDL_mutex* FractalMutex;
typedef SDL_cond* FractalCondition;
typedef SDL_sem* FractalSemaphore;

typedef int (*FractalThreadFunction)(void*);
typedef SDL_Thread* FractalThread;

void fractal_init_multithreading();
FractalThread fractal_create_thread(FractalThreadFunction thread_function, char *thread_name, void *data);
void fractal_detach_thread(FractalThread thread);
void fractal_wait_thread(FractalThread thread, int *ret);

void fractal_sleep(uint32_t ms);

/**
 * @brief                          Wrapper around SDL_CreateMutex that will correctly exit the
 *                                 protocol when SDL_LockMutex fails
 */
FractalMutex fractal_create_mutex();

/**
 * @brief                          Wrapper around SDL_LockMutex that will correctly exit the
 *                                 protocol when SDL_LockMutex fails
 */
void fractal_lock_mutex(FractalMutex mutex);

/**
 * @brief                          Wrapper around SDL_TryLockMutex that will correctly exit the
 *                                 protocol when SDL_TryLockMutex fails
 */
int fractal_try_lock_mutex(FractalMutex mutex);

/**
 * @brief                          Wrapper around SDL_UnlockMutex that will correctly exit the
 * protocol when SDL_UnlockMutex fails
 */
void fractal_unlock_mutex(FractalMutex mutex);

/**
 * @brief                          Wrapper around SDL_CondWait that will correctly exit the
 *                                 protocol when SDL_LockMutex fails
 */
 
FractalCondition fractal_create_cond();
void fractal_wait_cond(FractalCondition cond, FractalMutex mutex);
void fractal_broadcast_cond(FractalCondition cond);
void fractal_destroy_cond(FractalCondition cond);
void fractal_destroy_mutex(FractalMutex mutex);

FractalSemaphore fractal_create_semaphore(uint32_t initial_value);
void fractal_post_semaphore(FractalSemaphore semaphore);
void fractal_wait_semaphore(FractalSemaphore semaphore);

typedef enum FractalThreadPriority {
    FRACTAL_THREAD_PRIORITY_LOW = SDL_THREAD_PRIORITY_LOW,
    FRACTAL_THREAD_PRIORITY_NORMAL = SDL_THREAD_PRIORITY_NORMAL,
    FRACTAL_THREAD_PRIORITY_HIGH = SDL_THREAD_PRIORITY_HIGH
} FractalThreadPriority;

void fractal_set_thread_priority(FractalThreadPriority priority);

#endif  // FRACTAL_THREADS_H
