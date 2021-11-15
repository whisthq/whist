/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
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

// #define MS_IN_SECOND 1000

ClipboardActivity current_clipboard_activity;
ClipboardData* active_clipboard_buffer = NULL;
int clipboard_written_bytes = 0;

/*
============================
Private Functions
============================
*/

bool start_clipboard_transfer(FractalClipboardActionType new_clipboard_action_type);
void finish_clipboard_transfer();
void reap_active_clipboard_action_thread();
void reset_active_clipboard_activity();

/*** Thread Functions ***/
void transfer_clipboard_wait_loop(FractalClipboardActionType transfer_action_type);
int push_clipboard_thread_function(void* opaque);
int pull_clipboard_thread_function(void* opaque);

/*
============================
Private Function Implementations
============================
*/

bool start_clipboard_transfer(FractalClipboardActionType new_clipboard_action_type) {
    /*
        Create the thread for the active clipboard transfer (push or pull).

        Arguments:
            new_clipboard_action_type (FractalClipboardActionType):
                the new transfer type (PUSH or PULL)

        Returns:
            (bool): whether an activity thread has been created for the transfer

        NOTE: must be called with `current_clipboard_activity.clipboard_action_mutex` held
    */

    reset_active_clipboard_activity();

    LOG_INFO("Creating thread for clipboard transfer of type: %d", new_clipboard_action_type);

    // Create new thread
    if (new_clipboard_action_type == CLIPBOARD_ACTION_PUSH) {
        current_clipboard_activity.active_clipboard_action_thread =
        fractal_create_thread(push_clipboard_thread_function, "push_clipboard_thread", NULL);
    } else if (new_clipboard_action_type == CLIPBOARD_ACTION_PULL) {
        current_clipboard_activity.active_clipboard_action_thread =
        fractal_create_thread(pull_clipboard_thread_function, "pull_clipboard_thread", NULL);
    }

    return current_clipboard_activity.active_clipboard_action_thread != NULL;
}

void finish_clipboard_transfer() {
    /*
        Indicate that the active clipboard activity is complete and the
        thread can take care of the final clipboard push and/or clear.
    */

    LOG_INFO("Completing current clipboard action");

    current_clipboard_activity.action_completed = true;
}

void reap_active_clipboard_action_thread() {
    LOG_INFO("Reaping current clipboard action thread");
    if (current_clipboard_activity.active_clipboard_action_thread) {
        fractal_post_semaphore(current_clipboard_activity.chunk_transfer_semaphore);
        fractal_wait_thread(current_clipboard_activity.active_clipboard_action_thread, NULL);
    }
    current_clipboard_activity.active_clipboard_action_thread = NULL;
}

void reset_active_clipboard_activity() {
    /*
        When an active clipboard action has been interrupted by a new one,
        cancel the active action by exiting the thread and resetting all
        the action tracking variables. If there is no active action, it will
        just reset the active clipboard activity values without preforming
        any thread functions.

        NOTE: must be called with `current_clipboard_activity.clipboard_action_mutex` held
    */

    LOG_INFO("Resetting active clipboard action");

    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_NONE;
    current_clipboard_activity.action_completed = true;
    // If active thread, post semaphore and wait for thread to exit loop and finish
    if (current_clipboard_activity.active_clipboard_action_thread) {
        LOG_INFO("Cancelling current clipboard action");
        reap_active_clipboard_action_thread();
    }
    current_clipboard_activity.action_completed = false;
    clipboard_written_bytes = 0;
}

void transfer_clipboard_wait_loop(FractalClipboardActionType transfer_action_type) {
    /*
        Loop to wait for the clipboard transfer to complete or abort. This
        function is called by both the push and pull clipboard threads.

        NOTE: `current_clipboard_activity.clipboard_action_mutex` must NOT be held
        when calling this function. This function will return with this mutex HELD.
    */

    LOG_INFO("Waiting for clipboard action to complete");

    fractal_post_semaphore(current_clipboard_activity.transfer_thread_setup_semaphore);

    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);
    // semaphore is signaled each time a chunk is pushed to the clipboard
    while (current_clipboard_activity.clipboard_action_type == transfer_action_type &&
            !current_clipboard_activity.action_completed) {
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        fractal_wait_semaphore(current_clipboard_activity.chunk_transfer_semaphore);
        fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);
    }
}

int push_clipboard_thread_function(void* opaque) {
    /*
        Ephemeral update thread function for a clipboard push. This thread
        runs as long as an active clipboard push action is occurring. Once
        all chunks have been received, or the action has been aborted, the
        loop exits and the thread finishes by pushing the clipboard buffer
        onto the OS clipboard (or doing nothing if aborting) and cleaning
        up resources.

        Arguments:
            opaque (void*): thread argument (unused)

        Return:
            (int): 0 on success
    */

    UNUSED(opaque);

    LOG_INFO("Begun pushing clipboard");

    active_clipboard_buffer = allocate_region(sizeof(ClipboardData));
    active_clipboard_buffer->size = 0;

    current_clipboard_activity.action_completed = false;
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_PUSH;

    // Wait for all chunks to be pushed onto clipboard buffer
    transfer_clipboard_wait_loop(CLIPBOARD_ACTION_PUSH);

    // If the action is no longer CLIPBOARD_ACTION_PUSH, that means the current action
    //     has been cancelled and we don't want to set the clipboard
    if (current_clipboard_activity.clipboard_action_type == CLIPBOARD_ACTION_PUSH) {
        if (active_clipboard_buffer) {
            LOG_INFO("Setting OS clipboard");
            set_clipboard(active_clipboard_buffer);
        }
    }

    if (active_clipboard_buffer) {
        deallocate_region(active_clipboard_buffer);
    }
    active_clipboard_buffer = NULL;

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    reap_active_clipboard_action_thread();

    return 0;
}

int pull_clipboard_thread_function(void* opaque) {
    /*
        Ephemeral update thread function for a clipboard pull. This thread
        runs as long as an active clipboard pull action is occurring. Once
        all chunks have been sent, or the action has been aborted, the
        loop exits and the thread finishes by cleaning up resources.

        Arguments:
            opaque (void*): thread argument (unused)

        Return:
            (int): 0 on success

        NOTE: this thread should only be created when `has_clipboard_updated`
            evaluates to true
    */

    LOG_INFO("Begun pulling clipboard");

    current_clipboard_activity.action_completed = false;
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_PULL;

    // When thread is created, pull the clipboard
    active_clipboard_buffer = get_clipboard();

    // Wait for all chunks to be pulled from clipboard buffer
    transfer_clipboard_wait_loop(CLIPBOARD_ACTION_PULL);

    // After calling `get_clipboard()`, we call `free_clipboard()`
    if (active_clipboard_buffer) {
        free_clipboard(active_clipboard_buffer);
    }
    active_clipboard_buffer = NULL;

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    reap_active_clipboard_action_thread();

    return 0;
}


/*
============================
Public Function Implementations
============================
*/

bool is_clipboard_synchronizing() {
    /*
        Whether an active clipboard push or pull action is in effect

        Returns:
            (bool): true if clipboard is synchronizing, false otherwise
    */

    return current_clipboard_activity.active_clipboard_action_thread != NULL;
}

void init_clipboard_synchronizer(bool is_client) {
    /*
        Initialize the clipboard synchronizer

        Arguments:
            is_client (bool): whether the caller is the client (true) or the server (false)
    */

    LOG_INFO("Initializing clipboard");

    if (current_clipboard_activity.is_initialized) {
        LOG_ERROR("Tried to init_clipboard, but the clipboard is already initialized");
        return;
    }

    init_clipboard(is_client);

    current_clipboard_activity.is_initialized = true;
    current_clipboard_activity.active_clipboard_action_thread = NULL;
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_NONE;
    current_clipboard_activity.clipboard_action_mutex = fractal_create_mutex();
    current_clipboard_activity.chunk_transfer_semaphore = fractal_create_semaphore(0);
    current_clipboard_activity.transfer_thread_setup_semaphore = fractal_create_semaphore(0);
    current_clipboard_activity.action_completed = false;
    clipboard_written_bytes = 0;
}

ClipboardData* pull_clipboard_chunk() {
    /*
        When called, return the current clipboard chunk if a new clipboard activity
        has registered, or if the recently updated clipboard is being read.

        NOTE: this should typically be called in a loop of `while not NULL`,
              or similar

        Returns:
            cb_chunk (ClipboardData*): pointer to the latest clipboard chunk
                NOTE: cb_chunk must be freed by caller if not NULL
    */

    if (!current_clipboard_activity.is_initialized) {
        LOG_WARNING("Tried to pull_clipboard_chunk, but the clipboard is not initialized");
        return NULL;
    }

    // is this too many locks (every iteration)?
    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // If clipboard has updated, start new transfer
    if (has_clipboard_updated()) {
        start_clipboard_transfer(CLIPBOARD_ACTION_PULL);
        // Wait for thread to be set up before continuing to pull first chunk from buffer
        fractal_wait_semaphore(current_clipboard_activity.transfer_thread_setup_semaphore);
    }

    // Clipboard thread must be active with a PULL action
    if (!(is_clipboard_synchronizing() &&
        current_clipboard_activity.clipboard_action_type == CLIPBOARD_ACTION_PULL)) {
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return NULL;
    }

    if (active_clipboard_buffer && !current_clipboard_activity.action_completed) {
        LOG_INFO("Pulling next clipboard chunk");

        int chunk_size = min(CHUNK_SIZE, active_clipboard_buffer->size - clipboard_written_bytes);
        ClipboardData* cb_chunk = allocate_region(sizeof(ClipboardData) + chunk_size);
        cb_chunk->size = chunk_size;
        cb_chunk->type = active_clipboard_buffer->type;
        if (clipboard_written_bytes == 0) {
            cb_chunk->chunk_type = CLIPBOARD_START;
        } else if (clipboard_written_bytes == active_clipboard_buffer->size) {
            cb_chunk->chunk_type = CLIPBOARD_FINAL;
        } else {
            cb_chunk->chunk_type = CLIPBOARD_MIDDLE;
        }

        // for FINAL, this will just "copy" 0 bytes
        memcpy(cb_chunk->data, active_clipboard_buffer->data + clipboard_written_bytes, chunk_size);
        if (clipboard_written_bytes == active_clipboard_buffer->size) {
            finish_clipboard_transfer();
        }

        // update written bytes
        clipboard_written_bytes += chunk_size;

        fractal_post_semaphore(current_clipboard_activity.chunk_transfer_semaphore);

        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

        return cb_chunk;
    }

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    return NULL;
}

void push_clipboard_chunk(ClipboardData* cb_chunk) {
    /*
        When called, push the given chunk onto the active clipboard buffer

        Arguments:
            cb (ClipboardData*): pointer to the clipboard chunk to push
    */

    if (!current_clipboard_activity.is_initialized) {
        LOG_ERROR("Tried to push_clipboard_chunk, but the clipboard is not initialized");
        return;
    }

    LOG_INFO("Pushing clipboard chunk of size %d", cb_chunk->size);

    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // If received start chunk, then start new transfer
    if (cb_chunk->chunk_type == CLIPBOARD_START) {
        if (start_clipboard_transfer(CLIPBOARD_ACTION_PUSH)) {
            // Wait for thread to be set up before continuing to push chunk onto buffer
            fractal_wait_semaphore(current_clipboard_activity.transfer_thread_setup_semaphore);
        } else {
            LOG_ERROR("Failed to start a new clipboard push action");
            fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
            return;
        }
    }

    // Clipboard thread must be active with a PUSH action
    if (!(is_clipboard_synchronizing() &&
        current_clipboard_activity.clipboard_action_type == CLIPBOARD_ACTION_PUSH)) {
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return;
    }

    if (active_clipboard_buffer) {
        // Add chunk contents onto clipboard buffer and update clipboard size
        active_clipboard_buffer =
            realloc_region(active_clipboard_buffer, sizeof(ClipboardData) + active_clipboard_buffer->size + cb_chunk->size);

        if (cb_chunk->chunk_type == CLIPBOARD_START) {
            // First chunk needs to memcpy full contents into clipboard buffer because it contains
            //     clipboard detail fields that need to be copied (e.g. size, type)
            memcpy(active_clipboard_buffer, cb_chunk, sizeof(ClipboardData) + cb_chunk->size);
        } else {
            // Remaining chunks need to memcpy just the data buffer into clipboard buffer and also
            //     need to manually increment clipboard size
            memcpy(active_clipboard_buffer->data + active_clipboard_buffer->size, cb_chunk->data, cb_chunk->size);
            active_clipboard_buffer->size += cb_chunk->size;
        }

        if (cb_chunk->chunk_type == CLIPBOARD_FINAL) {
            // ready to set OS clipboard
            finish_clipboard_transfer();
            LOG_INFO("Finishing clipboard push");
        }
    }

    fractal_post_semaphore(current_clipboard_activity.chunk_transfer_semaphore);

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
}

void destroy_clipboard_synchronizer() {
    /*
        Cleanup the clipboard synchronizer and all resources
    */

    LOG_INFO("Trying to destroy clipboard synchronizer...");

    // If the clipboard is not initialized, then there is nothing to destroy
    if (!current_clipboard_activity.is_initialized) {
        LOG_ERROR("Clipboard synchronizer is not initialized!");
        return;
    }

    current_clipboard_activity.is_initialized = false;

    // If the clipboard is currently being updated, then cancel that action
    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);
    if (current_clipboard_activity.clipboard_action_type != CLIPBOARD_ACTION_NONE) {
        reset_active_clipboard_activity();
    }
    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    fractal_destroy_mutex(current_clipboard_activity.clipboard_action_mutex);
    fractal_destroy_semaphore(current_clipboard_activity.chunk_transfer_semaphore);

    // NOTE: Bad things could happen if initialize_clipboard is run
    // while destroy_clipboard() is running
    destroy_clipboard();

    LOG_INFO("Finished destroying clipboard synchronizer");
}
