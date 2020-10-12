/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clipboard_synchronizer.c
 * @brief This file contains code meant, to be used on the clientside, that will
 *        assist in synchronizing the client-server clipboard.
============================
Usage
============================

initClipboardSynchronizer("76.106.92.11");

ClipboardData server_clipboard;

// Will set the client clipboard to that data
ClipboardSynchronizerSetClipboard(&server_clipboard);

// Will likely return true because it's waiting on server_clipboard to be set
mprintf("Is Synchronizing? %d\n", isClipboardSynchronizing());

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

#include <stdio.h>

#include "../core/fractal.h"
#include "clipboard.h"

#define UNUSED(x) (void)(x)
#define MS_IN_SECOND 1000

int UpdateClipboardThread(void* opaque);
bool pendingUpdateClipboard();

extern char filename[300];
extern char username[50];
bool updating_set_clipboard;
bool updating_get_clipboard;
bool updating_clipboard;
bool pending_update_clipboard;
clock last_clipboard_update;
SDL_sem* clipboard_semaphore;
ClipboardData* clipboard;
SDL_Thread* thread;
static bool connected;

bool pending_clipboard_push;

bool isClipboardSynchronizing() { return updating_clipboard; }

bool pendingUpdateClipboard() { return pending_update_clipboard; }

void initClipboardSynchronizer() {
    initClipboard();

    connected = true;

    pending_clipboard_push = false;
    updating_clipboard = false;
    pending_update_clipboard = false;
    StartTimer((clock*)&last_clipboard_update);
    clipboard_semaphore = SDL_CreateSemaphore(0);

    thread = SDL_CreateThread(UpdateClipboardThread, "UpdateClipboardThread", NULL);

    pending_update_clipboard = true;
}

void destroyClipboardSynchronizer() {
    connected = false;
    SDL_SemPost(clipboard_semaphore);
}

bool ClipboardSynchronizerSetClipboard(ClipboardData* cb) {
    if (updating_clipboard) {
        LOG_INFO("Tried to SetClipboard, but clipboard is updating");
        return false;
    }

    updating_clipboard = true;
    updating_set_clipboard = true;
    updating_get_clipboard = false;
    clipboard = cb;

    SDL_SemPost(clipboard_semaphore);

    return true;
}

ClipboardData* ClipboardSynchronizerGetNewClipboard() {
    if (pending_clipboard_push) {
        pending_clipboard_push = false;
        return clipboard;
    }

    // If the clipboard has updated since we last checked, or a previous
    // clipboard update is still pending, then we try to update the clipboard
    if (hasClipboardUpdated() || pendingUpdateClipboard()) {
        if (updating_clipboard) {
            // Clipboard is busy, to set pending update clipboard to true to
            // make sure we keep checking the clipboard state
            pending_update_clipboard = true;
        } else {
            LOG_INFO("Pushing update to clipboard");
            // Clipboard is no longer pending
            pending_update_clipboard = false;
            updating_clipboard = true;
            updating_set_clipboard = false;
            updating_get_clipboard = true;
            SDL_SemPost(clipboard_semaphore);
        }
    }

    return NULL;
}

int UpdateClipboardThread(void* opaque) {
    UNUSED(opaque);

    while (connected) {
        SDL_SemWait(clipboard_semaphore);

        if (!connected) {
            break;
        }

        if (updating_set_clipboard) {
            LOG_INFO("Trying to set clipboard!");
            SetClipboard(clipboard);
            updating_set_clipboard = false;
        } else if (updating_get_clipboard) {
            LOG_INFO("Trying to get clipboard!");

            clipboard = GetClipboard();
            pending_clipboard_push = true;
            updating_get_clipboard = false;
        } else {
            clock clipboard_time;
            StartTimer(&clipboard_time);

            // If it hasn't been 500ms yet, then wait 500ms to prevent too much
            // spam
            const int spam_time_ms = 500;
            if (GetTimer(clipboard_time) < spam_time_ms / (double)MS_IN_SECOND) {
                SDL_Delay(max((int)(spam_time_ms - MS_IN_SECOND * GetTimer(clipboard_time)), 1));
            }
        }

        LOG_INFO("Updated clipboard!");
        updating_clipboard = false;
    }

    return 0;
}
