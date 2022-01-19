#ifndef CLIPBOARD_SYNCHRONIZER_H
#define CLIPBOARD_SYNCHRONIZER_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file clipboard_synchronizer.h
 * @brief This file contains code meant, to be used on the clientside, that will
 *        assist in synchronizing the client-server clipboard.
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

#include "clipboard.h"

typedef enum WhistClipboardActionType {
    CLIPBOARD_ACTION_NONE = 0,
    CLIPBOARD_ACTION_PUSH = 1,  // push onto local clipboard
    CLIPBOARD_ACTION_PULL = 2,  // pull from local clipboard
} WhistClipboardActionType;

typedef struct ClipboardActivity {
    // is_initialized will only go from true to false when
    //    there are active actions, so no mutex needed to
    //    protect
    bool is_initialized;

    // Protected by clipboard_action_mutex:
    WhistClipboardActionType clipboard_action_type;
    WhistThread active_clipboard_action_thread;
    bool* aborting_ptr;  // whether the current thread is being aborted
    bool* complete_ptr;  // whether the action has been completed
    ClipboardData** clipboard_buffer_ptr;
    int pulled_bytes;  // only relevant for PULL actions
    bool is_start_done;

    WhistMutex clipboard_action_mutex;
    WhistCondition continue_action_condvar;
    WhistSemaphore thread_setup_semaphore;
} ClipboardActivity;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the clipboard synchronizer
 *
 * @param is_client                Whether the caller is the client or the server
 *
 */
void init_clipboard_synchronizer(bool is_client);

/**
 * @brief                          When called, return the current clipboard chunk
 *                                 if a new clipboard activity has registered, or
 *                                 if the recently updated clipboard is being read.
 *
 * @returns                        Pointer to the latest clipboard chunk
 *
 */
ClipboardData* pull_clipboard_chunk();

/**
 * @brief                          When called, push the given chunk onto the active
 *                                 clipboard buffer
 *
 * @param cb_chunk                 Pointer to the clipboard chunk to push
 *
 */
void push_clipboard_chunk(ClipboardData* cb_chunk);

/**
 * @brief                          Cleanup the clipboard synchronizer
 */
void destroy_clipboard_synchronizer();

#endif  // CLIPBOARD_SYNCHRONIZER_H