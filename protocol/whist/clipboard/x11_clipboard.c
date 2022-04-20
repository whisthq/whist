/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file x11_clipboard.c
 * @brief This file contains the general clipboard functions for a shared
 *        client-server clipboard on Linux clients/servers.
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

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <whist/core/whist.h>
#include "clipboard.h"
#include "clipboard_internal.h"
#include "../utils/png.h"

/* #define CLOSE_FDS \
     "for fd in $(ls /proc/$$/fd); do\
   case \"$fd\" in 0|1|2|255)\
       ;;\
     *)\
       eval \"exec $fd>&-\"\
       ;;\
   esac \
 done; "
 */

static Display* display = NULL;
static Window window;
static bool xfixes_available;
static int event_base, error_base;

static Atom clipboard;
static Atom incr_id;

// Empty clipboard object for use when nothing else is available.
static ClipboardData clipboard_none = {
    .type = CLIPBOARD_NONE,
    .size = 0,
};

/*
============================
Private Functions
============================
*/

static bool start_tracking_clipboard_updates(void);
static bool clipboard_has_target(Atom property_atom, Atom target_atom);
static ClipboardData* get_os_clipboard_data(Atom property_atom, int header_size);

/*
============================
Private Function Implementations
============================
*/

void unsafe_init_clipboard(void) {
    /*
        Initialize the clipboard by starting tracking clipboard updates
    */

    start_tracking_clipboard_updates();
}

void unsafe_destroy_clipboard(void) {
    /*
        Destroy clipboard by closing associated X display
    */

    if (display) {
        XCloseDisplay(display);
    }
}

static ClipboardData* get_os_clipboard_picture(void) {
    /*
        Get an image from the Linux OS clipboard
        NOTE: Assume that clipboard stores pictures in png format when getting

        Returns:
            (ClipboardData*): clipboard data buffer into which image is loaded
    */

    Atom target_atom = XInternAtom(display, "image/png", False),
         property_atom = XInternAtom(display, "XSEL_DATA", False);

    // is PNG
    if (clipboard_has_target(property_atom, target_atom)) {
        ClipboardData* cb = NULL;
        if ((cb = get_os_clipboard_data(property_atom, 0)) == NULL) {
            LOG_WARNING("Failed to get clipboard data");
            return NULL;
        }
        cb->type = CLIPBOARD_IMAGE;
        return cb;
    }

    // request failed, e.g. owner can't convert to the target format
    LOG_WARNING("Can't convert clipboard image to target format");
    return NULL;
}

static ClipboardData* get_os_clipboard_string(void) {
    /*
        Get a string from the Linux OS clipboard

        Returns:
            (ClipboardData*): clipboard data buffer into which string is loaded
    */

    Atom target_atom = XInternAtom(display, "UTF8_STRING", False),
         property_atom = XInternAtom(display, "XSEL_DATA", False);

    if (clipboard_has_target(property_atom, target_atom)) {
        ClipboardData* cb = NULL;
        if ((cb = get_os_clipboard_data(property_atom, 0)) == NULL) {
            LOG_WARNING("Failed to get clipboard data");
            return NULL;
        }
        cb->type = CLIPBOARD_TEXT;
        return cb;
    }

    // request failed, e.g. owner can't convert to the target format
    LOG_WARNING("Can't convert clipboard string to target format");
    return NULL;
}

void unsafe_free_clipboard_buffer(ClipboardData* cb) {
    /*
        Free clipboard buffer memory

        Arguments:
            cb (ClipboardData*): clipboard buffer data to be freed
    */

    if (cb && cb != &clipboard_none) {
        deallocate_region(cb);
    }
}

ClipboardData* unsafe_get_os_clipboard(void) {
    /*
        Get and return the current contents of the Linux clipboard

        Returns:
            (ClipboardData*): the clipboard data that has been read
                from the Linux OS clipboard
    */

    ClipboardData* cb = NULL;

    XLockDisplay(display);

    // Try to get a string/picture/files clipboard object
    if (cb == NULL) {
        cb = get_os_clipboard_string();
    }
    if (cb == NULL) {
        cb = get_os_clipboard_picture();
    }
    if (cb == NULL) {
        // Not implementing clipboard files right now
        // cb = get_os_clipboard_files();
    }

    XUnlockDisplay(display);

    if (cb == NULL) {
        // If no cb was found, just return the CLIPBOARD_NONE object
        return &clipboard_none;
    }

    return cb;
}

void unsafe_set_os_clipboard(ClipboardData* cb) {
    /*
        Set the Linux OS clipboard to contain the data from `cb`

        Arguments:
            cb (ClipboardData*): the clipboard data to load into
                the Linux OS clipboard
    */

    static FILE* inp = NULL;

    //
    // cb is expected to be something that was once returned by get_os_clipboard
    // If it's text or an image, simply set the data and type to the current
    // clipboard so that the clipboard acts just like it did when the previous
    // get_os_clipboard was called

    if (cb->type == CLIPBOARD_TEXT) {
        LOG_INFO("Setting clipboard to text!");

        // Open up xclip
        inp = popen("xclip -i -selection clipboard", "w");

        // Write text data
        fwrite(cb->data, 1, cb->size, inp);

        // Close pipe
        pclose(inp);

    } else if (cb->type == CLIPBOARD_IMAGE) {
        LOG_INFO("Setting clipboard to image!");

        // Open up xclip
        inp = popen("xclip -i -selection clipboard -t image/png", "w");

        // Write text data
        fwrite(cb->data, 1, cb->size, inp);

        // Close pipe
        pclose(inp);

    } else if (cb->type == CLIPBOARD_FILES) {
        LOG_INFO("Setting clipboard to Files");

        LOG_WARNING("SetClipboard: FILE CLIPBOARD NOT BEING IMPLEMENTED");
        return;
    }
    return;
}

static bool start_tracking_clipboard_updates(void) {
    /*
        Open X display and begin tracking clipboard updates

        Returns:
            (bool): true on success, false on failure
    */

    // To be called at the beginning of clipboard usage
    display = XOpenDisplay(NULL);
    if (!display) {
        LOG_WARNING("StartTrackingClipboardUpdates display did not open");
        perror(NULL);
        return false;
    }
    unsigned long color = BlackPixel(display, DefaultScreen(display));
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, color, color);
    clipboard = XInternAtom(display, "CLIPBOARD", False);
    incr_id = XInternAtom(display, "INCR", False);
    // check if xfixes extension available
    xfixes_available = XFixesQueryExtension(display, &event_base, &error_base);
    if (!xfixes_available) {
        LOG_ERROR("XFixes not available - clipboard and cursor will not work!");
    }
    // tell xfixes to send selection events to display
    XFixesSelectSelectionInput(display, DefaultRootWindow(display), clipboard,
                               XFixesSetSelectionOwnerNotifyMask);
    return true;
}

bool unsafe_has_os_clipboard_updated(void) {
    /*
        Whether the clipboard has updated since this function was last called
        or since start_tracking_clipboard_updates was last called.

        Returns:
            (bool): true if clipboard has updated, else false
    */
    if (!display || !xfixes_available) {
        return false;
    }

    static bool first = true;  // static, so only sets to true on first call
    XEvent event;
    if (first) {
        first = false;
        if (should_preserve_local_clipboard()) {
            return true;
        }
        return false;
    }
    XLockDisplay(display);
    while (XPending(display)) {
        XNextEvent(display, &event);
        if (event.type == event_base + XFixesSelectionNotify &&
            ((XFixesSelectionNotifyEvent*)&event)->selection == clipboard) {
            XUnlockDisplay(display);
            return true;
        }
    }
    XUnlockDisplay(display);
    return false;
}

static bool clipboard_has_target(Atom property_atom, Atom target_atom) {
    /*
        Check whether `property_atom` contains content of type `target_atom`.
        Typically, `property_atom` will be "XSEL_DATA" because this refers
        to the clipboard selection data. An example of `target_atom` is
        "image/png" for images.

        Arguments:
            property_atom (Atom): the property name being checked for
            target_atom (Atom): the source in which object of type `property_atom`
                is being searched for

        Returns:
            (bool): true if `target_atom` contains `property_atom`, else false
    */

    XEvent event;

    XLockDisplay(display);

    XSelectInput(display, window, PropertyChangeMask);
    XConvertSelection(display, clipboard, target_atom, property_atom, window, CurrentTime);

    do {
        XNextEvent(display, &event);
    } while (event.type != SelectionNotify || event.xselection.selection != clipboard);

    XUnlockDisplay(display);

    if (event.xselection.property == property_atom) {
        return true;
    } else {
        return false;
    }
}

ClipboardData* get_os_clipboard_data(Atom property_atom, int header_size) {
    /*
        Get the data from the Linux clipboard, as type `property_atom`

        Arguments:
            property_atom (Atom): type of data to be retrieved from OS clipboard
            header_size (int): how many bytes of header to skip over in the OS clipboard

        Returns:
            (ClipboardData*): the clipboard data buffer loaded with the OS clipboard data
    */

    Atom new_atom;
    int resbits;
    long unsigned ressize, restail;
    char* result;

    ClipboardData* cb = allocate_region(sizeof(ClipboardData));

    XGetWindowProperty(display, window, property_atom, 0, LONG_MAX / 4, True, AnyPropertyType,
                       &new_atom, &resbits, &ressize, &restail, (unsigned char**)&result);
    ressize *= resbits / 8;

    cb->size = 0;

    bool bad_clipboard = false;

    if (new_atom == incr_id) {
        // Don't need the incr_id result anymore
        XFree(result);

        do {
            XEvent event;
            do {
                XNextEvent(display, &event);
            } while (event.type != PropertyNotify || event.xproperty.atom != property_atom ||
                     event.xproperty.state != PropertyNewValue);

            XGetWindowProperty(display, window, property_atom, 0, LONG_MAX / 4, True,
                               AnyPropertyType, &new_atom, &resbits, &ressize, &restail,
                               (unsigned char**)(&result));

            // Measure size in bytes
            int src_size = ressize * (resbits / 8);
            char* src_data = result;
            if (cb->size == 0) {
                src_data += header_size;
                src_size -= header_size;
            }

            if (!bad_clipboard) {
                // Resize the buffer to that it can fit the new data
                cb = realloc_region(cb, sizeof(ClipboardData) + cb->size + src_size);

                // Copy the data over and update the size
                memcpy(cb->data + cb->size, src_data, src_size);
                cb->size += src_size;
            }

            XFree(result);
        } while (ressize > 0);
    } else {
        int src_size = ressize * (resbits / 8);
        char* src_data = result;
        if (cb->size == 0) {
            src_data += header_size;
            src_size -= header_size;
        }

        // Resize the buffer to that it can fit the new data
        cb = realloc_region(cb, sizeof(ClipboardData) + cb->size + src_size);

        // Copy the data over and update the size
        memcpy(cb->data + cb->size, src_data, src_size);
        cb->size += src_size;

        XFree(result);
    }

    if (bad_clipboard) {
        LOG_WARNING("Clipboard too large!");
        deallocate_region(cb);
        return NULL;
    }

    return cb;
}
