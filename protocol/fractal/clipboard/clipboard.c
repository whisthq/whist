#include "clipboard.h"
#include <fractal/core/fractal.h>

SDL_mutex* mutex;
bool preserve_local_clipboard = false;

void init_clipboard(bool is_client) {
    /*
        Intialize clipboard

        Arguments:
            is_client (bool): true if client, false if server.
    */

    if (mutex) {
        LOG_ERROR("Clipboard is being initialized twice!");
        return;
    }
    // If the caller is the client, then the clipboard state
    //     should be preserved for the shared clipboard state.
    preserve_local_clipboard = is_client;
    mutex = safe_SDL_CreateMutex();
    unsafe_init_clipboard();
}

bool is_local_clipboard_preserved() {
    /*
        Returns whether the local clipboard should be preserved.
        The client should preserve its local clipboard by sharing
        the current clipboard state with the server. The server
        should not preserve its local clipboard state.

        Returns:
            true if client, false if server
    */

    return preserve_local_clipboard;
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
