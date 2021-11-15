#ifndef CLIPBOARD_SYNCHRONIZER_H
#define CLIPBOARD_SYNCHRONIZER_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file clipboard_synchronizer.h
 * @brief This file contains code meant, to be used on the clientside, that will
 *        assist in synchronizing the client-server clipboard.
============================
Usage
============================

init_clipboard_synchronizer("76.106.92.11");

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

typedef enum FractalClipboardActionType {
    CLIPBOARD_ACTION_NONE = 0,
    CLIPBOARD_ACTION_PUSH = 1,  // push onto local clipboard
    CLIPBOARD_ACTION_PULL = 2,  // pull from local clipboard
} FractalClipboardActionType;

typedef struct ClipboardActivity {
    bool is_initialized;
    // active_clipboard_action_thread is valid as long as
    //    clipboard_action_type is not CLIPBOARD_ACTION_NONE
    // (FractalThread can also evaluate to NULL)
    FractalThread active_clipboard_action_thread;
    FractalClipboardActionType clipboard_action_type;
    FractalMutex clipboard_action_mutex;
    FractalSemaphore chunk_transfer_semaphore;
    FractalSemaphore transfer_thread_setup_semaphore;
    bool action_completed;  // whether the push to clipboard or pull of all chunks is completed
} ClipboardActivity;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the clipboard synchronizer
 *
 * @returns                        True if clipboard is synchronizing, false otherwise
 *
 */
bool is_clipboard_synchronizing();

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
 * @brief                          When called, return the current clipboard chunk
 *                                 if a new clipboard activity has registered, or
 *                                 if the recently updated clipboard is being read.
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
