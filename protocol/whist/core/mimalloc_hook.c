#include <whist/core/whist.h>
#include <malloc/malloc.h>
#include <mimalloc.h>

static size_t page_size;
static malloc_zone_t* original_zone;
static size_t (*system_size)(malloc_zone_t* zone, const void* ptr);
static void* (*system_malloc)(malloc_zone_t* zone, size_t size);
static void (*system_free)(malloc_zone_t* zone, void* ptr);
static void (*system_free_definite_size)(malloc_zone_t* zone, void* ptr, size_t size);
static void* (*system_calloc)(malloc_zone_t* zone, size_t num_items, size_t size);
static void* (*system_valloc)(malloc_zone_t* zone, size_t size);
static void (*system_free)(malloc_zone_t* zone, void* ptr);
static void* (*system_realloc)(malloc_zone_t* zone, void* ptr, size_t size);
static void (*system_destroy)(malloc_zone_t* zone);

/* Optional batch callbacks; these may be NULL */
static unsigned (*system_batch_malloc)(malloc_zone_t* zone, size_t size, void** results,
                                       unsigned num_requested);
static void (*system_batch_free)(malloc_zone_t* zone, void** to_be_freed, unsigned num_to_be_freed);

/* aligned memory allocation. The callback may be NULL. Present in version >= 5. */
static void* (*system_memalign)(malloc_zone_t* zone, size_t alignment, size_t size);

/* free a pointer known to be in zone and known to have the given size. The callback may be NULL.
 * Present in version >= 6.*/
static void (*system_free_definite_size)(malloc_zone_t* zone, void* ptr, size_t size);

/* Empty out caches in the face of memory pressure. The callback may be NULL. Present in version
 * >= 8. */
static size_t (*system_pressure_relief)(malloc_zone_t* zone, size_t goal);

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
    // todo: ignore for now?
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

#define SET_SYSTEM(fun) system_##fun = original_zone->fun
#define HOOK_WHIST(fun) original_zone->fun = whist_##fun

void init_whist_malloc_hook() {
    LOG_INFO("Initializing custom malloc");
    // mlock_context.lock = whist_create_mutex();
    page_size = sysconf(_SC_PAGESIZE);
    original_zone = malloc_default_zone();
    SET_SYSTEM(size);
    SET_SYSTEM(malloc);
    SET_SYSTEM(calloc);
    SET_SYSTEM(valloc);
    SET_SYSTEM(free);
    SET_SYSTEM(realloc);
    SET_SYSTEM(destroy);
    SET_SYSTEM(batch_malloc);
    SET_SYSTEM(batch_free);
    SET_SYSTEM(memalign);
    SET_SYSTEM(free_definite_size);
    SET_SYSTEM(pressure_relief);

    HOOK_WHIST(size);
    HOOK_WHIST(malloc);
    HOOK_WHIST(calloc);
    HOOK_WHIST(valloc);
    HOOK_WHIST(free);
    HOOK_WHIST(realloc);
    HOOK_WHIST(destroy);
    HOOK_WHIST(batch_malloc);
    HOOK_WHIST(batch_free);
    HOOK_WHIST(memalign);
    HOOK_WHIST(free_definite_size);
    HOOK_WHIST(pressure_relief);
}
