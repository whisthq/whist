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

#include "atomic.h"

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

void whist_init_multithreading();

WhistThread whist_create_thread(WhistThreadFunction thread_function, const char* thread_name,
                                void* data);
WhistThreadID whist_get_thread_id(WhistThread thread);
void whist_detach_thread(WhistThread thread);
void whist_wait_thread(WhistThread thread, int* ret);
void whist_set_thread_priority(WhistThreadPriority priority);

void whist_sleep(uint32_t ms);
void whist_usleep(uint32_t us);

WhistMutex whist_create_mutex();
void whist_lock_mutex(WhistMutex mutex);
int whist_try_lock_mutex(WhistMutex mutex);
void whist_unlock_mutex(WhistMutex mutex);
void whist_destroy_mutex(WhistMutex mutex);

WhistCondition whist_create_cond();
void whist_wait_cond(WhistCondition cond, WhistMutex mutex);
void whist_broadcast_cond(WhistCondition cond);
void whist_destroy_cond(WhistCondition cond);

WhistSemaphore whist_create_semaphore(uint32_t initial_value);
void whist_post_semaphore(WhistSemaphore semaphore);
void whist_wait_semaphore(WhistSemaphore semaphore);
void whist_destroy_semaphore(WhistSemaphore semaphore);

/**
 * @brief Type used with whist_once().
 *
 * MUST be initialised with WHIST_ONCE_INIT.
 */
typedef struct {
    // Bits 0-28 = number of waiters.
    // Bit 29 = init running.
    // Bit 30 = init complete.
    atomic_int state;
#if _MSC_VER
    struct {
        void* value;
    } semaphore;
#elif __cplusplus
    std::atomic<void*> semaphore;
#else /* C11 */
    void* _Atomic semaphore;
#endif
} WhistOnce;

/**
 * Static initialiser for WhistOnce.
 */
#define WHIST_ONCE_INIT \
    { ATOMIC_VAR_INIT(0), ATOMIC_VAR_INIT(NULL) }

/**
 * @brief Ensure that a function has run exactly once.
 *
 * When this function returns, init_function will have been called exactly
 * once in some thread and run to completion regardless of how many times
 * and in what threads whist_once() has been called.  Once the function
 * has finished, the overhead of additional calls to whist_once() is
 * approximately zero - a single load and comparison.
 *
 * This can be useful to perform some sort of global initialisation step
 * for a component without requiring any additional user interaction.  An
 * alternative is to provide a global function and require that the user
 * call it and allow it to complete before calling any other functions in
 * that component.  The tradeoff here is between requiring callers to
 * always remember the global function versus having the once call on
 * every new entrypoint to the component.
 *
 * With a global init function:
 * @code{.c}
 * void something_init(void) {
 *     precalculate_something_tables();
 *     create_something_mutexes();
 * }
 *
 * void create_something(...) {
 *     ...
 * }
 *
 * void use_something(...) {
 *     ...
 * }
 *
 * // Then, in the main function of every program and the constructor of
 * // every library using the something component:
 * something_init();
 * @endcode
 *
 * Using whist_once():
 * @code{.c}
 * static WhistOnce something_once = WHIST_ONCE_INIT;
 *
 * static void something_init(void) {
 *     precalculate_something_tables();
 *     create_something_mutexes();
 * }
 *
 * void create_something(...) {
 *     whist_once(&something_once, &something_init);
 *     ...
 * }
 *
 * void use_something(...) {
 *     ...
 * }
 *
 * // No additional code required in callers.
 * @endcode
 *
 * @param once           WhistOnce instance.
 *                       This MUST be initialised to WHIST_ONCE_INIT before use!
 * @param init_function  Function to be called.
 */
void whist_once(WhistOnce* once, void (*init_function)(void));

#endif  // WHIST_THREADS_H
