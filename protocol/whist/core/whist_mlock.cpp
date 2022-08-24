#include "whist_mlock.h"
#include <map>
extern "C" {
#include <sys/mman.h>
#include <mach/vm_map.h>
#include <mach/vm_types.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <whist/logging/log_statistic.h>
}

#if OS_IS(OS_MACOS)
struct VMRegion {
    mach_vm_address_t addr;
    mach_vm_size_t size;

    bool operator<(const VMRegion& region) const {
        if (this->addr < region.addr) {
            return true;
        }
        if (this->addr > region.addr) {
            return false;
        }
        return this->size < region.size;
    }

    bool operator==(const VMRegion& region) const {
        return this->addr == region.addr && this->size == region.size;
    }
};

// map addr -> VMRegion*
// map VMRegion (or VMRegion*) -> number of mallocs
struct MlockContext {
    WhistMutex lock;
    std::map<void*, VMRegion> malloc_to_region;
    std::map<VMRegion, int> region_to_count;
};

static MlockContext mlock_context;

void init_whist_mlock() {
    mlock_context.lock = whist_create_mutex();
}

void whist_mlock(void* addr, size_t size) {
    static WhistTimer mach_timer;
    // call mach_vm_region on addr
    task_t t = mach_task_self();
    mach_vm_address_t mach_addr = (mach_vm_address_t) addr;
    kern_return_t rc;
    vm_region_basic_info_data_t info;
    mach_vm_size_t mach_size;
    mach_port_t obj_name;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    TIME_RUN(rc = mach_vm_region(t, &mach_addr, &mach_size, VM_REGION_BASIC_INFO, (vm_region_info_t)&info, &count, &obj_name), MACH_VM_REGION_TIME, mach_timer);
    if (rc) {
        LOG_INFO("mach_vm_region_failed, %s", mach_error_string(rc));
    } else {
        // LOG_INFO("mach_vm_region addr %p size %llx", (void*)mach_addr, mach_size);
        start_timer(&mach_timer);
        whist_lock_mutex(mlock_context.lock);
        VMRegion region = { .addr = mach_addr, .size = mach_size };
        mlock_context.malloc_to_region[(void*) addr] = region;
        if (mlock_context.region_to_count.contains(region)) {
            mlock_context.region_to_count[region]++;
        } else {
            mlock_context.region_to_count[region] = 1;
            mlock((void*) region.addr, (size_t) region.size);
            LOG_INFO("Mlocking region %p size %llx", (void*) region.addr, region.size);
        }
        whist_unlock_mutex(mlock_context.lock);
        log_double_statistic(MLOCK_TIME, get_timer(&mach_timer) * MS_IN_SECOND);
    }
}

void whist_munlock(void* addr) {
    static WhistTimer timer;
    start_timer(&timer);
    whist_lock_mutex(mlock_context.lock);
    if (mlock_context.malloc_to_region.contains(addr)) {
        VMRegion region = mlock_context.malloc_to_region[addr];
        if (mlock_context.region_to_count.contains(region)) {
            mlock_context.region_to_count[region]--;
            if (mlock_context.region_to_count[region] == 0) {
                munlock((void*) region.addr, (size_t) region.size);
                LOG_INFO("Munlock'ed region %p size %llx", (void*) region.addr, region.size);
            }
        } else {
            LOG_ERROR("Couldn't find region with addr %p size %llx in map", (void*) region.addr, region.size);
        }
    } else {
        LOG_ERROR("Tried to munlock an address %p that wasn't recorded", addr);
    }
    whist_unlock_mutex(mlock_context.lock);
    log_double_statistic(MUNLOCK_TIME, get_timer(&timer) * MS_IN_SECOND);
}
#else
void init_whist_mlock() {
    return;
}
void whist_mlock(void* addr, size_t size) {
    return;
}
void whist_munlock(void* addr) {
    return;
}
#endif
