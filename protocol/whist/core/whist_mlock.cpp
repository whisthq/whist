#include "whist_mlock.h"
#include <map>
#include <malloc/malloc.h>
extern "C" {
#include <sys/mman.h>
#include <mach/vm_map.h>
#include <mach/vm_types.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <whist/logging/log_statistic.h>
#include <stdlib.h>
#include <unistd.h>
}

#if USING_MLOCK
struct MLockRegion {
    void* addr;
    size_t size;

    bool operator<(const MLockRegion& region) const {
        if (this->addr < region.addr) {
            return true;
        }
        if (this->addr > region.addr) {
            return false;
        }
        return this->size < region.size;
    }

    bool operator==(const MLockRegion& region) const {
        return this->addr == region.addr && this->size == region.size;
    }
};

struct MComparator {
    bool operator()(const MLockRegion& a, const MLockRegion& b) const {
        if (a.addr < b.addr) {
            return true;
        }
        if (a.addr > b.addr) {
            return false;
        }
        return a.size < b.size;
    }
};

static size_t page_size;
static malloc_zone_t* original_zone;
static void* (*system_malloc)(malloc_zone_t* zone, size_t size);
static void (*system_free)(malloc_zone_t* zone, void* ptr);
static void (*system_free_definite_size)(malloc_zone_t* zone, void* ptr, size_t size);

template <class T>
struct Mallocator
{
  typedef T value_type;
 
  Mallocator () = default;
  template <class U> constexpr Mallocator (const Mallocator <U>&) noexcept {}
 
  [[nodiscard]] T* allocate(std::size_t n) {
    void* ret = system_malloc(original_zone, n * sizeof(T));
    if (ret) {
        // LOG_INFO("custom allocator %p size %zx", ret, n * sizeof(T));
        return (T*) ret;
    }
 
    throw std::bad_alloc();
  }
 
  void deallocate(T* p, std::size_t n) noexcept {
    // LOG_INFO("custom free %p size %zx", p, n);
    system_free(original_zone, (void*) p);
      // system_free_definite_size(original_zone, (void*) p, n);
  }
};
 
template <class T, class U>
bool operator==(const Mallocator <T>&, const Mallocator <U>&) { return true; }
template <class T, class U>
bool operator!=(const Mallocator <T>&, const Mallocator <U>&) { return false; }

// map addr -> MLockRegion*
// map MLockRegion (or MLockRegion*) -> number of mallocs
struct MlockContext {
    WhistMutex lock;
    std::map<void*, MLockRegion, std::less<void*>, Mallocator<std::pair<void* const, MLockRegion>>> malloc_to_region;
    std::map<MLockRegion, int, MComparator, Mallocator<std::pair<const MLockRegion, int>>> region_to_count;
};
// static thread_local int counter;

void* whist_mlock_malloc(malloc_zone_t* zone, size_t size) {
    // static int counter = 0;
    // LOG_INFO("Calling custom malloc with mlock, counter %d", counter);
    // counter++;
    void* ret = system_malloc(zone, size);
    // LOG_INFO("whist_mlock_malloc for %p, size %zx", ret, size);
    if (ret) {
        whist_mlock(ret, size);
    }
    // counter--;
    return ret;
}

void whist_mlock_free(malloc_zone_t* zone, void* ptr) {
    // LOG_INFO("whist_mlock_free for %p", ptr);
    if (ptr) {
        whist_munlock(ptr);
        system_free(zone, ptr);
    }
}

void whist_mlock_free_definite_size(malloc_zone_t* zone, void* ptr, size_t size) {
    if (ptr) {
        whist_munlock(ptr);
        system_free_definite_size(zone, ptr, size);
    }
}

static MlockContext mlock_context;

void init_whist_mlock() {
    LOG_INFO("Initializing custom malloc");
    mlock_context.lock = whist_create_mutex();
    page_size = sysconf(_SC_PAGESIZE);
    original_zone = malloc_default_zone();
    system_malloc = original_zone->malloc;
    system_free = original_zone->free;
    system_free_definite_size = original_zone->free_definite_size;
    original_zone->malloc = whist_mlock_malloc;
    original_zone->free = whist_mlock_free;
    original_zone->free_definite_size = whist_mlock_free_definite_size;
}

void whist_mlock(void* addr, size_t size) {
    // LOG_INFO("Calling whist_mlock...");
    // static WhistTimer mach_timer;
    // static int num_calls = 0;
    // static double total_time = 0;
    whist_lock_mutex(mlock_context.lock);
    // LOG_INFO("Locked mutex");
    MLockRegion region = {.addr = (void*)((uintptr_t)addr - ((uintptr_t)addr % page_size)),
                          .size = size + page_size - (size % page_size)};
    // LOG_INFO("region %p, size %zx", region.addr, region.size);
    mlock_context.malloc_to_region[addr] = region;
    // LOG_INFO("Recorded malloc to region entry");
    if (mlock_context.region_to_count.contains(region)) {
        mlock_context.region_to_count[region]++;
        // LOG_INFO("Page already locked, with count %d, doing nothing", mlock_context.region_to_count[region]);
    } else {
        // LOG_INFO("New page to lock");
        mlock_context.region_to_count[region] = 1;
        mlock(region.addr, region.size);
        // LOG_INFO("Mlocking region %p size %zx", region.addr, region.size);
    }
    whist_unlock_mutex(mlock_context.lock);
    // LOG_INFO("Unlocked mutex");
}

void whist_munlock(void* addr) {
    // LOG_INFO("Calling whist_munlock...");
    // static WhistTimer timer;
    // start_timer(&timer);
    whist_lock_mutex(mlock_context.lock);
    // LOG_INFO("Locked munlock mutex");
    if (mlock_context.malloc_to_region.contains(addr)) {
        MLockRegion region = mlock_context.malloc_to_region[addr];
        if (mlock_context.region_to_count.contains(region)) {
            mlock_context.region_to_count[region]--;
            if (mlock_context.region_to_count[region] == 0) {
                munlock(region.addr, region.size);
                mlock_context.region_to_count.erase(region);
                // LOG_INFO("Munlock'ed region %p size %zx", region.addr, region.size);
            } else {
                // LOG_INFO("Region still used by %d, doing nothing", mlock_context.region_to_count[region]);
            }
        } else {
            // LOG_ERROR("Couldn't find region with addr %p size %zx in map", region.addr,
                      // region.size);
        }
                mlock_context.malloc_to_region.erase(addr);
    } else {
        // LOG_ERROR("Tried to munlock an address %p that wasn't recorded", addr);
    }
    whist_unlock_mutex(mlock_context.lock);
    // LOG_INFO("Unlocked munlock mutex");
    // log_double_statistic(MUNLOCK_TIME, get_timer(&timer) * MS_IN_SECOND);
}

/*
void * operator new(std::size_t n) noexcept(false)
{
    // printf("Using custom new");
    return safe_malloc(n);
}
void operator delete(void * p) throw()
{
    // printf("Using custom delete");
     whist_free(p);
}
*/

#else
void init_whist_mlock() { return; }
void whist_mlock(void* addr, size_t size) { return; }
void whist_munlock(void* addr) { return; }
#endif
