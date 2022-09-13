#include <whist/core/whist.h>
#include <malloc/malloc.h>
#include <mimalloc/mimalloc.h>
#include <iostream>
extern "C" {
#include "whist_memory.h"
#include <mach/mach_vm.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach/vm_statistics.h>
#include <sys/mman.h>
}

// malloc_zone override
// save the members just in case
static size_t page_size;
static malloc_zone_t* original_zone;
static size_t (*system_size)(malloc_zone_t* zone, const void* ptr);
static void* (*system_malloc)(malloc_zone_t* zone, size_t size);
static void* (*system_calloc)(malloc_zone_t* zone, size_t num_items, size_t size);
static void* (*system_valloc)(malloc_zone_t* zone, size_t size);
static void (*system_free)(malloc_zone_t* zone, void* ptr);
static void* (*system_realloc)(malloc_zone_t* zone, void* ptr, size_t size);
static void (*system_destroy)(malloc_zone_t* zone);
static unsigned (*system_batch_malloc)(malloc_zone_t* zone, size_t size, void** results,
                                       unsigned num_requested);
static void (*system_batch_free)(malloc_zone_t* zone, void** to_be_freed, unsigned num_to_be_freed);
static void* (*system_memalign)(malloc_zone_t* zone, size_t alignment, size_t size);
static void (*system_free_definite_size)(malloc_zone_t* zone, void* ptr, size_t size);
static size_t (*system_pressure_relief)(malloc_zone_t* zone, size_t goal);

// Our malloc_zone members

static size_t whist_size(malloc_zone_t* zone, const void* p) {
    UNUSED(zone);
    if (!mi_is_in_heap_region(p)) {
        return 0;
    }  // not our pointer, bail out
    return mi_usable_size(p);
}

static void* whist_malloc(malloc_zone_t* zone, size_t size) {
    UNUSED(zone);
    return mi_malloc(size);
}

static void* whist_calloc(malloc_zone_t* zone, size_t count, size_t size) {
    UNUSED(zone);
    return mi_calloc(count, size);
}

static void* whist_valloc(malloc_zone_t* zone, size_t size) {
    UNUSED(zone);
    return mi_malloc_aligned(size, page_size);
}

static void whist_free(malloc_zone_t* zone, void* p) {
    UNUSED(zone);
    mi_cfree(p);
}

static void* whist_realloc(malloc_zone_t* zone, void* p, size_t newsize) {
    UNUSED(zone);
    return mi_realloc(p, newsize);
}

static void* whist_memalign(malloc_zone_t* zone, size_t alignment, size_t size) {
    UNUSED(zone);
    return mi_malloc_aligned(size, alignment);
}

static void whist_destroy(malloc_zone_t* zone) {
    UNUSED(zone);
}

static unsigned whist_batch_malloc(malloc_zone_t* zone, size_t size, void** ps, unsigned count) {
    size_t i;
    for (i = 0; i < count; i++) {
        ps[i] = whist_malloc(zone, size);
        if (ps[i] == NULL) break;
    }
    return i;
}

static void whist_batch_free(malloc_zone_t* zone, void** ps, unsigned count) {
    for (size_t i = 0; i < count; i++) {
        whist_free(zone, ps[i]);
        ps[i] = NULL;
    }
}

static size_t whist_pressure_relief(malloc_zone_t* zone, size_t size) {
    UNUSED(zone);
    UNUSED(size);
    mi_collect(false);
    return 0;
}

static void whist_free_definite_size(malloc_zone_t* zone, void* p, size_t size) {
    UNUSED(size);
    whist_free(zone, p);
}

static boolean_t whist_claimed_address(malloc_zone_t* zone, void* p) {
    UNUSED(zone);
    return mi_is_in_heap_region(p);
}

// Our introspection members (copied from mimalloc). Comments are copied also from mimalloc.
static kern_return_t intro_enumerator(task_t task, void* p, unsigned type_mask,
                                      vm_address_t zone_address, memory_reader_t reader,
                                      vm_range_recorder_t recorder) {
    // todo: enumerate all memory
    UNUSED(task);
    UNUSED(p);
    UNUSED(type_mask);
    UNUSED(zone_address);
    UNUSED(reader);
    UNUSED(recorder);
    return KERN_SUCCESS;
}

static size_t intro_good_size(malloc_zone_t* zone, size_t size) {
    UNUSED(zone);
    return mi_good_size(size);
}

static boolean_t intro_check(malloc_zone_t* zone) {
    UNUSED(zone);
    return true;
}

static void intro_print(malloc_zone_t* zone, boolean_t verbose) {
    UNUSED(zone);
    UNUSED(verbose);
    mi_stats_print(NULL);
}

static void intro_log(malloc_zone_t* zone, void* p) {
    UNUSED(zone);
    UNUSED(p);
    // todo?
}

static void intro_force_lock(malloc_zone_t* zone) {
    UNUSED(zone);
    // todo?
}

static void intro_force_unlock(malloc_zone_t* zone) {
    UNUSED(zone);
    // todo?
}

static void intro_statistics(malloc_zone_t* zone, malloc_statistics_t* stats) {
    UNUSED(zone);
    // todo...
    stats->blocks_in_use = 0;
    stats->size_in_use = 0;
    stats->max_size_in_use = 0;
    stats->size_allocated = 0;
}

static boolean_t intro_zone_locked(malloc_zone_t* zone) {
    UNUSED(zone);
    return false;
}

static malloc_introspection_t whist_introspect = {
    .enumerator = &intro_enumerator,
    .good_size = &intro_good_size,
    .check = &intro_check,
    .print = &intro_print,
    .log = &intro_log,
    .force_lock = &intro_force_lock,
    .force_unlock = &intro_force_unlock,
    .statistics = &intro_statistics,
    .zone_locked = &intro_zone_locked,
};

static malloc_zone_t whist_malloc_zone = {
    .size = &whist_size,
    .malloc = &whist_malloc,
    .calloc = &whist_calloc,
    .valloc = &whist_valloc,
    .free = &whist_free,
    .realloc = &whist_realloc,
    .destroy = &whist_destroy,
    .zone_name = "whist_mimalloc",
    .batch_malloc = &whist_batch_malloc,
    .batch_free = &whist_batch_free,
    .introspect = &whist_introspect,
    .version = 10,
    .memalign = &whist_memalign,
    .free_definite_size = &whist_free_definite_size,
    .pressure_relief = &whist_pressure_relief,
    .claimed_address = &whist_claimed_address,
};

static inline malloc_zone_t* whist_get_default_zone(void) {
    // The first returned zone is the real default
    malloc_zone_t** zones = NULL;
    unsigned count = 0;
    kern_return_t ret = malloc_get_all_zones(0, NULL, (vm_address_t**)&zones, &count);
    if (ret == KERN_SUCCESS && count > 0) {
        return zones[0];
    } else {
        // fallback
        return malloc_default_zone();
    }
}

// Helper function for locking statics

// documentation notes: the source code for mach/etc are in https://github.com/apple/darwin-xnu/.
static void mlock_statics() {
    // TODO: better way of mlock'ing static segments
    static mach_vm_address_t max_addr = (mach_vm_address_t)0x20000000000;
    task_t t = mach_task_self();

    mach_vm_address_t addr = 1;
    kern_return_t rc;
    vm_region_basic_info_data_t info;
    mach_vm_address_t prev_addr = 0;
    mach_vm_size_t size, prev_size;

    mach_port_t obj_name;
    mach_msg_type_number_t count;

    int done = 0;
    WhistTimer mlock_timer;
    static unsigned long long total_mlocked_size = 0;
    while (!done) {
        count = VM_REGION_BASIC_INFO_COUNT_64;

        rc = mach_vm_region(t, &addr, &size, VM_REGION_BASIC_INFO, (vm_region_info_t)&info, &count,
                            &obj_name);
        if (rc) {
            // indicates that we've given an invalid address.
            LOG_INFO("mach_vm_region_failed, %s", mach_error_string(rc));
            max_addr = addr;
            done = 1;
        } else {
            if ((prev_addr == 0 || addr >= prev_addr + prev_size) && addr <= max_addr) {
                // LOG_INFO("mlock'ed %p size %llx", (void*)addr, size);
                // update region list data and mlock
                mlock((void*)addr, size);
                prev_addr = addr;
                prev_size = size;
                // repeatedly call mach_vm_region
                addr = prev_addr + prev_size;
                // means address space has wrapped around
                if (prev_size == 0 || addr != prev_addr + prev_size || addr >= max_addr) {
                    // finished because we've wrapped around
                    done = 1;
                }
            } else {
                done = 1;
            }
        }
    }
}

extern malloc_zone_t* malloc_default_purgeable_zone(void) __attribute__((weak_import));

void init_whist_malloc_hook() {
    LOG_INFO("Initializing custom malloc");
    // mlock_context.lock = whist_create_mutex();
    page_size = sysconf(_SC_PAGESIZE);
    malloc_zone_t* purgeable_zone = NULL;

    // force the purgeable zone to exist to avoid strange bugs
    if (malloc_default_purgeable_zone) {
        purgeable_zone = malloc_default_purgeable_zone();
    }

    // Register our zone.
    // thomcc: I think this is still needed to put us in the zone list.
    malloc_zone_register(&whist_malloc_zone);
    // Unregister the default zone, this makes our zone the new default
    // as that was the last registered.
    malloc_zone_t* default_zone = whist_get_default_zone();
    // thomcc: Unsure if the next test is *always* false or just false in the
    // cases I've tried. I'm also unsure if the code inside is needed. at all
    if (default_zone != &whist_malloc_zone) {
        malloc_zone_unregister(default_zone);

        // Reregister the default zone so free and realloc in that zone keep working.
        malloc_zone_register(default_zone);
    }

    // Unregister, and re-register the purgeable_zone to avoid bugs if it occurs
    // earlier than the default zone.
    if (purgeable_zone != NULL) {
        malloc_zone_unregister(purgeable_zone);
        malloc_zone_register(purgeable_zone);
    }
    LOG_INFO("mlocking statics");
    mlock_statics();
}
