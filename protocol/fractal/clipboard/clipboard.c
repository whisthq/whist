#include "clipboard.h"
#include <fractal/core/fractal.h>

SDL_mutex* mutex;

void init_clipboard() {
    if (mutex) {
        LOG_ERROR("Clipboard is being initialized twice!");
        return;
    }
    mutex = safe_SDL_CreateMutex();
    unsafe_init_clipboard();
}

ClipboardData* get_clipboard() {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return NULL;
    }

    safe_SDL_LockMutex(mutex);
    ClipboardData* cb = unsafe_get_clipboard();
    safe_SDL_UnlockMutex(mutex);
    return cb;
}

void set_clipboard(ClipboardData* cb) {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return;
    }

    safe_SDL_LockMutex(mutex);
    unsafe_set_clipboard(cb);
    // clear out update from filling clipboard
    unsafe_has_clipboard_updated();
    safe_SDL_UnlockMutex(mutex);
}

bool has_clipboard_updated() {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return false;
    }

    if (SDL_TryLockMutex(mutex) == 0) {
        bool has_clipboard_updated = unsafe_has_clipboard_updated();
        safe_SDL_UnlockMutex(mutex);
        return has_clipboard_updated;
    } else {
        // LOG_WARNING("Could not check has_clipboard_updated, clipboard is busy!");
        return false;
    }
}

void destroy_clipboard() {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return;
    }

    safe_SDL_LockMutex(mutex);
    unsafe_destroy_clipboard();
    safe_SDL_UnlockMutex(mutex);

    SDL_DestroyMutex(mutex);
    mutex = NULL;
}
