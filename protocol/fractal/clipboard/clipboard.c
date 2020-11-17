#include "clipboard.h"
#include "../core/fractal.h"

SDL_mutex* mutex;

void init_clipboard() {
    mutex = SDL_CreateMutex();
    unsafe_init_clipboard();
}

ClipboardData* get_clipboard() {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return NULL;
    }

    if (SDL_LockMutex(mutex) == 0) {
        ClipboardData* cb = unsafe_get_clipboard();
        SDL_UnlockMutex(mutex);
        return cb;
    } else {
        LOG_WARNING("get_clipboard SDL_LockMutex failed");
        return NULL;
    }
}

void set_clipboard(ClipboardData* cb) {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return;
    }

    if (SDL_LockMutex(mutex) == 0) {
        unsafe_set_clipboard(cb);
        SDL_UnlockMutex(mutex);
    } else {
        LOG_WARNING("set_clipboard SDL_LockMutex failed");
    }
}

bool has_clipboard_updated() {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return false;
    }

    if (SDL_TryLockMutex(mutex) == 0) {
        bool has_clipboard_updated = unsafe_has_clipboard_updated();
        SDL_UnlockMutex(mutex);
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

    if (SDL_LockMutex(mutex) == 0) {
        unsafe_destroy_clipboard();
        SDL_UnlockMutex(mutex);
    } else {
        LOG_WARNING("destroy_clipboard SDL_LockMutex failed");
    }
}
