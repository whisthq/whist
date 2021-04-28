#include "clipboard.h"
#include <fractal/core/fractal.h>

// These clipboard primitives are defined in {x11,win,mac}_clipboard.c
// CMake will only link one of those C files, depending on the OS

void unsafe_init_clipboard();
ClipboardData* unsafe_get_clipboard();
void unsafe_set_clipboard(ClipboardData* cb);
void unsafe_free_clipboard(ClipboardData* cb);
bool unsafe_has_clipboard_updated();
void unsafe_destroy_clipboard();

// A Mutex to ensure unsafe commands don't overlap
FractalMutex mutex;
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
    mutex = fractal_create_mutex();
    unsafe_init_clipboard();
}

bool should_preserve_local_clipboard() {
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

    fractal_lock_mutex(mutex);
    ClipboardData* cb = unsafe_get_clipboard();
    fractal_unlock_mutex(mutex);
    return cb;
}

void set_clipboard(ClipboardData* cb) {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return;
    }

    fractal_lock_mutex(mutex);
    unsafe_set_clipboard(cb);
    // clear out update from filling clipboard
    unsafe_has_clipboard_updated();
    fractal_unlock_mutex(mutex);
}

void free_clipboard(ClipboardData* cb) {
    unsafe_free_clipboard(cb);
}

bool has_clipboard_updated() {
    if (!mutex) {
        LOG_ERROR("init_clipboard not called yet!");
        return false;
    }

    if (fractal_try_lock_mutex(mutex) == 0) {
        bool has_clipboard_updated = unsafe_has_clipboard_updated();
        fractal_unlock_mutex(mutex);
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

    fractal_lock_mutex(mutex);
    unsafe_destroy_clipboard();
    fractal_unlock_mutex(mutex);

    fractal_destroy_mutex(mutex);
    mutex = NULL;
}
