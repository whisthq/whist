/*
 * Clipboard getting and setting functions on Linux Ubuntu.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
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

#include "clipboard.h"
#include "../core/fractal.h"

#ifndef GET_CLIPBOARD
#define GET CLIPBOARD "./get_clipboard"
#endif

#ifndef SET_CLIPBOARD
#define SET CLIPBOARD "./set_clipboard"
#endif

#define MAX_CLIPBOARD_SIZE 10000000

static char cb_buf[MAX_CLIPBOARD_SIZE];
static Display* display = NULL;
static Window window;

static Atom clipboard;
static Atom incr_id;

bool clipboard_has_target(Atom property_atom, Atom target_atom);
bool get_clipboard_data(Atom property_atom, ClipboardData* cb, int header_size);

void initClipboard() {}

bool get_clipboard_picture(ClipboardData* cb) {
    Atom target_atom = XInternAtom(display, "image/bmp", False),
         property_atom = XInternAtom(display, "XSEL_DATA", False);

    if (clipboard_has_target(property_atom, target_atom)) {
        if (!get_clipboard_data(property_atom, cb, 14)) {
            LOG_WARNING("Failed to get clipboard data");
            return false;
        }

        LOG_INFO("content size: %d\n", cb->size);

        // WRITE TO FILE
        char* file_buf = malloc(cb->size + 14);
        memcpy(file_buf + 14, cb->data, cb->size);
        *((char*)(&file_buf[0])) = 'B';
        *((char*)(&file_buf[1])) = 'M';
        *((int*)(&file_buf[2])) = cb->size + 14;
        *((int*)(&file_buf[10])) = 54;
        FILE* fptr;
        fptr = fopen("output.bmp", "wb");
        fwrite(file_buf, 1, cb->size + 14, fptr);
        fclose(fptr);
        // END WRITE TO FILE

        cb->type = CLIPBOARD_IMAGE;
        return true;
    } else {  // request failed, e.g. owner can't convert to the target format
        LOG_WARNING("Can't convert clipboard image to target format");
        return false;
    }
}
bool get_clipboard_string(ClipboardData* cb) {
    Atom target_atom = XInternAtom(display, "UTF8_STRING", False),
         property_atom = XInternAtom(display, "XSEL_DATA", False);

    if (clipboard_has_target(property_atom, target_atom)) {
        if (!get_clipboard_data(property_atom, cb, 0)) {
            LOG_WARNING("Failed to get clipboard data");
            return false;
        }

        cb->type = CLIPBOARD_TEXT;
        return true;
    } else {  // request failed, e.g. owner can't convert to the target format
        LOG_WARNING("Can't convert clipboard image to target format");
        return false;
    }
}

bool get_clipboard_files(ClipboardData* cb) {
    Atom target_atom =
        XInternAtom(display, "x-special/gnome-copied-files", False);
    Atom property_atom = XInternAtom(display, "XSEL_DATA", False);

    if (clipboard_has_target(property_atom, target_atom)) {
        if (!get_clipboard_data(property_atom, cb, 0)) {
            LOG_WARNING("Failed to get clipboard data");
            return false;
        }
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
                printf("NAME: %s %s %s\n", final_filename, file,
                       basename(file));
                symlink(file + sizeof(file_prefix) - 1, final_filename);
            } else {
                mprintf("Not a file: %s\n", file);
            }
            file = strtok(NULL, "\n\r");
        }

        cb->type = CLIPBOARD_FILES;
        cb->size = 0;
        return true;
    } else {  // request failed, e.g. owner can't convert to the target format
        LOG_WARNING("Can't convert clipboard to target format");
        return false;
    }
}

ClipboardData* GetClipboard() {
    ClipboardData* cb = (ClipboardData*)cb_buf;
    cb->type = CLIPBOARD_NONE;
    cb->size = 0;
    get_clipboard_files(cb) || get_clipboard_picture(cb) ||
        get_clipboard_string(cb);

    // Essentially just cb_buf, we expect that the user of GetClipboard
    // will malloc his own version if he wants to save multiple clipboards
    // Otherwise, we just reuse the same memory buffer
    return cb;
}

void SetClipboard(ClipboardData* cb) {
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

        // Write file header
        char* file_buf = malloc(14);
        *((char*)(&file_buf[0])) = 'B';
        *((char*)(&file_buf[1])) = 'M';
        *((int*)(&file_buf[2])) = cb->size + 14;
        *((int*)(&file_buf[10])) = 54;
        fwrite(file_buf, 1, 14, inp);

        // Write file data
        fwrite(cb->data, 1, cb->size, inp);

        // Close pipe
        pclose(inp);
    } else if (cb->type == CLIPBOARD_FILES) {
        LOG_INFO("Setting clipboard to Files");

        inp = popen("xclip -i -sel clipboard -t x-special/gnome-copied-files",
                    "w");

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
        struct dirent* de;
        DIR* dr = opendir(SET_CLIPBOARD);
        while ((de = readdir(dr)) != NULL) {
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

    // We make sure that pending clipboard updates are wiped out, because we
    // just recently updated the clipboard hasClipboardUpdated() is going to
    // return true right now:
    hasClipboardUpdated();
    // hasClipboardUpdated() should return false after this
    return;
}

bool StartTrackingClipboardUpdates() {
    // To be called at the beginning of clipboard usage
    display = XOpenDisplay(NULL);
    if (!display) {
        LOG_WARNING("StartTrackingClipboardUpdates display did not open");
        perror(NULL);
        return false;
    }
    unsigned long color = BlackPixel(display, DefaultScreen(display));
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1,
                                 1, 0, color, color);
    clipboard = XInternAtom(display, "CLIPBOARD", False);
    incr_id = XInternAtom(display, "INCR", False);
}

bool hasClipboardUpdated() {
    //
    // If the clipboard has updated since this function was last called,
    // or since StartTrackingClipboardUpdates was last called,
    // then we return "true". Otherwise, return "false".
    //

    static bool first = true;
    int event_base, error_base;
    XEvent event;
    assert(XFixesQueryExtension(display, &event_base, &error_base));
    XFixesSelectSelectionInput(display, DefaultRootWindow(display), clipboard,
                               XFixesSetSelectionOwnerNotifyMask);
    if (first) {
        first = false;
        return true;
    }
    while (XPending(display)) {
        XNextEvent(display, &event);
        if (event.type == event_base + XFixesSelectionNotify &&
            ((XFixesSelectionNotifyEvent*)&event)->selection == clipboard)
            return true;
    }
    return false;
}

bool clipboard_has_target(Atom property_atom, Atom target_atom) {
    XEvent event;

    XSelectInput(display, window, PropertyChangeMask);
    XConvertSelection(display, clipboard, target_atom, property_atom, window,
                      CurrentTime);

    do {
        XNextEvent(display, &event);
    } while (event.type != SelectionNotify ||
             event.xselection.selection != clipboard);

    if (event.xselection.property == property_atom) {
        return true;
    } else {
        return false;
    }
}

bool get_clipboard_data(Atom property_atom, ClipboardData* cb,
                        int header_size) {
    Atom new_atom;
    int resbits;
    long unsigned ressize, restail;
    char* result;

    XGetWindowProperty(display, window, property_atom, 0, LONG_MAX / 4, True,
                       AnyPropertyType, &new_atom, &resbits, &ressize, &restail,
                       (unsigned char**)&result);
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
            } while (event.type != PropertyNotify ||
                     event.xproperty.atom != property_atom ||
                     event.xproperty.state != PropertyNewValue);

            XGetWindowProperty(display, window, property_atom, 0, LONG_MAX / 4,
                               True, AnyPropertyType, &new_atom, &resbits,
                               &ressize, &restail, (unsigned char**)(&result));

            // Measure size in bytes
            int src_size = ressize * (resbits / 8);
            char* src_data = result;
            if (cb->size == 0) {
                src_data += header_size;
                src_size -= header_size;
            }

            if (cb->size + src_size > MAX_CLIPBOARD_SIZE) {
                bad_clipboard = true;
            }

            if (!bad_clipboard) {
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

        memcpy(cb->data + cb->size, src_data, src_size);
        cb->size += src_size;

        XFree(result);
    }

    if (bad_clipboard) {
        LOG_WARNING("Clipboard too large!");
        cb->type = CLIPBOARD_NONE;
        cb->size = 0;
        return false;
    }

    return true;
}
