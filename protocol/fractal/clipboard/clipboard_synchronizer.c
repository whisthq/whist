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
#define MS_IN_SECOND 1000.0

int UpdateClipboardThread(void* opaque);
bool pendingUpdateClipboard();

extern char filename[300];
extern char username[50];
bool updating_set_clipboard;
bool updating_clipboard;
bool pending_update_clipboard;
clock last_clipboard_update;
SDL_sem* clipboard_semaphore;
ClipboardData* clipboard;
SDL_Thread* thread;
static bool connected;
static char* server_ip;

bool pending_clipboard_push;

bool isClipboardSynchronizing() { return updating_clipboard; }

bool pendingUpdateClipboard() { return pending_update_clipboard; }

void initClipboardSynchronizer(char* server_ip_local) {
    initClipboard();

    connected = true;

    server_ip = server_ip_local;

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
    clipboard = cb;

    SDL_SemPost(clipboard_semaphore);

    return true;
}

int UpdateClipboardThread(void* opaque) {
    UNUSED(opaque);

    while (connected) {
        SDL_SemWait(clipboard_semaphore);

        if (!connected) {
            break;
        }

        // ClipboardData* clipboard = GetClipboard();

        if (updating_set_clipboard) {
            LOG_INFO("Trying to set clipboard!");
            // ClipboardData cb; TODO: unused, still needed?
            if (clipboard->type == CLIPBOARD_FILES) {
                char cmd[1000] = "";

#ifndef _WIN32
                char* prefix = "UNISON=./.unison;";
#else
                char* prefix = "";
#endif

#ifdef _WIN32
                char* exc = "unison";
#elif __APPLE__
                char* exc = "./mac_unison";
#else  // Linux
                char* exc = "./linux_unison";
#endif

                snprintf(
                    cmd, sizeof(cmd),
                    "%s %s -follow \"Path *\" -ui text -ignorearchives -confirmbigdel=false -batch \
						 -sshargs \"-o UserKnownHostsFile=ssh_host_ecdsa_key.pub -l %s -i sshkey\" \
                         \"ssh://%s/%s/get_clipboard/\" \
                         %s \
                         -force \"ssh://%s/%s/get_clipboard/\"",
                    prefix, exc, username, server_ip, filename, SET_CLIPBOARD, server_ip, filename);

                LOG_INFO("COMMAND: %s", cmd);
                runcmd(cmd, NULL);
            }
            SetClipboard(clipboard);
        } else {
            clock clipboard_time;
            StartTimer(&clipboard_time);

            if (clipboard->type == CLIPBOARD_FILES) {
                char cmd[1000] = "";

#ifndef _WIN32
                char* prefix = "UNISON=./.unison;";
#else
                char* prefix = "";
#endif

#ifdef _WIN32
                char* exc = "unison";
#elif __APPLE__
                char* exc = "./mac_unison";
#else  // Linux
                char* exc = "./linux_unison";
#endif

                snprintf(
                    cmd, sizeof(cmd),
                    "%s %s -follow \"Path *\" -ui text -ignorearchives -confirmbigdel=false -batch \
						 -sshargs \"-o UserKnownHostsFile=ssh_host_ecdsa_key.pub -l %s -i sshkey\" \
                         %s \
                         \"ssh://%s/%s/set_clipboard/\" \
                         -force %s",
                    prefix, exc, username, GET_CLIPBOARD, server_ip, filename, GET_CLIPBOARD);

                LOG_INFO("COMMAND: %s", cmd);
                runcmd(cmd, NULL);
            }

            pending_clipboard_push = true;
            /*
            FractalClientMessage* fmsg =
                malloc(sizeof(FractalClientMessage) + sizeof(ClipboardData) +
                       clipboard->size);
            fmsg->type = CMESSAGE_CLIPBOARD;
            memcpy(&fmsg->clipboard, clipboard,
                   sizeof(ClipboardData) + clipboard->size);
            clipboard_fmsg = fmsg;
            */

            // If it hasn't been 500ms yet, then wait 500ms to prevent too much
            // spam
            const int spam_time_ms = 500;
            if (GetTimer(clipboard_time) < spam_time_ms / MS_IN_SECOND) {
                SDL_Delay(max((int)(spam_time_ms - 1000 * GetTimer(clipboard_time)), 1));
            }
        }

        LOG_INFO("Updated clipboard!");
        updating_clipboard = false;
    }

    return 0;
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
            clipboard = GetClipboard();
            SDL_SemPost(clipboard_semaphore);
        }
    }

    return NULL;
}
