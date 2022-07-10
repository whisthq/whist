#include <whist/core/whist.h>
extern "C" {
#include "threads.h"
}
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <memory>

// Include for thread priority
#if OS_IS(OS_WIN32)
// Manual Winthread control
#include <processthreadsapi.h>
#else
// Manual pthread control
#include <pthread.h>
// setpriority(3) on Linux
#if OS_IS(OS_LINUX)
#include <sys/resource.h>
#endif
#endif

struct WhistThreadStruct {
    std::string thread_name;
    std::shared_ptr<int> ret_ptr;
    std::thread whist_thread;
    WhistThreadStruct(std::string param_thread_name, WhistThreadFunction thread_function,
                      void *data)
        : thread_name(param_thread_name), ret_ptr(std::make_shared<int>(0)) {
        // Keep a local reference to ret_ptr
        std::shared_ptr<int> local_ret_ptr = this->ret_ptr;
        // Call thread_function(data) in its own std::thread,
        this->whist_thread = std::thread([local_ret_ptr, thread_function, data]() -> void {
            // Call, and store the return value into the shared_ptr
            *local_ret_ptr = thread_function(data);
        });
    }
};

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
    LOG_INFO("Created from thread %lx", whist_get_thread_id(NULL));
    return ret;
}

void whist_detach_thread(WhistThread thread) {
    LOG_INFO("Detaching thread \"%s\" from thread %lx", thread->thread_name.c_str(),
             whist_get_thread_id(thread));
    // Detach the thread, and delete its struct
    thread->whist_thread.detach();
    delete thread;
    LOG_INFO("Detached from thread %lx", whist_get_thread_id(NULL));
}

void whist_wait_thread(WhistThread thread, int *ret) {
    LOG_INFO("Waiting on thread \"%s\" from thread %lx", thread->thread_name.c_str(),
             whist_get_thread_id(thread));
    // Wait for the thread to finish executing,
    thread->whist_thread.join();
    // Copy out the return value, if the caller wants it
    if (ret != NULL) {
        // "join" creates a memory barrier, so this is safe
        *ret = *thread->ret_ptr;
    }
    // And then delete the thread struct
    delete thread;
    LOG_INFO("Waited from thread %lx", whist_get_thread_id(NULL));
}

bool whist_try_set_thread_priority(WhistThreadPriority priority) {
#if OS_IS(OS_WIN32)
    int priority_value;
    if (priority == WHIST_THREAD_PRIORITY_LOW) {
        priority_value = THREAD_PRIORITY_LOWEST;
    } else if (priority == WHIST_THREAD_PRIORITY_HIGH) {
        priority_value = THREAD_PRIORITY_HIGHEST;
    } else if (priority == WHIST_THREAD_PRIORITY_REALTIME) {
        priority_value = THREAD_PRIORITY_TIME_CRITICAL;
    } else {
        priority_value = THREAD_PRIORITY_NORMAL;
    }
    return SetThreadPriority(GetCurrentThread(), priority_value);
#else

    // Mac/Linux code for setting priority

    struct sched_param sched;
    int policy;
    pthread_t thread = pthread_self();

    if (pthread_getschedparam(thread, &policy, &sched) != 0) {
        LOG_ERROR("pthread_getschedparam(3) failed");
        return false;
    }

    /* SDL Comment: Higher priority levels may require changing the pthread scheduler policy
     * for the thread. */
    switch (priority) {
        case WHIST_THREAD_PRIORITY_LOW:
        case WHIST_THREAD_PRIORITY_NORMAL:
            policy = SCHED_OTHER;
            break;
        case WHIST_THREAD_PRIORITY_HIGH:
#if OS_IS(OS_MAC)
            /* SDL Comment: Apple requires SCHED_RR for high priority threads */
            policy = SCHED_RR;
#else
            policy = SCHED_OTHER;
#endif
            break;
        case WHIST_THREAD_PRIORITY_REALTIME:
            // WHIST COMMENT:
            // SDL uses SCHED_OTHER on Linux,
            // But we use SCHED_RR since it's better
            policy = SCHED_RR;
            break;
        default:
            LOG_FATAL("Invalid enum value");
    }

#if OS_IS(OS_LINUX)
    // SCHED_OTHER requires setpriority(3) on Linux,
    // pthread_setschedparam(3) won't work.
    if (policy == SCHED_OTHER) {
        // Set priority for non-realtime Linux priorities
        int os_priority;
        if (priority == WHIST_THREAD_PRIORITY_LOW) {
            os_priority = 19;
        } else if (priority == WHIST_THREAD_PRIORITY_HIGH) {
            os_priority = -10;
        } else if (priority == WHIST_THREAD_PRIORITY_REALTIME) {
            os_priority = -20;
        } else {
            os_priority = 0;
        }

        pid_t linux_tid = syscall(SYS_gettid);
        if (setpriority(PRIO_PROCESS, (id_t)linux_tid, os_priority) == 0) {
            return true;
        } else {
            LOG_ERROR("setpriority(3) failed");
            return false;
        }
    }
#endif

    // Set priority using pthread_setschedparam(3)

    // Get min/max priority
    errno = 0;
    int min_priority = sched_get_priority_min(policy);
    if (min_priority == -1 && errno != 0) {
        LOG_ERROR("Failed to get min sched priority");
        return false;
    }
    errno = 0;
    int max_priority = sched_get_priority_max(policy);
    // Check for error
    if (max_priority == -1 && errno != 0) {
        LOG_ERROR("Failed to get min sched priority");
        return false;
    }

    // Update the sched priority struct
    if (priority == WHIST_THREAD_PRIORITY_LOW) {
        sched.sched_priority = min_priority;
    } else if (priority == WHIST_THREAD_PRIORITY_REALTIME) {
        sched.sched_priority = max_priority;
    } else {
#if OS_IS(OS_MAC)
        // Apple-specific code
        if (min_priority == 15 && max_priority == 47) {
            /* Apple has a specific set of thread priorities */
            if (priority == WHIST_THREAD_PRIORITY_HIGH) {
                sched.sched_priority = 45;
            } else {
                sched.sched_priority = 37;
            }
        } else
#endif
        {
            // Use WHIST_THREAD_PRIORITY_NORMAL = 50% priority
            // And WHIST_THREAD_PRIORITY_HIGH = 75% priority
            sched.sched_priority = (min_priority + (max_priority - min_priority) / 2);
            if (priority == WHIST_THREAD_PRIORITY_HIGH) {
                sched.sched_priority += ((max_priority - min_priority) / 4);
            }
        }
    }

    // Update the priority, and return success code
    if (pthread_setschedparam(thread, policy, &sched) == 0) {
        return true;
    } else {
        LOG_ERROR("pthread_setschedparam(3) failed");
        return false;
    }
#endif /* Not windows */
}

void whist_set_thread_priority(WhistThreadPriority priority) {
    // This is the general cross-platform way, so that
    // we can set any thread priority on any operating system
    if (!whist_try_set_thread_priority(priority)) {
        LOG_ERROR("Failure setting thread priority to: %d", (int)priority);
        if (priority == WHIST_THREAD_PRIORITY_REALTIME) {
            // REALTIME requires sudo/admin on unix/windows.
            // So, if we fail to set the REALTIME priority, we should at least try _HIGH
            if (!whist_try_set_thread_priority(WHIST_THREAD_PRIORITY_HIGH)) {
                LOG_ERROR("Setting thread priority to REALTIME failed, and HIGH failed as well!");
            } else {
                LOG_INFO("REALTIME priority has fallen back to HIGH priority");
            }
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
