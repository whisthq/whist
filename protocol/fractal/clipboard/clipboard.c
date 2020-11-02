#include "clipboard.h"
#include "../core/fractal.h"

SDL_mutex* mutex;

void initClipboard() {
    mutex = SDL_CreateMutex();
    unsafe_initClipboard();
}

ClipboardData* GetClipboard() {
    if (!mutex) {
        LOG_ERROR("initClipboard not called yet!");
        return NULL;
    }

    if (SDL_LockMutex(mutex) == 0) {
        ClipboardData* cb = unsafe_GetClipboard();
        SDL_UnlockMutex(mutex);
        return cb;
    } else {
        LOG_WARNING("GetClipboard SDL_LockMutex failed");
        return NULL;
    }
}

void SetClipboard(ClipboardData* cb) {
    if (!mutex) {
        LOG_ERROR("initClipboard not called yet!");
        return;
    }

    if (SDL_LockMutex(mutex) == 0) {
        unsafe_SetClipboard(cb);
        SDL_UnlockMutex(mutex);
    } else {
        LOG_WARNING("SetClipboard SDL_LockMutex failed");
    }
}

bool hasClipboardUpdated() {
    if (!mutex) {
        LOG_ERROR("initClipboard not called yet!");
        return false;
    }

    if (SDL_TryLockMutex(mutex) == 0) {
        bool has_clipboard_updated = unsafe_hasClipboardUpdated();
        SDL_UnlockMutex(mutex);
        return has_clipboard_updated;
    } else {
        // LOG_WARNING("Could not check hasClipboardUpdated, clipboard is busy!");
        return false;
    }
}

void DestroyClipboard() {
    if (!mutex) {
        LOG_ERROR("initClipboard not called yet!");
        return;
    }

    if (SDL_LockMutex(mutex) == 0) {
        unsafe_DestroyClipboard();
        SDL_UnlockMutex(mutex);
    } else {
        LOG_WARNING("DestroyClipboard SDL_LockMutex failed");
    }
}
