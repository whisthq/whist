/**
 * Copyright Fractal Computers, Inc. 2020
 * @file x11_clipboard.c
 * @brief This file contains the general clipboard functions for a shared
 *        client-server clipboard on Linux clients/servers.
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

#include <fractal/core/fractal.h>
#include "clipboard.h"
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

static Atom clipboard;
static Atom incr_id;

/*
============================
Private Functions
============================
*/

bool start_tracking_clipboard_updates();
bool clipboard_has_target(Atom property_atom, Atom target_atom);
DynamicBuffer* get_clipboard_data(Atom property_atom, int header_size);

/*
============================
Private Function Implementations
============================
*/

void unsafe_init_clipboard() {
    /*
        Initialize the clipboard by starting tracking clipboard updates
    */

    start_tracking_clipboard_updates();
}

void unsafe_destroy_clipboard() {
    /*
        Destroy clipboard by closing associated X display
    */

    if (display) {
        XCloseDisplay(display);
    }
}

DynamicBuffer* get_clipboard_picture() {
    /*
        Get an image from the Linux OS clipboard
        NOTE: Assume that clipboard stores pictures in png format when getting

        Returns:
            (DynamicBuffer*): clipboard data buffer into which image is loaded
    */

    Atom target_atom = XInternAtom(display, "image/png", False),
         property_atom = XInternAtom(display, "XSEL_DATA", False);

    // is PNG
    if (clipboard_has_target(property_atom, target_atom)) {
        DynamicBuffer* db = NULL;
        if ((db = get_clipboard_data(property_atom, 0)) == NULL) {
            LOG_WARNING("Failed to get clipboard data");
            return NULL;
        }
        ((ClipboardData*)db->buf)->type = CLIPBOARD_IMAGE;
        return db;
    }

    // request failed, e.g. owner can't convert to the target format
    LOG_WARNING("Can't convert clipboard image to target format");
    return NULL;
}

DynamicBuffer* get_clipboard_string() {
    /*
        Get a string from the Linux OS clipboard

        Returns:
            (DynamicBuffer*): clipboard data buffer into which string is loaded
    */

    Atom target_atom = XInternAtom(display, "UTF8_STRING", False),
         property_atom = XInternAtom(display, "XSEL_DATA", False);

    if (clipboard_has_target(property_atom, target_atom)) {
        DynamicBuffer* db = NULL;
        if ((db = get_clipboard_data(property_atom, 0)) == NULL) {
            LOG_WARNING("Failed to get clipboard data");
            return NULL;
        }
        ((ClipboardData*)db->buf)->type = CLIPBOARD_TEXT;
        return db;
    }

    // request failed, e.g. owner can't convert to the target format
    LOG_WARNING("Can't convert clipboard string to target format");
    return NULL;
}

DynamicBuffer* get_clipboard_files() {
    /*
        Get files from the Linux OS clipboard

        Returns:
            (DynamicBuffer*): clipboard data buffer into which files are loaded
    */

    Atom target_atom = XInternAtom(display, "x-special/gnome-copied-files", False);
    Atom property_atom = XInternAtom(display, "XSEL_DATA", False);

    if (clipboard_has_target(property_atom, target_atom)) {
        DynamicBuffer* db = NULL;
        if ((db = get_clipboard_data(property_atom, 0)) == NULL) {
            LOG_WARNING("Failed to get clipboard data");
            return NULL;
        }

        ClipboardData* cb = (ClipboardData*)db->buf;

        // Increase size by 1 for the null terminator
        resize_dynamic_buffer(db, sizeof(ClipboardData) + cb->size + 1);
        cb = (ClipboardData*)db->buf;

        // Add null terminator
        cb->data[cb->size] = '\0';
        cb->size++;

        char command[100] = "rm -rf ";
        strcat(command, GET_CLIPBOARD);
        system(command);
        mkdir(GET_CLIPBOARD, 0777);

        char* file = strtok(cb->data, "\n\r");
        if (file != NULL) {
            file = strtok(NULL, "\n\r");
        }

        while (file != NULL) {
            char file_prefix[] = "file://";
            if (memcmp(file, file_prefix, sizeof(file_prefix) - 1) == 0) {
                char final_filename[1000] = "";
                strcat(final_filename, GET_CLIPBOARD);
                strcat(final_filename, "/");
                strcat(final_filename, basename(file));
                LOG_INFO("NAME: %s %s %s", final_filename, file, basename(file));
                symlink(file + sizeof(file_prefix) - 1, final_filename);
            } else {
                LOG_WARNING("Not a file: %s", file);
            }
            file = strtok(NULL, "\n\r");
        }

        cb->type = CLIPBOARD_FILES;
        cb->size = 0;
        return db;
    } else {  // request failed, e.g. owner can't convert to the target format
        LOG_WARNING("Can't convert clipboard to target format");
        return NULL;
    }
}

static DynamicBuffer* db = NULL;

void unsafe_free_clipboard(ClipboardData* cb) {
    /*
        Free clipboard buffer memory

        Arguments:
            cb (ClipboardData*): clipboard data to be freed
    */

    if (cb->type != CLIPBOARD_NONE) {
        if (db == NULL) {
            LOG_ERROR("Called unsafe_free_clipboard, but there's nothing to free!");
        } else {
            free_dynamic_buffer(db);
            db = NULL;
        }
    } else {
        if (db != NULL) {
            LOG_ERROR("cb->type is CLIPBOARD_NONE, but DynamicBuffer is still allocated?");
            free_dynamic_buffer(db);
            db = NULL;
        }
    }
}

ClipboardData* unsafe_get_clipboard() {
    /*
        Get and return the current contents of the Linux clipboard

        Returns:
            (ClipboardData*): the clipboard data that has been read
                from the Linux OS clipboard
    */

    // Free the previous dynamic buffer (shouldn't be necessary)
    if (db != NULL) {
        LOG_ERROR(
            "Called unsafe_get_clipboard, but the caller hasn't called unsafe_free_clipboard yet!");
        free_dynamic_buffer(db);
        db = NULL;
    }

    // Try to get a string/picture/files clipboard object (Stored in db's buf)
    if (db == NULL) {
        db = get_clipboard_string();
    }
    if (db == NULL) {
        db = get_clipboard_picture();
    }
    if (db == NULL) {
        // Not implementing clipboard files right now
        // db = get_clipboard_files();
    }

    if (db == NULL) {
        // If no db was found, just return a CLIPBOARD_NONE object
        static ClipboardData cb_none;
        cb_none.type = CLIPBOARD_NONE;
        cb_none.size = 0;
        return &cb_none;
    } else {
        // We expect that the user of GetClipboard
        // will safe_malloc and memcpy his own version if he wants to
        // save multiple clipboards
        // Otherwise, we will free on the next call to get_clipboard
        return (ClipboardData*)db->buf;
    }
}

void unsafe_set_clipboard(ClipboardData* cb) {
    /*
        Set the Linux OS clipboard to contain the data from `cb`

        Arguments:
            cb (ClipboardData*): the clipboard data to load into
                the Linux OS clipboard
    */

    static FILE* inp = NULL;

    //
    // cb is expected to be something that was once returned by GetClipboard
    // If it's text or an image, simply set the data and type to the current
    // clipboard so that the clipboard acts just like it did when the previous
    // GetClipboard was called
    //
    // If cb->type == CLIPBOARD_FILES, then we take all of the files and folers
    // in
    // "./set_clipboard" and set them to be our clipboard. These files and
    // folders should not be symlinks, simply assume that they will never be
    // symlinks
    //

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
        /*
        struct dirent* de;
        DIR* dr = opendir(SET_CLIPBOARD);

        if (dr) {
            inp = popen(CLOSE_FDS "xclip -i -sel clipboard -t x-special/gnome-copied-files", "w");

            char prefix[] = "copy";
            fwrite(prefix, 1, sizeof(prefix) - 1, inp);

            // Construct path as "\nfile:///path/to/fractal/set_clipboard/"
            char path[PATH_MAX] = "\nfile://";
            int substr = strlen(path);
            realpath(SET_CLIPBOARD, path + substr);
            strcat(path, "/");
            // subpath is an index that points to the end of the string
            int subpath = strlen(path);

            // Read through the directory
            while (dr && (de = readdir(dr)) != NULL) {
                // Ignore . and ..
                if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
                    continue;
                }

                // Copy the dname into it
                int d_name_size = strlen(de->d_name);
                memcpy(path + subpath, de->d_name, d_name_size);

                // Write the subpath and the dname into it
                fwrite(path, 1, subpath + d_name_size, inp);
            }

            // Close the file descriptor
            pclose(inp);
        }
        */
    }
    return;
}

bool start_tracking_clipboard_updates() {
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
    return true;
}

bool unsafe_has_clipboard_updated() {
    /*
        Whether the clipboard has updated since this function was last called
        or since start_tracking_clipboard_updates was last called.

        Returns:
            (bool): true if clipboard has updated, else false
    */

    if (!display) {
        return false;
    }

    static bool first = true;  // static, so only sets to true on first call
    int event_base, error_base;
    XEvent event;
    if (!XFixesQueryExtension(display, &event_base, &error_base)) {
        return false;
    }
    XFixesSelectSelectionInput(display, DefaultRootWindow(display), clipboard,
                               XFixesSetSelectionOwnerNotifyMask);
    if (first) {
        first = false;
        if (should_preserve_local_clipboard()) {
            return true;
        }
        return false;
    }
    while (XPending(display)) {
        XNextEvent(display, &event);
        if (event.type == event_base + XFixesSelectionNotify &&
            ((XFixesSelectionNotifyEvent*)&event)->selection == clipboard) {
            return true;
        }
    }
    return false;
}

bool clipboard_has_target(Atom property_atom, Atom target_atom) {
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

    XSelectInput(display, window, PropertyChangeMask);
    XConvertSelection(display, clipboard, target_atom, property_atom, window, CurrentTime);

    do {
        XNextEvent(display, &event);
    } while (event.type != SelectionNotify || event.xselection.selection != clipboard);

    if (event.xselection.property == property_atom) {
        return true;
    } else {
        return false;
    }
}

DynamicBuffer* get_clipboard_data(Atom property_atom, int header_size) {
    /*
        Get the data from the clipboard, as type `property_atom`

        Arguments:
            property_atom (Atom): type of data to be retrieved from OS clipboard
            header_size (int): how many bytes of header to skip over in the OS clipboard

        Returns:
            (DynamicBuffer*): the clipboard data buffer loaded with the OS clipboard data
    */

    Atom new_atom;
    int resbits;
    long unsigned ressize, restail;
    char* result;

    DynamicBuffer* db = init_dynamic_buffer(true);
    resize_dynamic_buffer(db, sizeof(ClipboardData));

    ClipboardData* cb = (ClipboardData*)db->buf;

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
                // Resize the dynamic buffer to that it can fit the new data
                resize_dynamic_buffer(db, sizeof(ClipboardData) + cb->size + src_size);
                cb = (ClipboardData*)db->buf;

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

        // Resize the dynamic buffer to that it can fit the new data
        resize_dynamic_buffer(db, sizeof(ClipboardData) + cb->size + src_size);
        cb = (ClipboardData*)db->buf;

        // Copy the data over and update the size
        memcpy(cb->data + cb->size, src_data, src_size);
        cb->size += src_size;

        XFree(result);
    }

    if (bad_clipboard) {
        LOG_WARNING("Clipboard too large!");
        free_dynamic_buffer(db);
        return NULL;
    }

    return db;
}
