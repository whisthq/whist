#ifndef CLIPBOARD_SYNCHRONIZER_H
#define CLIPBOARD_SYNCHRONIZER_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clipboard_synchronizer.h
 * @brief This file contains code meant, to be used on the clientside, that will assist in 
 *        synchronizing the client-server clipboard.
============================
Usage
============================




initClipboardSynchronizer("76.106.92.11");

ClipboardData server_clipboard;

ClipboardSynchronizerSetClipboard(&server_clipboard);           // Will set the
client clipboard to that data

mprintf("Is Synchronizing? %d\n", isClipboardSynchronizing());  // Will likely
return true because it's waiting on server_clipboard to be set

while(isClipboardSynchronizing());                              // Wait for
clipboard to be done synchronizing

ClipboardData* client_clipboard = ClipboardSynchronizerGetNewClipboard();

if ( client_clipboard ) {
  // We have a new clipboard, this should be sent to the server
  Send(client_clipboard);
} else {
  // There is no new clipboard
}

destroyClipboardSynchronizer();
*/





#include "clipboard.h"

/**
@brief                          Initialize the Update Clipboard Helper

@param server_ip				The IP of the server to
synchronize the clipboard with
*/
void initClipboardSynchronizer(char* server_ip);

/**
@brief                          Set the clipboard to a given clipboard data

@param clipboard				The clipboard data to update the
clipboard with

@returns						true on success, false
on failure
*/
bool ClipboardSynchronizerSetClipboard(ClipboardData* clipboard);

/**
@brief                          Check if the clipboard is in the midst of being
updated

@returns						True if the clipboard is
currently busy being updated. This will be true for a some period of time after
updateSetClipboard
*/
bool isClipboardSynchronizing();

/**
@brief                          Get a new clipboard, if any

@returns						A pointer to new
clipboard data that should be sent to the server. NULL if the clipboard has not
been updated since the last call to the clipboard
*/
ClipboardData* ClipboardSynchronizerGetNewClipboard();

/**
@brief                          Cleanup the clipboard synchronizer
*/
void destroyClipboardSynchronizer();

#endif  // CLIPBOARD_SYNCHRONIZER_H
