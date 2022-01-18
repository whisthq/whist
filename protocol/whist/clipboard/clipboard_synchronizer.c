/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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

#include <whist/core/whist.h>
#include <whist/utils/atomic.h>
#include "whist/utils/linked_list.h"
#include "clipboard.h"

/*
============================
Defines
============================
*/

ClipboardActivity current_clipboard_activity;

// Need to be protected by current_clipboard_activity.clipboard_action_mutex
// We don't include these in the struct because they are not specific to the
//     active action thread. They can persist for detached threads depending
//     on whether those threads are the latest ones to need to set the OS
//     clipboard with their buffers.
// setting_os_clipboard indicates whether a thread is actively setting the OS clipboard
bool setting_os_clipboard;
// queued_os_clipboard_setter_thread_id indicates which thread gets to set the OS clipboard next
WhistThreadID queued_os_clipboard_setter_thread_id;
// condvar to signal whenever the OS clipboard setting thread has updated to another thread
//     (meaning that `queued_os_clipboard_setting_thread_id` is a new non-zero number) and also
//     to signal when the OS clipboard is available for setting again (`setting_os_clipboard` is
//     false)
WhistCondition os_clipboard_setting_condvar;

typedef struct ClipboardThread {
    LINKED_LIST_HEADER;
    // Thread handle: to allow joining the thread.
    WhistThread thread;
    // Thread finish marker: initially zero, thread can be joined (hopefully)
    // without blocking when it becomes nonzero.
    atomic_int finished;
} ClipboardThread;

// Linked list of running (/ possibly-finished) clipboard threads.
// Protected by current_clipboard_activity.clipboard_action_mutex.
static LinkedList clipboard_thread_list;

/*
============================
Private Functions
============================
*/

bool clipboard_action_is_active(WhistClipboardActionType check_action_type);
bool start_clipboard_transfer(WhistClipboardActionType new_clipboard_action_type);
void finish_active_transfer(bool action_complete);
void join_outstanding_threads(bool join_all);

/*** Thread Functions ***/
int push_clipboard_thread_function(void* opaque);
int pull_clipboard_thread_function(void* opaque);

/*
============================
Private Function Implementations
============================
*/

bool clipboard_action_is_active(WhistClipboardActionType check_action_type) {
    /*
        Check whether there is an action of type `clipboard_action_type`
        currently active

        Arguments:
            clipboard_action_type (WhistClipboardActionType): the action type to check for

        NOTE: must be called with `current_clipboard_activity.clipboard_action_mutex` held
    */

    if (current_clipboard_activity.is_initialized &&
        current_clipboard_activity.clipboard_action_type == check_action_type &&
        current_clipboard_activity.aborting_ptr != NULL &&
        !*(current_clipboard_activity.aborting_ptr) &&
        current_clipboard_activity.complete_ptr != NULL &&
        !*(current_clipboard_activity.complete_ptr) &&
        current_clipboard_activity.clipboard_buffer_ptr != NULL &&
        *(current_clipboard_activity.clipboard_buffer_ptr) != NULL) {
        return true;
    }

    return false;
}

bool start_clipboard_transfer(WhistClipboardActionType new_clipboard_action_type) {
    /*
        Create the thread for the active clipboard transfer (push or pull).

        Arguments:
            new_clipboard_action_type (WhistClipboardActionType):
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

    // Create thread reference in the linked list so we can join it later.
    ClipboardThread* thr = safe_malloc(sizeof(*thr));
    atomic_init(&thr->finished, 0);
    linked_list_add_tail(&clipboard_thread_list, thr);

    // Create new thread
    if (new_clipboard_action_type == CLIPBOARD_ACTION_PUSH) {
        current_clipboard_activity.active_clipboard_action_thread =
            whist_create_thread(push_clipboard_thread_function, "push_clipboard_thread", thr);
    } else {
        current_clipboard_activity.active_clipboard_action_thread =
            whist_create_thread(pull_clipboard_thread_function, "pull_clipboard_thread", thr);
    }
    thr->thread = current_clipboard_activity.active_clipboard_action_thread;

    if (current_clipboard_activity.active_clipboard_action_thread == NULL) {
        return false;
    }

    // Make sure the thread is set up before continuing because we need certain thread-owned
    //     variables to be set up before continuing with chunk transfers
    // Don't worry about wait_semaphore hanging here because a successful
    //     thread creation will increment the semaphore.
    whist_wait_semaphore(current_clipboard_activity.thread_setup_semaphore);

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

    // If action is complete, then we signal the complete flag, otherwise
    //     signal the abort flag
    if (action_complete) {
        if (current_clipboard_activity.complete_ptr) {
            *(current_clipboard_activity.complete_ptr) = true;

            // We have successfully pushed all chunks onto the thread's buffer, so this thread
            //     is queued as the next thread to push its buffer onto the OS clipboard
            if (current_clipboard_activity.clipboard_action_type == CLIPBOARD_ACTION_PUSH) {
                queued_os_clipboard_setter_thread_id =
                    whist_get_thread_id(current_clipboard_activity.active_clipboard_action_thread);

                // Wake up all threads waiting to set the OS clipboard so they can determine
                //     whether they should stop waiting and die.
                whist_broadcast_cond(os_clipboard_setting_condvar);
            }
        }
    } else {
        if (current_clipboard_activity.aborting_ptr) {
            *(current_clipboard_activity.aborting_ptr) = true;
        }
    }

    // Reset non-pointer members that don't belong to the thread
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_NONE;
    current_clipboard_activity.pulled_bytes = 0;

    // Wake up the current thread and clean up its resources
    whist_broadcast_cond(current_clipboard_activity.continue_action_condvar);

    current_clipboard_activity.active_clipboard_action_thread = NULL;
    current_clipboard_activity.aborting_ptr = NULL;
    current_clipboard_activity.complete_ptr = NULL;
    current_clipboard_activity.clipboard_buffer_ptr = NULL;

    // Join any threads which have actually finished.  The current thread may
    // or may not finish in time - this is fine, we will join it next time if
    // if so.
    join_outstanding_threads(false);
}

void join_outstanding_threads(bool join_all) {
    /*
        Join with any outstanding clipboard threads which have finished.

        Arguments:
            join_all (bool): join all threads, waiting while they finish
                if necessary.

        NOTE: if a new transfer might be issued then should be called with
           `current_clipboard_activity.clipboard_action_mutex` held to avoid
            racing on manipulating the list, if that isn't possible (during
            destruction) then not necessary.
    */

    LOG_INFO("Joining outstanding threads.");

    linked_list_for_each(&clipboard_thread_list, ClipboardThread, thr) {
        if (join_all || atomic_load(&thr->finished)) {
            LOG_INFO("Joining %p.", thr->thread);
            whist_wait_thread(thr->thread, NULL);
            linked_list_remove(&clipboard_thread_list, thr);
            free(thr);
        } else {
            LOG_INFO("Not joining %p.", thr->thread);
        }
    }
}

/*** Thread Functions ***/

int push_clipboard_thread_function(void* opaque) {
    /*
        Ephemeral update thread function for a clipboard push. This
        thread runs as long as an active clipboard push action is occurring.
        Once all chunks have been handled, or the action has been aborted, the
        loop exits and the thread finishes by cleaning up resources.

        Arguments:
            opaque (void*): ClipboardThread reference

        Return:
            (int): 0 on success

        NOTE: thread must be created with `current_clipboard_activity.clipboard_action_mutex` held.
            `current_clipboard_activity.clipboard_action_mutex` is safe to release when thread posts
            `current_clipboard_activity.thread_setup_semaphore`.
    */

    LOG_INFO("Begun pushing clipboard");

    ClipboardThread* thread = opaque;

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
    whist_post_semaphore(current_clipboard_activity.thread_setup_semaphore);

    whist_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // When this condition is signaled, we can continue this thread.
    //     This condition is signaled in one of two cases:
    //     1) the current action has completed
    //     2) the current action has been aborted
    // This unlocks the mutex and blocks until signaled
    if (!complete && !aborting) {
        whist_wait_cond(current_clipboard_activity.continue_action_condvar,
                        current_clipboard_activity.clipboard_action_mutex);
    }

    // The logic below is necessary for the following reason:
    //     Say that a user performs two copies on the peer in quick succession, timed so that
    //     both push thread buffers have been completely filled, but order is lost
    //     when the mutex is acquired by `whist_wait_cond` and released before `set_os_clipboard`.
    //     This makes it nondeterministic as to whether the first thread or the second thread
    //     will set the OS clipboard first. To fix this problem, we maintain a queued setting thread
    //     that is assigned whenever we are finishing a push action (which is mutex protected and
    //     therefore still in order) and then force every push thread to check whether it is the
    //     queued setting thread and the OS clipboard is available to set with new contents before
    //     actually setting the OS clipboard with its own buffer. If a rapidly successive push
    //     overtakes a thread's position in the queue, it is bumped and aborted - it will not
    //     be setting its buffer on the OS clipboard at all.

    // If all chunks have been pushed to our buffer, then we consider whether we can set the OS
    //     clipboard
    if (complete) {
        // As long as we are the queued thread to set the OS clipboard and a different thread is
        // currently
        //     setting the clipboard, we wait until either a different thread has overtaken our spot
        //     in the queue or the currently setting thread has finished setting the OS clipboard.
        while (whist_get_thread_id(NULL) == queued_os_clipboard_setter_thread_id &&
               setting_os_clipboard) {
            whist_wait_cond(os_clipboard_setting_condvar,
                            current_clipboard_activity.clipboard_action_mutex);
        }

        // If we are still the queued setting thread and the OS clipboard is open to be set,
        //     we mark that the OS clipboard is being set
        // Note that the retrieved thread IDs cannot ever be the same for different active threads
        //     because a thread ID only gets released when the thread dies
        if (whist_get_thread_id(NULL) == queued_os_clipboard_setter_thread_id &&
            !setting_os_clipboard) {
            setting_os_clipboard = true;
            // Since we are the currently queued thread, we can reset the queued thread ID to 0
            queued_os_clipboard_setter_thread_id = 0;
        } else {
            // If we have lost our spot in the queue, then we abort and don't set the OS clipboard
            //     with our buffer
            aborting = true;
        }
    }

    whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // If the current action has completed and we have not aborted our set, then we can safely
    //     push the buffer onto the OS clipboard
    if (complete && !aborting) {
        set_os_clipboard(clipboard_buffer);

        whist_lock_mutex(current_clipboard_activity.clipboard_action_mutex);
        setting_os_clipboard = false;
        // Wake up waiting threads so that they know the OS clipboard is available for setting again
        whist_broadcast_cond(os_clipboard_setting_condvar);
        whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
    }

    // Free the allocated clipboard buffer
    deallocate_region(clipboard_buffer);

    atomic_store(&thread->finished, 1);

    return 0;
}

int pull_clipboard_thread_function(void* opaque) {
    /*
        Ephemeral update thread function for a clipboard pull. This
        thread runs as long as an active clipboard pull action is occurring.
        Once all chunks have been handled, or the action has been aborted, the
        loop exits and the thread finishes by cleaning up resources.

        Arguments:
            opaque (void*): ClipboardThread reference

        Return:
            (int): 0 on success

        NOTE: thread must be created with `current_clipboard_activity.clipboard_action_mutex` held.
            `current_clipboard_activity.clipboard_action_mutex` is safe to release when thread posts
            `current_clipboard_activity.thread_setup_semaphore`.
    */

    LOG_INFO("Begun pulling clipboard");

    ClipboardThread* thread = opaque;

    // Set thread status members
    bool aborting = false;
    bool complete = false;

    current_clipboard_activity.aborting_ptr = &aborting;
    current_clipboard_activity.complete_ptr = &complete;
    current_clipboard_activity.clipboard_action_type = CLIPBOARD_ACTION_PULL;

    // Let the pull thread know that it is safe to continue while the
    //     clipboard is being pulled
    whist_post_semaphore(current_clipboard_activity.thread_setup_semaphore);

    // When thread is created, pull the clipboard
    // We don't have to worry about race conditions here. If two pull threads
    //     are created in rapid succession, the first one will get aborted
    //     because we have set `current_clipboard_activity.aborting_ptr` before
    //     posting `current_clipboard_activity.thread_setup_semaphore`.
    ClipboardData* clipboard_buffer = get_os_clipboard();

    whist_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // Between the clipboard get and the lock mutex, this action may have been cancelled,
    //     so we have to check before setting the buffer and blocking on the condvar
    if (!complete && !aborting) {
        current_clipboard_activity.clipboard_buffer_ptr = &clipboard_buffer;

        // When this condition is signaled, we can continue this thread.
        //     This condition is signaled in one of two cases:
        //     1) the current action has completed
        //     2) the current action has been aborted
        // This unlocks the mutex and blocks until signaled
        whist_wait_cond(current_clipboard_activity.continue_action_condvar,
                        current_clipboard_activity.clipboard_action_mutex);
    }

    whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // After calling `get_os_clipboard()`, we call `free_clipboard_buffer()`
    free_clipboard_buffer(clipboard_buffer);

    atomic_store(&thread->finished, 1);

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

    current_clipboard_activity.clipboard_action_mutex = whist_create_mutex();
    current_clipboard_activity.continue_action_condvar = whist_create_cond();
    current_clipboard_activity.thread_setup_semaphore = whist_create_semaphore(0);

    setting_os_clipboard = false;
    queued_os_clipboard_setter_thread_id = 0;
    os_clipboard_setting_condvar = whist_create_cond();

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

    whist_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    LOG_INFO("Pushing clipboard chunk of size %d", cb_chunk->size);

    // If received start chunk, then start new transfer
    if (cb_chunk->chunk_type == CLIPBOARD_START) {
        if (!start_clipboard_transfer(CLIPBOARD_ACTION_PUSH)) {
            LOG_ERROR("Failed to start a new clipboard push action");
            whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
            return;
        }
    }

    // If we're not actively pushing to clipboard, return immediately
    // This also takes care of any state changes that may have occurred between the
    //     `whist_wait_semaphore` and `whist_lock_mutex` after a successful
    //     `start_clipboard_transfer`
    if (!clipboard_action_is_active(CLIPBOARD_ACTION_PUSH)) {
        whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return;
    }

    // Add chunk contents onto clipboard buffer and update clipboard size
    *current_clipboard_activity.clipboard_buffer_ptr = realloc_region(
        (*current_clipboard_activity.clipboard_buffer_ptr),
        sizeof(ClipboardData) + (*current_clipboard_activity.clipboard_buffer_ptr)->size +
            cb_chunk->size);

    if (cb_chunk->chunk_type == CLIPBOARD_START) {
        // First chunk needs to memcpy full contents into clipboard buffer because it contains
        //     clipboard detail fields that need to be copied (e.g. size, type)
        memcpy(*current_clipboard_activity.clipboard_buffer_ptr, cb_chunk,
               sizeof(ClipboardData) + cb_chunk->size);
    } else {
        // Remaining chunks need to memcpy just the data buffer into clipboard buffer and also
        //     need to manually increment clipboard size
        memcpy((*current_clipboard_activity.clipboard_buffer_ptr)->data +
                   (*current_clipboard_activity.clipboard_buffer_ptr)->size,
               cb_chunk->data, cb_chunk->size);
        (*current_clipboard_activity.clipboard_buffer_ptr)->size += cb_chunk->size;
    }

    if (cb_chunk->chunk_type == CLIPBOARD_FINAL) {
        // ready to set OS clipboard
        LOG_INFO("Finishing clipboard push");
        finish_active_transfer(true);
    }

    whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
}

ClipboardData* pull_clipboard_chunk() {
    /*
        When called, return the current clipboard chunk if a new clipboard activity
        has registered, or if the recently updated clipboard is being read.

        Returns:
            cb_chunk (ClipboardData*): pointer to the latest clipboard chunk
                NOTE: cb_chunk must be freed by caller if not NULL

        NOTE: this should typically be called in a loop of `while not NULL`,
              or similar to get all the chunks in the OS clipboard
    */

    if (!current_clipboard_activity.is_initialized) {
        LOG_WARNING("Tried to pull_clipboard_chunk, but the clipboard is not initialized");
        return NULL;
    }

    // A bit unfortunate that we have to lock this mutex with every call to `pull_clipboard_chunk`,
    //     especially since not every call to this function yields a chunk and it is called
    //     repeatedly in a loop, but it's the best we can do for now.
    whist_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    // If clipboard has updated, start new transfer
    if (has_os_clipboard_updated()) {
        start_clipboard_transfer(CLIPBOARD_ACTION_PULL);
        whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return NULL;
    }

    // If we're not actively pulling from clipboard, then return immediately
    if (!clipboard_action_is_active(CLIPBOARD_ACTION_PULL)) {
        whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
        return NULL;
    }

    LOG_INFO("Pulling next clipboard chunk");

    int chunk_size = min(CHUNK_SIZE, (*current_clipboard_activity.clipboard_buffer_ptr)->size -
                                         current_clipboard_activity.pulled_bytes);
    ClipboardData* cb_chunk = allocate_region(sizeof(ClipboardData) + chunk_size);
    cb_chunk->size = chunk_size;
    cb_chunk->type = (*current_clipboard_activity.clipboard_buffer_ptr)->type;

    // for FINAL, this will just "copy" 0 bytes
    memcpy(cb_chunk->data,
           (*current_clipboard_activity.clipboard_buffer_ptr)->data +
               current_clipboard_activity.pulled_bytes,
           chunk_size);

    // based on how many bytes have been written so far, set the chunk type
    if (current_clipboard_activity.pulled_bytes == 0) {
        cb_chunk->chunk_type = CLIPBOARD_START;
    } else if (current_clipboard_activity.pulled_bytes ==
               (*current_clipboard_activity.clipboard_buffer_ptr)->size) {
        cb_chunk->chunk_type = CLIPBOARD_FINAL;
        LOG_INFO("Finished pulling chunks from OS clipboard");
        finish_active_transfer(true);
    } else {
        cb_chunk->chunk_type = CLIPBOARD_MIDDLE;
    }

    // update pulled bytes
    current_clipboard_activity.pulled_bytes += chunk_size;

    whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);
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

    whist_lock_mutex(current_clipboard_activity.clipboard_action_mutex);

    current_clipboard_activity.is_initialized = false;
    finish_active_transfer(false);

    whist_unlock_mutex(current_clipboard_activity.clipboard_action_mutex);

    join_outstanding_threads(true);

    whist_destroy_mutex(current_clipboard_activity.clipboard_action_mutex);
    whist_destroy_cond(current_clipboard_activity.continue_action_condvar);
    whist_destroy_semaphore(current_clipboard_activity.thread_setup_semaphore);

    whist_destroy_cond(os_clipboard_setting_condvar);

    // NOTE: Bad things could happen if initialize_clipboard is run
    // while destroy_clipboard() is running
    destroy_clipboard();

    LOG_INFO("Finished destroying clipboard synchronizer");
}
