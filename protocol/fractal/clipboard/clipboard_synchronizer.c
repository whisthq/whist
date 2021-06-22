/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clipboard_synchronizer.c
 * @brief This file contains code meant, to be used by both the server and client
 *        to synchronize clipboard contents, both setting and gettings. Client
 *        and server clipboard activities are parallel, and therefore the
 *        synchronizer abstracts out and allows threading of clipboard actions.
============================
Usage
============================

initClipboardSynchronizer();

ClipboardData server_clipboard;

// Will set the client clipboard to that data
ClipboardSynchronizerSetClipboard(&server_clipboard);

// Will likely return true because it's waiting on server_clipboard to be set
LOG_INFO("Is Synchronizing? %d", isClipboardSynchronizing());

// Wait for clipboard to be done synchronizing
while(isClipboardSynchronizing());

ClipboardData* client_clipboard = ClipboardSynchronizerGetNewClipboard();

if (client_clipboard) {
  // We have a new clipboard, this should be sent to the server
  Send(client_clipboard);
} else {
  // There is no new clipboard
}

destroyClipboardSynchronizer();
*/

/*
============================
Includes
============================
*/

#include <stdio.h>

#include <fractal/core/fractal.h>
#include "clipboard.h"

/*
============================
Defines
============================
*/

#define MS_IN_SECOND 1000

volatile bool updating_set_clipboard;  // set to true when SetClipboard() needs to be called
volatile bool updating_get_clipboard;  // set to true when GetClipboard() needs to be called
volatile bool updating_clipboard;  // acts as a mutex to prevent clipboard activity from overlapping
// volatile bool pending_update_clipboard;  // set to true when GetClipboard() has finished running
volatile bool abort_active_clipboard_action; // set to true when a new get/set aborts the current one
clock last_clipboard_update;
FractalSemaphore clipboard_semaphore;  // used to signal clipboard_synchronizer_thread to continue
ClipboardData* clipboard;
int clipboard_written_bytes; // number of bytes of clipboard already transmitted
FractalThread clipboard_synchronizer_thread;
static bool connected = false;

bool pending_clipboard_push;

/*
============================
Private Functions
============================
*/

int update_clipboard(void* opaque);

/*
============================
Public Function Implementations
============================
*/

bool is_clipboard_synchronizing() {
    /*
        Check if the clipboard is in the midst of being updated

        Returns:
            (bool): True if the clipboard is currently busy being updated.
                This will be true for a some period of time after
                updateSetClipboard.
    */

    if (!connected) {
        LOG_ERROR("Tried to is_clipboard_synchronizing, but the clipboard is not initialized");
        return true;
    }
    return updating_clipboard;
}

void init_clipboard_synchronizer(bool is_client) {
    /*
        Initialize the clipboard and the synchronizer thread

        Arguments:
            is_client (bool): whether the caller is the client (true) or the server (false)
    */

    LOG_INFO("Initializing clipboard");

    if (connected) {
        LOG_ERROR("Tried to init_clipboard, but the clipboard is already initialized");
        return;
    }

    init_clipboard(is_client);

    connected = true;

    pending_clipboard_push = false;
    updating_clipboard = false;
    // pending_update_clipboard = false;
    start_timer((clock*)&last_clipboard_update);
    clipboard_semaphore = fractal_create_semaphore(0);

    clipboard_synchronizer_thread =
        fractal_create_thread(update_clipboard, "update_clipboard", NULL);

    // pending_update_clipboard = true;
}

void destroy_clipboard_synchronizer() {
    /*
        Clean up and destroy the clipboard synchronizer
    */

    LOG_INFO("Destroying clipboard");

    if (!connected) {
        LOG_ERROR("Tried to destroy_clipboard, but the clipboard is already destroyed");
        return;
    }

    connected = false;

    if (updating_clipboard) {
        LOG_FATAL("Trying to destroy clipboard while the clipboard is being updated");
    }

    destroy_clipboard();

    fractal_post_semaphore(clipboard_semaphore);
}

void clipboard_synchronizer_abort_active_clipboard_action() {
    /*
        Abort an active synchronizer get or set by deallocating the active clipboard
        and resetting all update booleans to false.

        NOTE: this must be called with the (TODO) update mutex held
    */

    if (clipboard) {
        // TODO: fix the logic around selecting which one of these to do
        free_clipboard(clipboard);
        deallocate_region(clipboard);
        clipboard = NULL;
    }
    updating_clipboard = false;
    updating_set_clipboard = false;
    updating_get_clipboard = false;
    pending_clipboard_push = false;
}

bool clipboard_synchronizer_set_clipboard(ClipboardData* cb) {
    /*
        When called, signal that the clipboard can be set to the given contents

        Arguments:
            cb (ClipboardData*): pointer to the clipboard to load

        Returns:
            updatable (bool): whether the clipboard can be set right now
    */

    if (!connected) {
        LOG_ERROR("Tried to set_clipboard, but the clipboard is not initialized");
        return false;
    }

    // TODO: updating_clipboard needs to be mutex-protected
    // If the clipboard is currently being updated and the received chunk is a
    //     START chunk, then abort the active action
    if (updating_clipboard && cb->chunk_type == CLIPBOARD_START) {
        // LOG_INFO("Tried to SetClipboard, but clipboard is updating");
        abort_active_clipboard_action = true;
        clipboard_synchronizer_abort_active_clipboard_action();
        // return false;
    }

    // TODO: make these variables atomic?
    updating_clipboard = true;
    updating_set_clipboard = true;
    updating_get_clipboard = false;

    // clipboard->chunk_type is unused, but cb->chunk_type is important
    if (cb->chunk_type == CLIPBOARD_START) {
        clipboard = allocate_region(sizeof(ClipboardData) + cb->size);
        memcpy(clipboard, cb, sizeof(ClipboardData) + cb->size);
    } else {
        // if not the first clipboard chunk, then add onto the end of the buffer
        //     and update clipboard size
        clipboard = realloc_region(clipboard, sizeof(ClipboardData) + clipboard->size + cb->size);
        memcpy(clipboard->data + clipboard->size, cb->data, cb->size);
        clipboard->size += cb->size;

        if (cb->chunk_type == CLIPBOARD_FINAL) {
            // ready to set OS clipboard
            fractal_post_semaphore(clipboard_semaphore);
        }
    }

    return true;
}

// TODO: change this name to get_next_clipboard_chunk
// TODO: this should be called in a loop of `while not NULL`
ClipboardData* clipboard_synchronizer_get_new_clipboard() {
    /*
        When called, return the current clipboard if a new clipboard activity
        has registered.

        Returns:
            cb (ClipboardData*): pointer to the current clipboard
    */

    if (!connected) {
        LOG_ERROR("Tried to get_new_clipboard, but the clipboard is not initialized");
        return NULL;
    }

    // UPDATE: removed pending_update_clipboard because we abort on new updates now

    // If the clipboard has updated since we last checked, or a previous
    // clipboard update is still pending, then we try to update the clipboard
    // if (has_clipboard_updated() || pending_update_clipboard) {
    if (has_clipboard_updated()) {
        if (updating_clipboard) {
            // // Clipboard is busy, to set pending update clipboard to true to
            // // make sure we keep checking the clipboard state
            // pending_update_clipboard = true;
            abort_active_clipboard_action = true;
            clipboard_synchronizer_abort_active_clipboard_action();
        }
        // } else {
        LOG_INFO("Pushing update to clipboard");
        // Clipboard is no longer pending
        // pending_update_clipboard = false;
        // TODO: add mutex for these variables, or somehow just use one and make it atomic
        updating_clipboard = true;
        updating_set_clipboard = false;
        updating_get_clipboard = true;
        fractal_post_semaphore(clipboard_semaphore);

        return NULL;
    }

    // TODO: use pending_clipboard_push and `clipboard` to return the appropriate chunk of clipboard
    //     set clipboard to NULL when finished and use this as a condition
    if (pending_clipboard_push) {
        int chunk_size = min(CHUNK_SIZE, clipboard->size - clipboard_written_bytes);
        ClipboardData* cb_chunk = allocate_region(sizeof(ClipboardData) + chunk_size);
        cb_chunk->size = chunk_size;
        cb_chunk->type = clipboard->type;
        if (clipboard_written_bytes == 0) {
            cb_chunk->chunk_type = CLIPBOARD_START;
        } else if (clipboard_written_bytes == clipboard->size) {
            cb_chunk->chunk_type = CLIPBOARD_FINAL;
        } else {
            cb_chunk->chunk_type = CLIPBOARD_MIDDLE;
        }

        // for FINAL, this will just "copy" 0 bytes
        memcpy(cb_chunk->data, clipboard->data + clipboard_written_bytes, chunk_size);
        if (clipboard_written_bytes == clipboard->size) {
            pending_clipboard_push = false;
            // Free clipboard
            free_clipboard(clipboard);
        }

        // update written bytes
        clipboard_written_bytes += chunk_size;

        // return clipboard;
        return cb_chunk;
    }

    // We don't want to call `has_clipboard_updated` if the clipboard is currently updating
    //    because each call to `has_clipboard_updated` resets the "updated check" and we only
    //    want to check for an update if we're ready to process it.
    // if (updating_clipboard) {
    //     return NULL;
    // }

    return NULL;
}

int update_clipboard(void* opaque) {
    /*
        Thread to get and set clipboard as signals are received.
    */

    UNUSED(opaque);

    while (connected) {
        fractal_wait_semaphore(clipboard_semaphore);

        if (!connected) {
            break;
        }

        if (updating_set_clipboard) {
            LOG_INFO("Trying to set clipboard!");

            set_clipboard(clipboard);
            updating_set_clipboard = false;
            deallocate_region(clipboard);
            clipboard = NULL;
        } else if (updating_get_clipboard) {
            LOG_INFO("Trying to get clipboard!");

            clipboard = get_clipboard();
            pending_clipboard_push = true;
            updating_get_clipboard = false;
        } else {
            clock clipboard_time;
            start_timer(&clipboard_time);

            // If it hasn't been 500ms yet, then wait 500ms to prevent too much
            // spam
            const int spam_time_ms = 500;
            if (get_timer(clipboard_time) < spam_time_ms / (double)MS_IN_SECOND) {
                fractal_sleep(
                    max((int)(spam_time_ms - MS_IN_SECOND * get_timer(clipboard_time)), 1));
            }
        }

        LOG_INFO("Updated clipboard!");
        updating_clipboard = false;
    }

    return 0;
}
