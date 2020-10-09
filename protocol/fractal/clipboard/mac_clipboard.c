/**
 * Copyright Fractal Computers, Inc. 2020
 * @file mac_clipboard.c
 * @brief This file contains the general clipboard functions for a shared
 *        client-server clipboard on MacOS clients.
============================
Usage
============================

GET_CLIPBOARD and SET_CLIPBOARD will return strings representing directories
important for getting and setting file clipboards. When GetClipboard() is called
and it returns a CLIPBOARD_FILES type, then GET_CLIPBOARD will be filled with
symlinks to the clipboard files. When SetClipboard(cb) is called and is given a
clipboard with a CLIPBOARD_FILES type, then the clipboard will be set to
whatever files are in the SET_CLIPBOARD directory.
*/

#include "../core/fractal.h"
#include "clipboard.h"

bool StartTrackingClipboardUpdates();

void unsafe_initClipboard() { StartTrackingClipboardUpdates(); }

#include <sys/syslimits.h>
#include <unistd.h>

#include "../utils/mac_utils.h"
#include "clipboard_osx.h"

bool clipboardHasImage;
bool clipboardHasString;
bool clipboardHasFiles;
static int last_clipboard_sequence_number = -1;

static char clipboard_buf[9000000];

bool StartTrackingClipboardUpdates() {
    last_clipboard_sequence_number = GetClipboardChangecount();  // to capture the first event
    clipboardHasImage = false;
    clipboardHasString = false;
    clipboardHasFiles = false;
    return true;
}

bool unsafe_hasClipboardUpdated() {
    bool hasUpdated = false;

    int new_clipboard_sequence_number = GetClipboardChangecount();
    if (new_clipboard_sequence_number > last_clipboard_sequence_number) {
        // check if new clipboard is an image or a string
        clipboardHasImage = ClipboardHasImage();
        clipboardHasString = ClipboardHasString();
        clipboardHasFiles = ClipboardHasFiles();
        hasUpdated = (clipboardHasImage || clipboardHasString ||
                      clipboardHasFiles);  // should be always set to true in here
        LOG_INFO("has clipboard updated - image: %d string: %d files: %d", clipboardHasImage, clipboardHasString, clipboardHasFiles);
        last_clipboard_sequence_number = new_clipboard_sequence_number;
    }
    return hasUpdated;
}

ClipboardData* unsafe_GetClipboard() {
    ClipboardData* cb = (ClipboardData*)clipboard_buf;

    cb->size = 0;
    cb->type = CLIPBOARD_NONE;

    // do clipboardHasFiles check first because it is more restrictive
    // otherwise gets multiple files confused with string and thinks both are
    // true
    if (clipboardHasFiles) {
        LOG_INFO("Getting files from clipboard");

        // allocate memory for filenames and paths
        OSXFilenames* filenames[MAX_URLS];
        for (size_t i = 0; i < MAX_URLS; i++) {
            filenames[i] = (OSXFilenames*)malloc(sizeof(OSXFilenames));
            filenames[i]->filename = (char*)malloc(PATH_MAX * sizeof(char));
            filenames[i]->fullPath = (char*)malloc(PATH_MAX * sizeof(char));

            // pad with null terminators
            memset(filenames[i]->filename, '\0', PATH_MAX * sizeof(char));
            memset(filenames[i]->fullPath, '\0', PATH_MAX * sizeof(char));
        }

        // populate filenames array with file URLs
        ClipboardGetFiles(filenames);

        // set clipboard data attributes to be returned
        cb->type = CLIPBOARD_FILES;
        cb->size = 0;

        // delete clipboard directory and all its files
        if (dir_exists(GET_CLIPBOARD) > 0) {
            mac_rm_rf(GET_CLIPBOARD);
        }

        // make new clipboard directory
        mkdir(GET_CLIPBOARD, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        // make symlinks for all files in clipboard and store in directory
        for (size_t i = 0; i < MAX_URLS; i++) {
            // if there are no more file URLs in clipboard, exit loop
            if (*filenames[i]->fullPath != '\0') {
                char symlinkName[PATH_MAX] = "";
                strcat(symlinkName, GET_CLIPBOARD);
                strcat(symlinkName, "/");
                strcat(symlinkName, filenames[i]->filename);
                symlink(filenames[i]->fullPath, symlinkName);
            } else {
                break;
            }
        }

        // free heap memory
        for (size_t i = 0; i < MAX_URLS; i++) {
            free(filenames[i]->filename);
            free(filenames[i]->fullPath);
            free(filenames[i]);
        }

    } else if (clipboardHasString) {
        // get the string
        const char* clipboard_string = ClipboardGetString();
        // int data_size = strlen(clipboard_string) + 1;  // for null terminator
        int data_size = strlen(clipboard_string);
        // copy the data
        if ((unsigned long)data_size < sizeof(clipboard_buf)) {
            cb->size = data_size;
            memcpy(cb->data, clipboard_string, data_size);
            cb->type = CLIPBOARD_TEXT;
            LOG_INFO("CLIPBOARD STRING: %s", cb->data);
            LOG_INFO("Len %d, Strlen %d", cb->size, strlen(cb->data));
        } else {
            LOG_WARNING("Could not copy, clipboard too large! %d bytes", data_size);
        }
    } else if (clipboardHasImage) {
        // malloc some space for the image
        OSXImage* clipboard_image = (OSXImage*)malloc(sizeof(OSXImage));
        memset(clipboard_image, 0, sizeof(OSXImage));

        // get the image
        ClipboardGetImage(clipboard_image);

        // copy the data
        if ((unsigned long)clipboard_image->size < sizeof(clipboard_buf)) {
            cb->size = clipboard_image->size;
            memcpy(cb->data, clipboard_image->data, clipboard_image->size);
            // dimensions for sanity check
            // now that the image is in Clipboard struct, we can free this
            // struct
            free(clipboard_image);
        } else {
            LOG_WARNING("Could not copy, clipboard too large! %d bytes", cb->size);
        }
    } else {
        LOG_INFO("Nothing in the clipboard!");
    }

    return cb;
}

void unsafe_SetClipboard(ClipboardData* cb) {
    if (cb->type == CLIPBOARD_NONE) {
        return;
    }
    // check the type of the data
    // Because we are declaring variables within each `case`, make sure each one is
    //  scoped properly using brackets:
    //  https://stackoverflow.com/questions/92396/why-cant-variables-be-declared-in-a-switch-statement
    switch (cb->type) {
        case CLIPBOARD_TEXT: {
            // Since Objective-C clipboard string pasting does not take
            //   string size as an argument, and pastes until null character,
            //   must malloc string to end with null character to be pasted.
            //   This means null characters cannot be within the string being
            //   pasted.
            char* string_data = malloc(cb->size + 1);
            memset(string_data, 0, cb->size + 1);
            memcpy(string_data, cb->data, cb->size);
            LOG_INFO("SetClipboard to Text: %s", string_data);
            ClipboardSetString(string_data);
            free(string_data);
            break;
        }
        case CLIPBOARD_IMAGE: {
            LOG_INFO("SetClipboard to Image with size %d", cb->size);
            ClipboardSetImage(cb->data, cb->size);
            break;
        }
        case CLIPBOARD_FILES: {
            LOG_INFO("SetClipboard to Files");

            // allocate memory to store filenames in clipboard
            char* filenames[MAX_URLS];
            for (size_t i = 0; i < MAX_URLS; i++) {
                filenames[i] = (char*)malloc(PATH_MAX * sizeof(char));
                memset(filenames[i], '\0', PATH_MAX * sizeof(char));
            }

            // populate filenames
            get_filenames(SET_CLIPBOARD, filenames);

            // add files to clipboard
            ClipboardSetFiles(filenames);

            // free memory
            for (size_t i = 0; i < MAX_URLS; i++) {
                free(filenames[i]);
            }

            break;
        }
        default: {
            LOG_INFO("No clipboard data to set!");
            break;
        }
    }

    // Update the status so that this specific update doesn't count
    unsafe_hasClipboardUpdated();
}
