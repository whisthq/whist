#ifndef WHIST_THREADS_H
#define WHIST_THREADS_H

// So that SDL sees symbols such as memcpy
#ifdef _WIN32
#include <windows.h>
#else
#include <string.h>
#endif

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

typedef SDL_mutex* FractalMutex;
typedef SDL_cond* FractalCondition;
typedef SDL_sem* FractalSemaphore;
typedef SDL_Thread* FractalThread;
typedef SDL_threadID FractalThreadID;
typedef int (*FractalThreadFunction)(void*);
typedef enum FractalThreadPriority {
    WHIST_THREAD_PRIORITY_LOW = SDL_THREAD_PRIORITY_LOW,
    WHIST_THREAD_PRIORITY_NORMAL = SDL_THREAD_PRIORITY_NORMAL,
    WHIST_THREAD_PRIORITY_HIGH = SDL_THREAD_PRIORITY_HIGH,
    WHIST_THREAD_PRIORITY_REALTIME = SDL_THREAD_PRIORITY_TIME_CRITICAL
} FractalThreadPriority;

void fractal_init_multithreading();

FractalThread fractal_create_thread(FractalThreadFunction thread_function, const char* thread_name,
                                    void* data);
FractalThreadID fractal_get_thread_id(FractalThread thread);
void fractal_detach_thread(FractalThread thread);
void fractal_wait_thread(FractalThread thread, int* ret);
void fractal_set_thread_priority(FractalThreadPriority priority);

void fractal_sleep(uint32_t ms);
void fractal_usleep(uint32_t us);

FractalMutex fractal_create_mutex();
void fractal_lock_mutex(FractalMutex mutex);
int fractal_try_lock_mutex(FractalMutex mutex);
void fractal_unlock_mutex(FractalMutex mutex);
void fractal_destroy_mutex(FractalMutex mutex);

FractalCondition fractal_create_cond();
void fractal_wait_cond(FractalCondition cond, FractalMutex mutex);
void fractal_broadcast_cond(FractalCondition cond);
void fractal_destroy_cond(FractalCondition cond);

FractalSemaphore fractal_create_semaphore(uint32_t initial_value);
void fractal_post_semaphore(FractalSemaphore semaphore);
void fractal_wait_semaphore(FractalSemaphore semaphore);
void fractal_destroy_semaphore(FractalSemaphore semaphore);

#endif  // WHIST_THREADS_H
