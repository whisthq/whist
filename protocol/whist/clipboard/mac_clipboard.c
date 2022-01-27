/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file mac_clipboard.c
 * @brief This file contains the general clipboard functions for a shared
 *        client-server clipboard on MacOS clients.
============================
Usage
============================

GET_OS_CLIPBOARD and SET_OS_CLIPBOARD will return strings representing directories
important for getting and setting file clipboards. When get_os_clipboard() is called
and it returns a CLIPBOARD_FILES type, then GET_OS_CLIPBOARD will be filled with
symlinks to the clipboard files. When set_os_clipboard(cb) is called and is given a
clipboard with a CLIPBOARD_FILES type, then the clipboard will be set to
whatever files are in the SET_OS_CLIPBOARD directory.
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include "clipboard.h"

#include <sys/syslimits.h>
#include <unistd.h>

#include "../utils/mac_utils.h"
#include "clipboard_osx.h"

static bool clipboard_has_image = false;
static bool clipboard_has_string = false;
static bool clipboard_has_files = false;
static int last_clipboard_sequence_number = -1;

/*
============================
Private Function Implementations
============================
*/

void unsafe_init_clipboard(void){};

void unsafe_destroy_clipboard(void){};

bool unsafe_has_os_clipboard_updated(void) {
    /*
        Check if the Mac clipboard has updated

        Returns:
            (bool): true if the clipboard has updated since the last
                call to this function, else false
    */

    bool has_updated = false;

    int new_clipboard_sequence_number = get_clipboard_changecount();

    // Do not update last_clipboard_sequence_number until we have parsed the clipboard after the
    // clipboard has data ready. Otherwise this condition will be erroneously affected.
    if (new_clipboard_sequence_number > last_clipboard_sequence_number) {
        // If latest clipboard changed ownership but there's no items on it yet return false
        if (!check_clipboard_has_data_ready()) {
            return false;
        }

        // Check if new clipboard is an image/string/file after clipboard has been populated
        clipboard_has_image = check_clipboard_has_image();
        clipboard_has_string = check_clipboard_has_string();
        clipboard_has_files = check_clipboard_has_files();

        if (should_preserve_local_clipboard()) {
            has_updated = (clipboard_has_image || clipboard_has_string ||
                           clipboard_has_files);  // should be always set to true in here
        }

        // Only update last_clipboard_sequence_number if we have parsed the clipboard!
        last_clipboard_sequence_number = new_clipboard_sequence_number;
    }

    return has_updated;
}

void unsafe_free_clipboard_buffer(ClipboardData* cb) {
    /*
        Free the clipboard buffer memory

        Arguments:
            cb (ClipboardData*): the clipboard buffer to be freed
    */

    deallocate_region(cb);
}

ClipboardData* unsafe_get_os_clipboard(void) {
    /*
        Get and return the current contents of the Mac clipboard

        Returns:
            (ClipboardData*): the clipboard data that has been read
                from the Mac OS clipboard
    */

    ClipboardData* cb = allocate_region(sizeof(ClipboardData));

    cb->size = 0;
    cb->type = CLIPBOARD_NONE;

    // do clipboardHasFiles check first because it is more restrictive
    // otherwise gets multiple files confused with string and thinks both are
    // true
    if (clipboard_has_files) {
        LOG_INFO("Getting files from clipboard");
        LOG_WARNING("get_os_clipboard: FILE CLIPBOARD NOT BEING IMPLEMENTED");
        return cb;

        // allocate memory for filenames and paths
        OSXFilenames* filenames[MAX_URLS];
        for (size_t i = 0; i < MAX_URLS; i++) {
            filenames[i] = (OSXFilenames*)safe_malloc(sizeof(OSXFilenames));
            filenames[i]->filename = (char*)safe_malloc((PATH_MAXLEN + 1) * sizeof(char));
            filenames[i]->fullPath = (char*)safe_malloc((PATH_MAXLEN + 1) * sizeof(char));

            // pad with null terminators
            memset(filenames[i]->filename, '\0', (PATH_MAXLEN + 1) * sizeof(char));
            memset(filenames[i]->fullPath, '\0', (PATH_MAXLEN + 1) * sizeof(char));
        }

        // populate filenames array with file URLs
        clipboard_get_files(filenames);

        // Set clipboard data attributes to be returned
        cb->type = CLIPBOARD_FILES;
        cb->size = 0;

        // delete clipboard directory and all its files
        if (dir_exists(GET_OS_CLIPBOARD) > 0) {
            mac_rm_rf(GET_OS_CLIPBOARD);
        }

        // make new clipboard directory
        mkdir(GET_OS_CLIPBOARD, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        // make symlinks for all files in clipboard and store in directory
        for (size_t i = 0; i < MAX_URLS; i++) {
            // if there are no more file URLs in clipboard, exit loop
            if (*filenames[i]->fullPath != '\0') {
                char symlink_name[PATH_MAXLEN + 1] = "";
                strcat(symlink_name, GET_OS_CLIPBOARD);
                strcat(symlink_name, "/");
                strcat(symlink_name, filenames[i]->filename);
                symlink(filenames[i]->fullPath, symlink_name);
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
    } else if (clipboard_has_string) {
        const char* clipboard_string = clipboard_get_string();
        int data_size = strlen(clipboard_string);
        cb = realloc_region(cb, sizeof(ClipboardData) + data_size);
        // Copy the text data
        cb->type = CLIPBOARD_TEXT;
        cb->size = data_size;
        memcpy(cb->data, clipboard_string, data_size);
    } else if (clipboard_has_image) {
        // safe_malloc some space for the image struct
        OSXImage* clipboard_image = (OSXImage*)safe_malloc(sizeof(OSXImage));
        memset(clipboard_image, 0, sizeof(OSXImage));

        // get the image
        clipboard_get_image(clipboard_image);

        // Allocate new cb struct
        deallocate_region(cb);
        cb = allocate_region(sizeof(ClipboardData) + clipboard_image->size);
        // Copy cb over
        cb->type = CLIPBOARD_IMAGE;
        cb->size = clipboard_image->size;
        memcpy(cb->data, clipboard_image->data, clipboard_image->size);
        // Now that the image is in Clipboard struct, we can free
        // the OSXImage struct
        free(clipboard_image);
    } else {
        LOG_INFO("Nothing in the clipboard!");
    }

    return cb;
}

void unsafe_set_os_clipboard(ClipboardData* cb) {
    /*
        Set the Mac OS clipboard to contain the data from `cb`

        Arguments:
            cb (ClipboardData*): the clipboard data to load into
                the Mac OS clipboard
    */

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
            //   must safe_malloc string to end with null character to be pasted.
            //   This means null characters cannot be within the string being
            //   pasted.
            char* string_data = safe_malloc(cb->size + 1);
            memset(string_data, 0, cb->size + 1);
            memcpy(string_data, cb->data, cb->size);
            LOG_INFO("SetClipboard to Text with size %d", cb->size);
            clipboard_set_string(string_data);
            free(string_data);
            break;
        }
        case CLIPBOARD_IMAGE: {
            LOG_INFO("SetClipboard to Image with size %d", cb->size);
            clipboard_set_image(cb->data, cb->size);
            break;
        }
        case CLIPBOARD_FILES: {
            LOG_INFO("SetClipboard to Files");
            LOG_WARNING("SetClipboard: FILE CLIPBOARD NOT BEING IMPLEMENTED");
            return;
        }
        default: {
            LOG_INFO("No clipboard data to set!");
            break;
        }
    }
}
