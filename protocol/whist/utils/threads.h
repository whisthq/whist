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

typedef SDL_mutex* WhistMutex;
typedef SDL_cond* WhistCondition;
typedef SDL_sem* WhistSemaphore;
typedef SDL_Thread* WhistThread;
typedef SDL_threadID WhistThreadID;
typedef int (*WhistThreadFunction)(void*);
typedef enum WhistThreadPriority {
    WHIST_THREAD_PRIORITY_LOW = SDL_THREAD_PRIORITY_LOW,
    WHIST_THREAD_PRIORITY_NORMAL = SDL_THREAD_PRIORITY_NORMAL,
    WHIST_THREAD_PRIORITY_HIGH = SDL_THREAD_PRIORITY_HIGH,
    WHIST_THREAD_PRIORITY_REALTIME = SDL_THREAD_PRIORITY_TIME_CRITICAL
} WhistThreadPriority;

void whist_init_multithreading(void);

WhistThread whist_create_thread(WhistThreadFunction thread_function, const char* thread_name,
                                void* data);
WhistThreadID whist_get_thread_id(WhistThread thread);
void whist_detach_thread(WhistThread thread);
void whist_wait_thread(WhistThread thread, int* ret);
void whist_set_thread_priority(WhistThreadPriority priority);

void whist_sleep(uint32_t ms);
void whist_usleep(uint32_t us);

WhistMutex whist_create_mutex(void);
void whist_lock_mutex(WhistMutex mutex);
int whist_try_lock_mutex(WhistMutex mutex);
void whist_unlock_mutex(WhistMutex mutex);
void whist_destroy_mutex(WhistMutex mutex);

WhistCondition whist_create_cond(void);
void whist_wait_cond(WhistCondition cond, WhistMutex mutex);
void whist_broadcast_cond(WhistCondition cond);
void whist_destroy_cond(WhistCondition cond);

WhistSemaphore whist_create_semaphore(uint32_t initial_value);
void whist_post_semaphore(WhistSemaphore semaphore);
void whist_wait_semaphore(WhistSemaphore semaphore);
void whist_destroy_semaphore(WhistSemaphore semaphore);

#endif  // WHIST_THREADS_H
