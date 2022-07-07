#include <whist/core/whist.h>
extern "C" {
#include "threads.h"
}
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

struct WhistThreadStruct {
    std::string thread_name;
    int ret_value;
    std::thread whist_thread;
    WhistThreadStruct(std::string param_thread_name, WhistThreadFunction thread_function,
                      void *data)
        : thread_name(param_thread_name),
          ret_value(0),
          whist_thread([this, thread_function, data]() -> void {
              int ret = thread_function(data);
              this->ret_value = ret;
          }) {}
};

#if OS_IS(OS_LINUX)
// Manual pthread control
#include <pthread.h>
#endif

void whist_init_multithreading(void) {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_THREAD_FORCE_REALTIME_TIME_CRITICAL, "1");
}

WhistThreadID whist_get_thread_id(WhistThread thread) {
    // `thread` == NULL returns the current thread ID
    if (thread == NULL) {
        return (WhistThreadID)std::hash<std::thread::id>{}(std::this_thread::get_id());
    } else {
        return (WhistThreadID)std::hash<std::thread::id>{}(thread->whist_thread.get_id());
    }
}

WhistThread whist_create_thread(WhistThreadFunction thread_function, const char *thread_name,
                                void *data) {
    if (thread_name == NULL) {
        thread_name = "[[UNNAMED]]";
    }
    LOG_INFO("Creating thread \"%s\" from thread %lx", thread_name, whist_get_thread_id(NULL));
    WhistThread ret = new WhistThreadStruct(thread_name, thread_function, data);
    if (ret == NULL) {
        LOG_FATAL("Failure creating thread: %s", SDL_GetError());
    }
    LOG_INFO("Created from thread %lx", whist_get_thread_id(NULL));
    return ret;
}

void whist_detach_thread(WhistThread thread) {
    LOG_INFO("Detaching thread \"%s\" from thread %lx", thread->thread_name.c_str(),
             whist_get_thread_id(thread));
    std::thread detach_thread([thread]() -> void {
        thread->whist_thread.join();
        // We delete after joining,
        // to ensure that "ret" isn't written to after the struct has been freed
        delete thread;
    });
    detach_thread.detach();
    LOG_INFO("Detached from thread %lx", whist_get_thread_id(NULL));
}

void whist_wait_thread(WhistThread thread, int *ret) {
    LOG_INFO("Waiting on thread \"%s\" from thread %lx", thread->thread_name.c_str(),
             whist_get_thread_id(thread));
    thread->whist_thread.join();
    if (ret != NULL) {
        *ret = thread->ret_value;
    }
    delete thread;
    LOG_INFO("Waited from thread %lx", whist_get_thread_id(NULL));
}

void whist_set_thread_priority(WhistThreadPriority priority) {
    // Add in special case handling when trying to set specifically REALTIME on Linux,
    // As this leads to increased performance
#if OS_IS(OS_LINUX)
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

class WhistThreadLocalStorageContainer {
   public:
    void *data = nullptr;
    WhistThreadLocalStorageDestructor data_destructor = nullptr;
    WhistThreadLocalStorageContainer() {}
    WhistThreadLocalStorageContainer(void *ptr, WhistThreadLocalStorageDestructor destructor)
        : data(ptr), data_destructor(destructor) {}
    // Custom destructor
    ~WhistThreadLocalStorageContainer() {
        if (this->data_destructor != nullptr) {
            this->data_destructor(this->data);
        }
    }
    // Rule of three
    WhistThreadLocalStorageContainer(const WhistThreadLocalStorageContainer &other) = delete;
    WhistThreadLocalStorageContainer &operator=(const WhistThreadLocalStorageContainer &other) =
        delete;
    // Move-assignment
    WhistThreadLocalStorageContainer &operator=(WhistThreadLocalStorageContainer &&other) {
        std::swap(this->data, other.data);
        std::swap(this->data_destructor, other.data_destructor);
        return *this;
    }
};

// 1, so that a default-initalized WhistThreadLocalStorageKey won't exist in tls_map
static thread_local WhistThreadLocalStorageKey next_key = 1;
static thread_local std::map<WhistThreadLocalStorageKey, WhistThreadLocalStorageContainer> tls_map;

WhistThreadLocalStorageKey whist_create_thread_local_storage(void) {
    WhistThreadLocalStorageKey current_key = next_key++;
    tls_map[current_key] = WhistThreadLocalStorageContainer();
    return current_key;
}

void whist_set_thread_local_storage(WhistThreadLocalStorageKey key, void *data,
                                    WhistThreadLocalStorageDestructor destructor) {
    // Replace the storage container with the new (data, destructor) pair
    tls_map.at(key) = WhistThreadLocalStorageContainer(data, destructor);
}

void *whist_get_thread_local_storage(WhistThreadLocalStorageKey key) {
    return tls_map.at(key).data;
}

void whist_sleep(uint32_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

void whist_usleep(uint32_t us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }

struct WhistMutexStruct {
    std::recursive_mutex mutex;
};

WhistMutex whist_create_mutex() { return new WhistMutexStruct(); }

void whist_lock_mutex(WhistMutex mutex) { mutex->mutex.lock(); }

int whist_try_lock_mutex(WhistMutex mutex) {
    // Return 0 on success, 1 on failure
    return mutex->mutex.try_lock() ? 0 : 1;
}

void whist_unlock_mutex(WhistMutex mutex) { mutex->mutex.unlock(); }

void whist_destroy_mutex(WhistMutex mutex) { delete mutex; }

struct WhistConditionStruct {
    std::condition_variable_any cond;
};

WhistCondition whist_create_cond(void) { return new WhistConditionStruct(); }

void whist_wait_cond(WhistCondition cond, WhistMutex mutex) { cond->cond.wait(mutex->mutex); }

bool whist_timedwait_cond(WhistCondition cond, WhistMutex mutex, uint32_t timeout_ms) {
    std::cv_status status =
        cond->cond.wait_for(mutex->mutex, std::chrono::milliseconds(timeout_ms));
    if (status == std::cv_status::timeout) {
        return false;
    } else {
        return true;
    }
}

void whist_broadcast_cond(WhistCondition cond) {
    // A spurious wake, can cause a wait_cond thread to not actually be waiting, during this
    // broadcast. Thus, the mutex must be locked in order to guarantee a wakeup
    cond->cond.notify_all();
}

void whist_destroy_cond(WhistCondition cond) { delete cond; }

struct WhistSemaphoreStruct {
    // Note: std::counting_semaphore doesn't compile on Mac
    std::mutex mutex;
    std::condition_variable_any condvar;
    uint32_t count;
};

WhistSemaphore whist_create_semaphore(uint32_t initial_value) {
    WhistSemaphoreStruct *ret = new WhistSemaphoreStruct();
    ret->count = initial_value;
    return ret;
}

void whist_post_semaphore(WhistSemaphore semaphore) {
    std::unique_lock<std::mutex> lock(semaphore->mutex);
    semaphore->count++;
    semaphore->condvar.notify_one();
}

void whist_wait_semaphore(WhistSemaphore semaphore) {
    std::unique_lock<std::mutex> lock(semaphore->mutex);
    semaphore->condvar.wait(semaphore->mutex, [semaphore] { return semaphore->count > 0; });
    semaphore->count--;
}

bool whist_wait_timeout_semaphore(WhistSemaphore semaphore, int timeout_ms) {
    std::unique_lock<std::mutex> lock(semaphore->mutex);
    bool wait_succeeded =
        semaphore->condvar.wait_for(semaphore->mutex, std::chrono::milliseconds(timeout_ms),
                                    [semaphore] { return semaphore->count > 0; });
    if (wait_succeeded) {
        semaphore->count--;
        return true;
    } else {
        return false;
    }
}

uint32_t whist_semaphore_value(WhistSemaphore semaphore) {
    std::unique_lock<std::mutex> lock(semaphore->mutex);
    return semaphore->count;
}

void whist_destroy_semaphore(WhistSemaphore semaphore) { delete semaphore; }
