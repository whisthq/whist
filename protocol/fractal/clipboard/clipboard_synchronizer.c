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

init_clipboard_synchronizer(true);

ClipboardData received_clipboard_chunk;

// Will push this chunk onto our clipboard
push_clipboard_chunk(&received_clipboard_chunk);

ClipboardData* our_chunk_to_send = pull_clipboard_chunk();

if (our_chunk_to_send) {
  // We have a new clipboard chunk, this should be sent to the server
  Send(our_chunk_to_send);
} else {
  // There is no new clipboard
}

destroy_clipboard_synchronizer();
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

ClipboardActivity current_clipboard_activity;

/*
============================
Private Functions
============================
*/

bool clipboard_action_is_active(FractalClipboardActionType check_action_type);
bool start_clipboard_transfer(FractalClipboardActionType new_clipboard_action_type);
void finish_active_transfer(bool action_complete);
void reap_active_clipboard_action_thread();

/*** Thread Functions ***/
int push_clipboard_thread_function(void* opaque);
int pull_clipboard_thread_function(void* opaque);

/*
============================
Private Function Implementations
============================
*/

bool clipboard_action_is_active(FractalClipboardActionType check_action_type) {
    /*
        Check whether there is an action of type `clipboard_action_type`
        currently active

        Arguments:
            clipboard_action_type (FractalClipboardActionType): the action type to check for

        NOTE: must be called with `current_clipboard_activity.clipboard_action_mutex` held
    */

    if (
        current_clipboard_activity.is_initialized &&
        current_clipboard_activity.clipboard_action_type == check_action_type &&
        current_clipboard_activity.aborting_ptr != NULL &&
        !*(current_clipboard_activity.aborting_ptr) &&
        current_clipboard_activity.complete_ptr != NULL &&
        !*(current_clipboard_activity.complete_ptr) &&
        *(current_clipboard_activity.clipboard_buffer_ptr) != NULL
    ) {
        return true;
    }

    return false;
}

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

    LOG_INFO("Creating thread for clipboard transfer of type: %d", new_clipboard_action_type);

    if (new_clipboard_action_type == CLIPBOARD_ACTION_NONE) {
        return false;
    }

    // Abort an active transfer if it exists
    finish_active_transfer(false);

    // Create new thread
    if (new_clipboard_action_type == CLIPBOARD_ACTION_PUSH) {
        current_clipboard_activity.active_clipboard_action_thread =
            fractal_create_thread(push_clipboard_thread_function, "push_clipboard_thread", NULL);
    } else {
        current_clipboard_activity.active_clipboard_action_thread =
            fractal_create_thread(pull_clipboard_thread_function, "pull_clipboard_thread", NULL);
    }

    if (current_clipboard_activity.active_clipboard_action_thread == NULL) {
        return false;
    }

    // Make sure the thread is set up before continuing because we need certain thread-owned
    //     variables to be set up before continuing with chunk transfers
    // Don't worry about wait_semaphore hanging here because a successful
    //     thread creation will increment the semaphore.
    fractal_wait_semaphore(current_clipboard_activity.thread_setup_semaphore);

    return true;
}

void finish_active_transfer(bool action_complete) {
    /*
        If active clipboard action thread exists, wake up the thread,
        set all variables that will allow this thread to exit,
        then detach the thread and set the global thread tracker to NULL.

        Arguments:
            action_complete (bool): true if the action is complete, false
                if the action is being aborted

        NOTE: must be called with `current_clipboard_activity.clipboard_action_mutex` held
    */

    // If active thread: if action is complete, then we log that we are
    //     finishing the current action, otherwise log that we are cancelling
    //     the current action.
    if (current_clipboard_activity.active_clipboard_action_thread) {
        if (action_complete) {
            LOG_INFO("Finishing current clipboard action");
        } else {
            LOG_INFO("Cancelling current clipboard action");
        }
    }

    // Reset non-pointer members that don't belong to the thread
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_NONE;
    current_clipboard_activity.pulled_bytes = 0;

    // If action is complete, then we signal the complete flag, otherwise
    //     signal the abort flag
    if (action_complete) {
        if (current_clipboard_activity.complete_ptr) {
            *(current_clipboard_activity.complete_ptr) = true;
        }
    } else {
        if (current_clipboard_activity.aborting_ptr) {
            *(current_clipboard_activity.aborting_ptr) = true;
        }
    }

    // Wake up the current thread and clean up its resources
    fractal_broadcast_cond(current_clipboard_activity.continue_action_condvar);
    reap_active_clipboard_action_thread();
}

void reap_active_clipboard_action_thread() {
    /*
        Detach and reset the active action thread and its associated members

        NOTE: must be called with `current_clipboard_activity.clipboard_action_mutex` held
    */

    // It is safe to pass NULL to `fractal_detach_thread`, so we don't need to check
    fractal_detach_thread(current_clipboard_activity.active_clipboard_action_thread);
    current_clipboard_activity.active_clipboard_action_thread = NULL;
    current_clipboard_activity.aborting_ptr = NULL;
    current_clipboard_activity.complete_ptr = NULL;
    current_clipboard_activity.clipboard_buffer_ptr = NULL;
}

/*** Thread Functions ***/

int push_clipboard_thread_function(void* opaque) {
    /*
        Ephemeral update thread function for a clipboard push. This
        thread runs as long as an active clipboard push action is occurring.
        Once all chunks have been handled, or the action has been aborted, the
        loop exits and the thread finishes by cleaning up resources.

        Arguments:
            opaque (void*): thread argument (unused)

        Return:
            (int): 0 on success

        NOTE: thread must be created with `current_clipboard_activity.clipboard_action_mutex` held.
            `current_clipboard_activity.clipboard_action_mutex` is safe to release when thread posts
            `current_clipboard_activity.thread_setup_semaphore`.
    */

    LOG_INFO("Begun pushing clipboard");

    // Set thread status members
    bool aborting = false;
    bool complete = false;

    // When thread is created, create initial buffer
    ClipboardData* clipboard_buffer = allocate_region(sizeof(ClipboardData));
    clipboard_buffer->size = 0;

    current_clipboard_activity.aborting_ptr = &aborting;
    current_clipboard_activity.complete_ptr = &complete;
    current_clipboard_activity.clipboard_buffer_ptr = &clipboard_buffer;
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_PUSH;

    // Let the calling thread know that this thread's buffer is ready for pushing
    fractal_post_semaphore(current_clipboard_activity.thread_setup_semaphore);

    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // Make sure we are still the active thread before blocking this thread
    // If we are not the active thread, then clean up and return because we have
    //     been detached.
    if (current_clipboard_activity.active_clipboard_action_thread == NULL ||
        fractal_get_thread_id(current_clipboard_activity.active_clipboard_action_thread) !=
        fractal_get_thread_id(NULL)) {
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

        deallocate_region(clipboard_buffer);
        return 0;
    }

    // When this condition is signaled, we can continue this thread. This condition is signaled in one of
    //     two cases:
    //     1) the current action has completed
    //     2) the current action has been aborted
    // This unlocks the mutex and blocks until signaled
    fractal_wait_cond(
        current_clipboard_activity.continue_action_condvar,
        current_clipboard_activity.clipboard_action_mutex
    );

    // Handle the allocated clipboard buffer
    // If the current action has completed, then we can safely
    //     push the buffer onto the OS clipboard
    if (complete) {
        set_os_clipboard(clipboard_buffer);
    }

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // Free the allocated clipboard buffer
    deallocate_region(clipboard_buffer);

    return 0;
}

int pull_clipboard_thread_function(void* opaque) {
    /*
        Ephemeral update thread function for a clipboard pull. This
        thread runs as long as an active clipboard pull action is occurring.
        Once all chunks have been handled, or the action has been aborted, the
        loop exits and the thread finishes by cleaning up resources.

        Arguments:
            opaque (void*): thread argument (unused)

        Return:
            (int): 0 on success

        NOTE: thread must be created with `current_clipboard_activity.clipboard_action_mutex` held.
            `current_clipboard_activity.clipboard_action_mutex` is safe to release when thread posts
            `current_clipboard_activity.thread_setup_semaphore`.
    */

    LOG_INFO("Begun pulling clipboard");

    // Set thread status members
    bool aborting = false;
    bool complete = false;

    // Let the pull thread know that it is safe to continue while the
    //     clipboard is being pulled
    fractal_post_semaphore(current_clipboard_activity.thread_setup_semaphore);

    // When thread is created, pull the clipboard
    ClipboardData* clipboard_buffer = get_os_clipboard();

    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // Make sure we are still the active thread before setting globals.
    // If we are not the active thread, then clean up and return because we have
    //     been detached.
    if (current_clipboard_activity.active_clipboard_action_thread == NULL ||
        fractal_get_thread_id(current_clipboard_activity.active_clipboard_action_thread) !=
        fractal_get_thread_id(NULL)) {
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

        free_clipboard_buffer(clipboard_buffer);
        return 0;
    }

    current_clipboard_activity.aborting_ptr = &aborting;
    current_clipboard_activity.complete_ptr = &complete;
    current_clipboard_activity.clipboard_buffer_ptr = &clipboard_buffer;
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_PULL;

    // When this condition is signaled, we can continue this thread. This condition is signaled in one of
    //     two cases:
    //     1) the current action has completed
    //     2) the current action has been aborted
    // This unlocks the mutex and blocks until signaled
    fractal_wait_cond(
        current_clipboard_activity.continue_action_condvar,
        current_clipboard_activity.clipboard_action_mutex
    );

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // After calling `get_os_clipboard()`, we call `free_clipboard_buffer()`
    free_clipboard_buffer(clipboard_buffer);

    return 0;
}

/*
============================
Public Function Implementations
============================
*/

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

    current_clipboard_activity.active_clipboard_action_thread = NULL;
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_NONE;
    current_clipboard_activity.aborting_ptr = NULL;
    current_clipboard_activity.complete_ptr = NULL;
    current_clipboard_activity.clipboard_buffer_ptr = NULL;

    current_clipboard_activity.pulled_bytes = 0;

    current_clipboard_activity.clipboard_action_mutex = fractal_create_mutex();
    current_clipboard_activity.continue_action_condvar = fractal_create_cond();
    current_clipboard_activity.thread_setup_semaphore = fractal_create_semaphore(0);

    current_clipboard_activity.is_initialized = true;
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

    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    LOG_INFO("Pushing clipboard chunk of size %d", cb_chunk->size);

    // If received start chunk, then start new transfer
    if (cb_chunk->chunk_type == CLIPBOARD_START) {
        if (start_clipboard_transfer(CLIPBOARD_ACTION_PUSH)) {
            fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);
        } else {
            LOG_ERROR("Failed to start a new clipboard push action");
            fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
            return;
        }
    }

    // If we're not actively pushing to clipboard, return immediately
    // This also takes care of any state changes that may have occurred between the
    //     `fractal_wait_semaphore` and `fractal_lock_mutex` after a successful
    //     `start_clipboard_transfer`
    if (!clipboard_action_is_active(CLIPBOARD_ACTION_PUSH)) {
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return;
    }

    // Add chunk contents onto clipboard buffer and update clipboard size
    *current_clipboard_activity.clipboard_buffer_ptr =
        realloc_region((*current_clipboard_activity.clipboard_buffer_ptr),
                       sizeof(ClipboardData) + (*current_clipboard_activity.clipboard_buffer_ptr)->size + cb_chunk->size);

    if (cb_chunk->chunk_type == CLIPBOARD_START) {
        // First chunk needs to memcpy full contents into clipboard buffer because it contains
        //     clipboard detail fields that need to be copied (e.g. size, type)
        memcpy(*current_clipboard_activity.clipboard_buffer_ptr, cb_chunk, sizeof(ClipboardData) + cb_chunk->size);
    } else {
        // Remaining chunks need to memcpy just the data buffer into clipboard buffer and also
        //     need to manually increment clipboard size
        memcpy((*current_clipboard_activity.clipboard_buffer_ptr)->data + (*current_clipboard_activity.clipboard_buffer_ptr)->size, cb_chunk->data,
               cb_chunk->size);
        (*current_clipboard_activity.clipboard_buffer_ptr)->size += cb_chunk->size;
    }

    if (cb_chunk->chunk_type == CLIPBOARD_FINAL) {
        // ready to set OS clipboard
        LOG_INFO("Finishing clipboard push");
        finish_active_transfer(true);
    }

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
}

ClipboardData* pull_clipboard_chunk() {
    /*
        When called, return the current clipboard chunk if a new clipboard activity
        has registered, or if the recently updated clipboard is being read.

        NOTE: this should typically be called in a loop of `while not NULL`,
              or similar to get all the chunks in the OS clipboard

        Returns:
            cb_chunk (ClipboardData*): pointer to the latest clipboard chunk
                NOTE: cb_chunk must be freed by caller if not NULL
    */

    if (!current_clipboard_activity.is_initialized) {
        LOG_WARNING("Tried to pull_clipboard_chunk, but the clipboard is not initialized");
        return NULL;
    }

    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // If clipboard has updated, start new transfer
    // We don't want to wait around for `start_clipboard_transfer` to finish on this
    //     function call because we would hang the thread
    if (has_os_clipboard_updated()) {
        start_clipboard_transfer(CLIPBOARD_ACTION_PULL);
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return NULL;
    }

    // If we're not actively pulling from clipboard, then return immediately
    if (!clipboard_action_is_active(CLIPBOARD_ACTION_PULL)) {
        fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return NULL;
    }

    LOG_INFO("Pulling next clipboard chunk");

    int chunk_size = min(CHUNK_SIZE,
        (*current_clipboard_activity.clipboard_buffer_ptr)->size -
        current_clipboard_activity.pulled_bytes);
    ClipboardData* cb_chunk = allocate_region(sizeof(ClipboardData) + chunk_size);
    cb_chunk->size = chunk_size;
    cb_chunk->type = (*current_clipboard_activity.clipboard_buffer_ptr)->type;

    // for FINAL, this will just "copy" 0 bytes
    memcpy(cb_chunk->data, (*current_clipboard_activity.clipboard_buffer_ptr)->data + current_clipboard_activity.pulled_bytes, chunk_size);

    // based on how many bytes have been written so far, set the chunk type
    if (current_clipboard_activity.pulled_bytes == 0) {
        cb_chunk->chunk_type = CLIPBOARD_START;
    } else if (current_clipboard_activity.pulled_bytes == (*current_clipboard_activity.clipboard_buffer_ptr)->size) {
        cb_chunk->chunk_type = CLIPBOARD_FINAL;
        LOG_INFO("Finished pulling chunks from OS clipboard");
        finish_active_transfer(true);
    } else {
        cb_chunk->chunk_type = CLIPBOARD_MIDDLE;
    }

    // update pulled bytes
    current_clipboard_activity.pulled_bytes += chunk_size;

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
    return cb_chunk;
}

void destroy_clipboard_synchronizer() {
    /*
        Cleanup the clipboard synchronizer and all resources

        NOTE: Do not call this function until all threads that can call
            `pull_clipboard_chunk` and `push_clipboard_chunk` have exited.
    */

    LOG_INFO("Trying to destroy clipboard synchronizer...");

    // If the clipboard is not initialized, then there is nothing to destroy
    if (!current_clipboard_activity.is_initialized) {
        LOG_ERROR("Clipboard synchronizer is not initialized!");
        return;
    }

    fractal_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    current_clipboard_activity.is_initialized = false;
    finish_active_transfer(false);

    fractal_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    fractal_destroy_mutex(current_clipboard_activity.clipboard_action_mutex);
    fractal_destroy_cond(current_clipboard_activity.continue_action_condvar);
    fractal_destroy_semaphore(current_clipboard_activity.thread_setup_semaphore);

    // NOTE: Bad things could happen if initialize_clipboard is run
    // while destroy_clipboard() is running
    destroy_clipboard();

    LOG_INFO("Finished destroying clipboard synchronizer");
}
