#include <whist/core/whist.h>
#include <malloc/malloc.h>
#include <mimalloc/mimalloc.h>
#include <iostream>
extern "C" {
#include "whist_memory.h"
#include <mach/mach_vm.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <sys/mman.h>
#include <string.h>
}

// malloc_zone override
// save the members just in case
static size_t page_size;

// Our malloc_zone members; these are copied from mimalloc's override file
// https://github.com/microsoft/mimalloc/blob/master/src/alloc-override-osx.c

static size_t new_size(malloc_zone_t* zone, const void* p) {
    UNUSED(zone);
    if (!mi_is_in_heap_region(p)) {
        return 0;
    }  // not our pointer, bail out
    return mi_usable_size(p);
}

static void* new_malloc(malloc_zone_t* zone, size_t size) {
    UNUSED(zone);
    return mi_malloc(size);
}

static void* new_calloc(malloc_zone_t* zone, size_t count, size_t size) {
    UNUSED(zone);
    return mi_calloc(count, size);
}

static void* new_valloc(malloc_zone_t* zone, size_t size) {
    UNUSED(zone);
    return mi_malloc_aligned(size, page_size);
}

static void new_free(malloc_zone_t* zone, void* p) {
    UNUSED(zone);
    mi_cfree(p);
}

static void* new_realloc(malloc_zone_t* zone, void* p, size_t newsize) {
    UNUSED(zone);
    return mi_realloc(p, newsize);
}

static void* new_memalign(malloc_zone_t* zone, size_t alignment, size_t size) {
    UNUSED(zone);
    return mi_malloc_aligned(size, alignment);
}

static void new_destroy(malloc_zone_t* zone) { UNUSED(zone); }

static unsigned new_batch_malloc(malloc_zone_t* zone, size_t size, void** ps, unsigned count) {
    size_t i;
    for (i = 0; i < count; i++) {
        ps[i] = new_malloc(zone, size);
        if (ps[i] == NULL) break;
    }
    return i;
}

static void new_batch_free(malloc_zone_t* zone, void** ps, unsigned count) {
    for (size_t i = 0; i < count; i++) {
        new_free(zone, ps[i]);
        ps[i] = NULL;
    }
}

static size_t new_pressure_relief(malloc_zone_t* zone, size_t size) {
    UNUSED(zone);
    UNUSED(size);
    mi_collect(false);
    return 0;
}

static void new_free_definite_size(malloc_zone_t* zone, void* p, size_t size) {
    UNUSED(size);
    new_free(zone, p);
}

static boolean_t new_claimed_address(malloc_zone_t* zone, void* p) {
    UNUSED(zone);
    return mi_is_in_heap_region(p);
}

// Our introspection members (copied from mimalloc). Comments are copied also from mimalloc.
static kern_return_t intro_enumerator(task_t task, void* p, unsigned type_mask,
                                      vm_address_t zone_address, memory_reader_t reader,
                                      vm_range_recorder_t recorder) {
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

static void intro_force_lock(malloc_zone_t* zone) { UNUSED(zone); }

static void intro_force_unlock(malloc_zone_t* zone) { UNUSED(zone); }

static void intro_statistics(malloc_zone_t* zone, malloc_statistics_t* stats) {
    UNUSED(zone);
    stats->blocks_in_use = 0;
    stats->size_in_use = 0;
    stats->max_size_in_use = 0;
    stats->size_allocated = 0;
}

static boolean_t intro_zone_locked(malloc_zone_t* zone) {
    UNUSED(zone);
    return false;
}

static malloc_introspection_t new_introspect = {
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

static malloc_zone_t new_malloc_zone = {
    .size = &new_size,
    .malloc = &new_malloc,
    .calloc = &new_calloc,
    .valloc = &new_valloc,
    .free = &new_free,
    .realloc = &new_realloc,
    .destroy = &new_destroy,
    .zone_name = "new_mimalloc",
    .batch_malloc = &new_batch_malloc,
    .batch_free = &new_batch_free,
    .introspect = &new_introspect,
    .version = 10,
    .memalign = &new_memalign,
    .free_definite_size = &new_free_definite_size,
    .pressure_relief = &new_pressure_relief,
    .claimed_address = &new_claimed_address,
};

static inline malloc_zone_t* new_get_default_zone(void) {
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
// mach_vm_region(vm_map_t map,
// mach_vm_address_t *address,
// mach_vm_size_t *size,
// vm_region_flavor_t flavor,
// vm_region_info_t info,
// mach_msg_type_number_t *cnt,
// mach_port_t *object_name)
// this displays information about the VM Region for virtual memory address address for task map
// there are two "flavors", but we only need the basic one (and in fact the extended flavor takes a
// lot longer to run) output is: - address: starting address for the region
// - size: size of the region
// - info : returned info struct (we don't use this at all)
// - count: the # of entries of structs corresponding to the flavor (we don't use this either)
// - obj_name: some other output, also unused
static void mlock_statics() {
    // TODO: better way of mlock'ing static segments
    static mach_vm_address_t max_addr = (mach_vm_address_t)0x20000000000;
    task_t t = mach_task_self();

    mach_vm_address_t addr = 1;
    kern_return_t rc;  // return value for mach_vm_region
    vm_region_basic_info_data_t info;
    mach_vm_address_t prev_addr = 0;
    mach_vm_size_t size, prev_size;

    mach_port_t obj_name;
    mach_msg_type_number_t count;

    int done = 0;
    // starting from address 1, use mach_vm_region to obtain the address and size of the next region
    // repeat until we've mlock'ed all static memory
    while (!done) {
        count = VM_REGION_BASIC_INFO_COUNT_64;

        rc = mach_vm_region(t, &addr, &size, VM_REGION_BASIC_INFO, (vm_region_info_t)&info, &count,
                            &obj_name);
        if (rc) {
            // indicates that we've given an invalid address.
            LOG_ERROR("mach_vm_region_failed, %s", mach_error_string(rc));
            max_addr = addr;
            done = 1;
        } else {
            // check for overflow; all VM regions are distinct, so this should not error out until
            // we have reached the end of the memory we want to mlock
            if ((prev_addr == 0 || addr >= prev_addr + prev_size) && addr <= max_addr) {
                // LOG_INFO("mlock'ed %p size %llx", (void*)addr, size);
                // update region list data and mlock
                int ret = mlock((void*)addr, size);
                if (ret == -1) {
                    LOG_MESSAGE_RATE_LIMITED(0.1, 1, ERROR, "mlock failed with error %s",
                                             strerror(errno));
                }
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
    LOG_INFO("mlocking statics");
    mlock_statics();
    LOG_INFO("Initializing custom malloc");
    // This zone replacement is copied from mimalloc; I've preserved the comments in case we want to
    // try to resolve them in the future
    page_size = sysconf(_SC_PAGESIZE);
    malloc_zone_t* purgeable_zone = NULL;

    // force the purgeable zone to exist to avoid strange bugs
    if (malloc_default_purgeable_zone) {
        purgeable_zone = malloc_default_purgeable_zone();
    }

    // Register our zone.
    // thomcc: I think this is still needed to put us in the zone list.
    malloc_zone_register(&new_malloc_zone);
    // Unregister the default zone, this makes our zone the new default
    // as that was the last registered.
    malloc_zone_t* default_zone = new_get_default_zone();
    // thomcc: Unsure if the next test is *always* false or just false in the
    // cases I've tried. I'm also unsure if the code inside is needed. at all
    if (default_zone != &new_malloc_zone) {
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
}
