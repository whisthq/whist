#include "whist_mlock.h"
#include <map>
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

// map addr -> MLockRegion*
// map MLockRegion (or MLockRegion*) -> number of mallocs
struct MlockContext {
    WhistMutex lock;
    std::map<void*, MLockRegion> malloc_to_region;
    std::map<MLockRegion, int> region_to_count;
};

static MlockContext mlock_context;
static size_t page_size;

void init_whist_mlock() {
    mlock_context.lock = whist_create_mutex();
    page_size = sysconf(_SC_PAGESIZE);
}

void whist_mlock(void* addr, size_t size) {
    static WhistTimer mach_timer;
    start_timer(&mach_timer);
    whist_lock_mutex(mlock_context.lock);
    MLockRegion region = {.addr = (void*)((uintptr_t)addr - ((uintptr_t)addr % page_size)),
                          .size = size + page_size - (size % page_size)};
    mlock_context.malloc_to_region[addr] = region;
    if (mlock_context.region_to_count.contains(region)) {
        mlock_context.region_to_count[region]++;
    } else {
        mlock_context.region_to_count[region] = 1;
        mlock(region.addr, region.size);
        LOG_INFO("Mlocking region %p size %zx", region.addr, region.size);
    }
    whist_unlock_mutex(mlock_context.lock);
    log_double_statistic(MLOCK_TIME, get_timer(&mach_timer) * MS_IN_SECOND);
}

void whist_munlock(void* addr) {
    static WhistTimer timer;
    start_timer(&timer);
    whist_lock_mutex(mlock_context.lock);
    if (mlock_context.malloc_to_region.contains(addr)) {
        MLockRegion region = mlock_context.malloc_to_region[addr];
        if (mlock_context.region_to_count.contains(region)) {
            mlock_context.region_to_count[region]--;
            if (mlock_context.region_to_count[region] == 0) {
                munlock(region.addr, region.size);
                LOG_INFO("Munlock'ed region %p size %zx", region.addr, region.size);
            }
        } else {
            LOG_ERROR("Couldn't find region with addr %p size %zx in map", region.addr,
                      region.size);
        }
    } else {
        LOG_ERROR("Tried to munlock an address %p that wasn't recorded", addr);
    }
    whist_unlock_mutex(mlock_context.lock);
    log_double_statistic(MUNLOCK_TIME, get_timer(&timer) * MS_IN_SECOND);
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
