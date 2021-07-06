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
clock last_clipboard_update;
FractalSemaphore clipboard_semaphore;  // used to signal clipboard_synchronizer_thread to continue
FractalMutex clipboard_update_mutex;   // used to protect the global clipboard and update toggles
ClipboardData* clipboard;
int clipboard_written_bytes = 0;  // number of bytes of clipboard already transmitted
FractalThread clipboard_synchronizer_thread;
static bool is_initialized = false;

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

    if (!is_initialized) {
        LOG_ERROR("Tried to is_clipboard_synchronizing, but the clipboard is not initialized");
        return true;
    }
    return updating_clipboard || pending_clipboard_push;
}

void init_clipboard_synchronizer(bool is_client) {
    /*
        Initialize the clipboard and the synchronizer thread

        Arguments:
            is_client (bool): whether the caller is the client (true) or the server (false)
    */

    LOG_INFO("Initializing clipboard");

    if (is_initialized) {
        LOG_ERROR("Tried to init_clipboard, but the clipboard is already initialized");
        return;
    }

    init_clipboard(is_client);

    pending_clipboard_push = false;
    updating_clipboard = false;
    start_timer((clock*)&last_clipboard_update);
    clipboard_semaphore = fractal_create_semaphore(0);
    clipboard_update_mutex = fractal_create_mutex();

    // is_initialized = true must be at the bottom,
    // so that if (!is_initialized) checks work
    is_initialized = true;
    // The update loop will happen after is_initialized is set true,
    // This update loop will exit if is_initialized is false
    clipboard_synchronizer_thread =
        fractal_create_thread(update_clipboard, "update_clipboard", NULL);
}

void destroy_clipboard_synchronizer() {
    /*
        Clean up and destroy the clipboard synchronizer
    */

    LOG_INFO("Destroying clipboard");

    if (!is_initialized) {
        LOG_ERROR("Tried to destroy_clipboard, but the clipboard is already destroyed");
        return;
    }

    if (updating_clipboard) {
        LOG_FATAL("Trying to destroy clipboard while the clipboard is being updated");
    }

    is_initialized = false;

    // NOTE: Bad things could happen if initialize_clipboard is run
    // While destroy_clipboard is running

    destroy_clipboard();

    fractal_post_semaphore(clipboard_semaphore);
}

void clipboard_synchronizer_abort_active_clipboard_action() {
    /*
        Abort an active synchronizer get or set by deallocating the active clipboard
        and resetting all update booleans to false.

        NOTE: this must be called with `clipboard_update_mutex` held
    */

    // Release the clipboard global
    if (clipboard) {
        if (pending_clipboard_push) {
            // This means that we are actively "getting" the clipboard,
            //     which is the only case in which we should call `free_clipboard`
            free_clipboard(clipboard);
        } else {
            deallocate_region(clipboard);
        }
        clipboard = NULL;
    }
    updating_clipboard = false;
    updating_set_clipboard = false;
    updating_get_clipboard = false;
    pending_clipboard_push = false;
    clipboard_written_bytes = 0;
}

bool clipboard_synchronizer_set_clipboard_chunk(ClipboardData* cb_chunk) {
    /*
        When called, signal that the clipboard can be set to the given contents

        Arguments:
            cb (ClipboardData*): pointer to the clipboard chunk to load

        Returns:
            updatable (bool): whether the clipboard can be set right now
    */

    if (!is_initialized) {
        LOG_ERROR("Tried to set_clipboard, but the clipboard is not initialized");
        return false;
    }

    fractal_lock_mutex(clipboard_update_mutex);

    // If the clipboard is currently being updated and the received chunk is a
    //     START chunk, then abort the active action
    if (updating_clipboard && cb_chunk->chunk_type == CLIPBOARD_START) {
        clipboard_synchronizer_abort_active_clipboard_action();
    }

    updating_clipboard = true;
    updating_set_clipboard = true;
    updating_get_clipboard = false;

    // clipboard->chunk_type is unused, but cb_chunk->chunk_type is important
    if (cb_chunk->chunk_type == CLIPBOARD_START) {
        clipboard = allocate_region(sizeof(ClipboardData) + cb_chunk->size);
        memcpy(clipboard, cb_chunk, sizeof(ClipboardData) + cb_chunk->size);
    } else {
        // if not the first clipboard chunk, then add onto the end of the buffer
        //     and update clipboard size
        clipboard =
            realloc_region(clipboard, sizeof(ClipboardData) + clipboard->size + cb_chunk->size);
        memcpy(clipboard->data + clipboard->size, cb_chunk->data, cb_chunk->size);
        clipboard->size += cb_chunk->size;

        if (cb_chunk->chunk_type == CLIPBOARD_FINAL) {
            // ready to set OS clipboard
            fractal_post_semaphore(clipboard_semaphore);
        }
    }

    fractal_unlock_mutex(clipboard_update_mutex);

    return true;
}

ClipboardData* clipboard_synchronizer_get_next_clipboard_chunk() {
    /*
        When called, return the current clipboard chunk if a new clipboard activity
        has registered, or if the recently updated clipboard is being read.

        NOTE: this should typically be called in a loop of `while not NULL`,
              or similar

        Returns:
            cb_chunk (ClipboardData*): pointer to the latest clipboard chunk
    */

    if (!is_initialized) {
        LOG_ERROR("Tried to get_new_clipboard, but the clipboard is not initialized");
        return NULL;
    }

    fractal_lock_mutex(clipboard_update_mutex);

    // If clipboard has updated - if is currently being updated, abort that action
    if (has_clipboard_updated()) {
        if (updating_clipboard) {
            clipboard_synchronizer_abort_active_clipboard_action();
        }
        LOG_INFO("Pushing update to clipboard");
        updating_clipboard = true;
        updating_set_clipboard = false;
        updating_get_clipboard = true;

        fractal_unlock_mutex(clipboard_update_mutex);
        fractal_post_semaphore(clipboard_semaphore);

        return NULL;
    }

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
            clipboard_written_bytes = 0;
        }

        // update written bytes
        clipboard_written_bytes += chunk_size;

        fractal_unlock_mutex(clipboard_update_mutex);

        return cb_chunk;
    }

    fractal_unlock_mutex(clipboard_update_mutex);

    return NULL;
}

int update_clipboard(void* opaque) {
    /*
        Thread to get and set clipboard as signals are received.
    */

    UNUSED(opaque);

    while (is_initialized) {
        fractal_wait_semaphore(clipboard_semaphore);

        if (!is_initialized) {
            break;
        }

        fractal_lock_mutex(clipboard_update_mutex);

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

        fractal_unlock_mutex(clipboard_update_mutex);
    }

    return 0;
}
