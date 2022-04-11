/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file file_drop.c
 * @brief This file contains all the code for dropping a file into a window.
============================
Usage
============================

init_file_drop_handler();

TransferringFile* drop_file;
--- GET FILE INFO ---
drop_file_into_active_window(drop_file);

destroy_file_drop_handler();

*/

#include <whist/core/whist.h>
#include "file_drop.h"

#ifndef __linux__

bool init_file_drop_handler(void) {
    LOG_INFO("File drop handler not supported on this platform");
    return false;
}

int drop_file_into_active_window(TransferringFile* drop_file) {
    LOG_WARNING("UNIMPLEMENTED: drop_file_into_active_window on non-Linux");
    return -1;
}

const char* file_drop_prepare(int id, FileMetadata* file_metadata) {
    LOG_WARNING("UNIMPLEMENTED: file_drop_prepare on non-Linux");
    return NULL;
}

int file_drop_mark_ready(int unique_id) {
    LOG_WARNING("UNIMPLEMENTED: file_drop_mark_ready on non-Linux");
    return -1;
}

void destroy_file_drop_handler(void) {
    LOG_WARNING("UNIMPLEMENTED: destroy_file_drop_handler on non-Linux");
}

int file_drag_update(bool is_dragging, int x, int y) {
    LOG_WARNING("UNIMPLEMENTED: file_drag_update on non-Linux");
    return -1;
}

#elif __linux__

/*
============================
Includes
============================
*/

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdbool.h>

// As we support for drag and drop types, we can add onto this list.
static Atom XA_text_uri_list;  // NOLINT

// The atoms used for the different messages sent between drag and drop peers
static Atom XA_XdndSelection;   // NOLINT
static Atom XA_XdndAware;       // NOLINT
static Atom XA_XdndEnter;       // NOLINT
static Atom XA_XdndLeave;       // NOLINT
static Atom XA_XdndTypeList;    // NOLINT
static Atom XA_XdndPosition;    // NOLINT
static Atom XA_XdndActionCopy;  // NOLINT
static Atom XA_XdndActionAsk;   // NOLINT
static Atom XA_XdndStatus;      // NOLINT
static Atom XA_XdndDrop;        // NOLINT
static Atom XA_XdndFinished;    // NOLINT

static Display* display = NULL;

Window our_window;
Window active_window;

/*
============================
Public Function Implementations
============================
*/

bool init_file_drop_handler(void) {
    /*
        Initialize the X11 display and XDND atoms for the file
        drop handler

        Returns:
            (bool): true if successful, false otherwise
    */

    if (display == NULL) {
        display = XOpenDisplay(NULL);
    }

    XA_text_uri_list = XInternAtom(display, "text/uri-list", False);
    XA_XdndSelection = XInternAtom(display, "XdndSelection", False);
    XA_XdndAware = XInternAtom(display, "XdndAware", False);
    XA_XdndEnter = XInternAtom(display, "XdndEnter", False);
    XA_XdndLeave = XInternAtom(display, "XdndLeave", False);
    XA_XdndTypeList = XInternAtom(display, "XdndTypeList", False);
    XA_XdndPosition = XInternAtom(display, "XdndPosition", False);
    XA_XdndActionCopy = XInternAtom(display, "XdndActionCopy", False);
    XA_XdndActionAsk = XInternAtom(display, "XdndActionAsk", False);
    XA_XdndStatus = XInternAtom(display, "XdndStatus", False);
    XA_XdndDrop = XInternAtom(display, "XdndDrop", False);
    XA_XdndFinished = XInternAtom(display, "XdndFinished", False);

    return true;
}

int drop_file_into_active_window(TransferringFile* drop_file) {
    /*
        Use the XDND protocol to execute a drag and drop of `drop_file`
        onto the active X11 window.

        Arguments:
            drop_file (TransferringFile*): the file to be dropped into the active X11 window

        Returns:
            (int): 0 on success, -1 on failure

        NOTE: This is only for servers running a parallel FUSE filesystem
    */

    // Try to wait for FUSE path to exist before initiating drop sequence
    WhistTimer fuse_ready_wait_timer;
    start_timer(&fuse_ready_wait_timer);
    char fuse_ready_path[128];
    snprintf(fuse_ready_path, 128, "/home/whist/.teleport/drag-drop/fuse_ready/%d", drop_file->id);
    while (access(fuse_ready_path, F_OK) != 0 &&
           get_timer(&fuse_ready_wait_timer) < 200.0 / MS_IN_SECOND) {
        whist_sleep(50);
    }

    // Just in case a drag end event was sent before
    if (file_drag_update(true, drop_file) < 0) {
        return -1;
    }

    XClientMessageEvent m;

    // XDND 5 - Active X11 window will respond with XdndStatus
    //     this ClientMessage indicates whether active X11 window will accept the drop and what
    //     action will be taken in the meantime, the active X11 window may send SelectionRequest
    //     messages - process and respond to these
    const char* fuse_directory = "/home/whist/drag-drop/";
    int fuse_path_maxlen =
        strlen(fuse_directory) + 20 + strlen(drop_file->filename) + 1;  // have enough space for int
    char* fuse_path = safe_malloc(fuse_path_maxlen);
    memset(fuse_path, 0, fuse_path_maxlen);
    snprintf(fuse_path, fuse_path_maxlen, "%s%d/%s", fuse_directory, drop_file->id,
             drop_file->filename);

    // Create the file URL to send as part of the SelectionNotify event to the active X11 window
    int xdnd_file_url_len = 7 + strlen(fuse_path) + 1;  // +7 for 'file://'; +1 for null terminator
    char* xdnd_file_url = safe_malloc(xdnd_file_url_len);
    memset(xdnd_file_url, 0, xdnd_file_url_len);
    safe_strncpy(xdnd_file_url, "file://", 8);
    safe_strncpy(xdnd_file_url + 7, fuse_path, strlen(fuse_path) + 1);

    WhistTimer active_window_response_loop_timer;
    start_timer(&active_window_response_loop_timer);
    XEvent e;
    bool accepted_drop = false;
    // Wait for up to 2 seconds for the XDND exchange to successfully complete, and then abort.
    while (get_timer(&active_window_response_loop_timer) < 2.0) {
        XLockDisplay(display);
        XNextEvent(display, &e);
        XUnlockDisplay(display);

        if (e.type == ClientMessage && e.xclient.message_type == XA_XdndStatus) {
            if ((e.xclient.data.l[1] & 1) == 0) {
                // XDND 4.5 - Active X11 window is not accepting the drop yet,
                //     so we wait and resend our XdndPosition message
                whist_sleep(50);
                // XSendEvent(display, active_window, False, NoEventMask, (XEvent*)&position_message);
                // XFlush(display);
                file_drag_update(true, drop_file->event_info.server_drop.x, drop_file->event_info.server_drop.y);
            } else {
                // Active X11 window is accepting the drop
                // XDND 6 - Send XdndDrop to active X11 window
                memset(&m, 0, sizeof(m));
                m.type = ClientMessage;
                m.display = display;
                m.window = active_window;
                m.message_type = XA_XdndDrop;
                m.format = 32;
                m.data.l[0] = our_window;
                m.data.l[1] = 0;
                m.data.l[2] =
                    CurrentTime;  // Our data is not time dependent, so send a generic timestamp;
                m.data.l[3] = 0;
                m.data.l[4] = 0;

                XSendEvent(display, active_window, False, NoEventMask, (XEvent*)&m);
                XFlush(display);

                accepted_drop = true;
            }
        } else if (e.type == SelectionRequest) {
            // When we receive a SelectionRequest from the active X11 window, we send a
            // SelectionNotify event back to
            //     the active X11 window with all of the information about the drop.

            Window requestor_window = e.xselectionrequest.requestor;
            Atom selection_request_property = e.xselectionrequest.property;

            // Load file URL into XdndSelection
            XEvent s;
            s.xselection.type = SelectionNotify;
            s.xselection.requestor = requestor_window;  // Should be the active X11 window
            s.xselection.selection = XA_XdndSelection;
            s.xselection.target = e.xselectionrequest.target;
            s.xselection.time = e.xselectionrequest.time;
            s.xselection.property = selection_request_property;

            XChangeProperty(display, requestor_window, selection_request_property, XA_text_uri_list,
                            8, PropModeReplace, (unsigned char*)xdnd_file_url,
                            strlen(xdnd_file_url));

            XSendEvent(display, requestor_window, True, 0, &s);
            XFlush(display);
        } else if (e.type == ClientMessage && e.xclient.message_type == XA_XdndFinished) {
            // The active X11 window has indicated that it is done with the drag and drop sequence,
            // so we can break
            break;
        }
    }

    if (!accepted_drop) {
        LOG_WARNING(
            "XDND exchange for file ID %d was rejected by target and completed unsuccessfully",
            drop_file->id);
    }

    // Just to make sure that the file drag event ends
    file_drag_update(false, drop_file);

    free(fuse_path);

    LOG_INFO("XDND exchange for file ID %d complete", drop_file->id);

    return 0;
}

const char* file_drop_prepare(int id, FileMetadata* file_metadata) {
    /*
        Create directories to house the file data and metadata, and write
        necesary metadata (for example, the file size) to the metadata file.

        Arguments:
            id (int): the unique ID for the file transfer
            file_metadata (FileMetadata*): the metadata for the file being transferred

        Returns:
            const char*: on success, the path to the file data directory, NULL on failure

        NOTE: This is only for servers running a parallel FUSE filesystem
    */

    static const char* base_downloads_directory = "/home/whist/.teleport/drag-drop/downloads";
    static const char* base_info_directory = "/home/whist/.teleport/drag-drop/file_info";
#define MAX_INT_LEN 64
    char* downloads_path = safe_malloc(strlen(base_downloads_directory) + 1 + MAX_INT_LEN + 1);
    char* info_path = safe_malloc(strlen(base_info_directory) + 1 + MAX_INT_LEN + 1);
    snprintf(downloads_path, strlen(base_downloads_directory) + 1 + MAX_INT_LEN + 1, "%s/%d",
             base_downloads_directory, id);
    snprintf(info_path, strlen(base_info_directory) + 1 + MAX_INT_LEN + 1, "%s/%d",
             base_info_directory, id);

    // Create file_info directory
    if (safe_mkdir(info_path)) {
        LOG_ERROR("Failed to create info directory for file ID %d", id);
        free(downloads_path);
        free(info_path);
        return NULL;
    }

    // Write the file size to file_info/id/file_size
    char* file_size_path = safe_malloc(strlen(info_path) + 1 + MAX_INT_LEN + 1);
    snprintf(file_size_path, strlen(info_path) + 1 + MAX_INT_LEN + 1, "%s/file_size", info_path);
    FILE* file_size_file_handle = fopen(file_size_path, "w");
    if (file_size_file_handle) {
        fprintf(file_size_file_handle, "%d", file_metadata->file_size);
        fclose(file_size_file_handle);
    }
    free(file_size_path);
    free(info_path);

    // Create download directory last, since that's what activates teleport
    if (safe_mkdir(downloads_path)) {
        LOG_ERROR("Failed to create downloads directory for file ID %d", id);
        free(downloads_path);
        return NULL;
    }

    return downloads_path;
}

int file_drop_mark_ready(int id) {
    /*
        Write the ready file that indicates that the file has been fully
        written and the FUSE node is ready.

        Arguments:
            id (int): the unique ID for which we are writing the ready file

        Returns:
            (int): 0 on success, -1 on failure

        NOTE: This is only for servers running a parallel FUSE filesystem
    */

    LOG_INFO("Writing FUSE ready file for ID %d", id);

    // Create file path for file `/home/whist/.teleport/drag-drop/ready/[ID]`
    static const char* ready_directory = "/home/whist/.teleport/drag-drop/ready/";
    char ready_file_path[64];
    memset(ready_file_path, 0, 64);
    snprintf(ready_file_path, 64, "%s%d", ready_directory, id);

    FILE* ready_file_handle = fopen(ready_file_path, "w");
    if (!ready_file_handle) {
        LOG_ERROR("Failed to open FUSE ready file");
        return -1;
    }
    fclose(ready_file_handle);
    return 0;
}

void destroy_file_drop_handler(void) {
    /*
        Clean up the file drop handler
    */

    if (display != NULL) {
        XCloseDisplay(display);
        display = NULL;
    }
}

int file_drag_update(bool is_dragging, int x, int y) {
    /*
        Update the file drag indicator
    */

    // if (!drop_file) {
    //     return -1;
    // }

    static bool active_file_drag = false;
    // static Window our_window;
    // static Window active_window;

    XClientMessageEvent m;

    if (!display) {
        return -1;
    }

    if (is_dragging) {
        if (!active_file_drag) {
            // DRAG BEGINS

            int revert;

            // Get our window and active X11 window
            unsigned long color = BlackPixel(display, DefaultScreen(display));
            our_window =
                XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, color, color);
            XGetInputFocus(display, &active_window, &revert);

            if (!active_window) {
                // No window is active.
                return -1;
            }

            // LOG_INFO("Executing XDND exchange for file ID %d", drop_file->id);

            // The XDND communication exchange begins. Number steps are taken from
            // https://freedesktop.org/wiki/Specifications/XDND/

            // XDND 1 - We take ownership of XdndSelection
            XSetSelectionOwner(display, XA_XdndSelection, our_window, CurrentTime);

            // XDND 2 - We send XdndEnter to active X11 window
            // Get our XDND version
            int our_xdnd_version = 5;
            XChangeProperty(display, our_window, XA_XdndAware, XA_ATOM, 32, PropModeReplace,
                            (unsigned char*)&our_xdnd_version, 1);

            // Get the active X11 window's XDND version
            unsigned char* version_data = 0;
            Atom atmp;
            int fmt;
            unsigned long nitems, bytes_remaining;
            int active_window_xdnd_version = -1;
            if (XGetWindowProperty(display, active_window, XA_XdndAware, 0, 2, False, AnyPropertyType,
                                   &atmp, &fmt, &nitems, &bytes_remaining, &version_data) != Success) {
                LOG_ERROR("Failed to read active X11 window version");
                return -1;
            } else {
                active_window_xdnd_version = version_data[0];
            }

            // Formulate XdndEnter message and send to active X11 window
            memset(&m, 0, sizeof(m));
            m.type = ClientMessage;
            m.display = display;
            m.window = active_window;
            m.message_type = XA_XdndEnter;
            m.format = 32;
            m.data.l[0] = our_window;
            // We need to use the minimum supported XDND version between the two windows
            m.data.l[1] = min(our_xdnd_version, active_window_xdnd_version) << 24 | 0;
            // TODO: Currently we only support drag and drop of files. In order to support more types (e.g.
            // dragging text or images),
            //     we need to add on to this list of supported types here.
            m.data.l[2] = XA_text_uri_list;
            m.data.l[3] = None;
            m.data.l[4] = None;

            XSendEvent(display, active_window, False, NoEventMask, (XEvent*)&m);
            XFlush(display);

            // XDND 3 - TODO: related to the above TODO, if we support more than 3 types of
            // drag-and-droppable content, then we will need to
            //     request XdndTypeList and call
            //     XChangeProperty(disp, w, XdndTypeList, XA_ATOM, 32, PropModeReplace, (unsigned
            //     char*)&targets[0], targets.size()); beforehand. Since we only support one type right now,
            //     we can skip this step in the XDND exchange.
        }

        // XDND 4 - Send XdndPosition to active X11 window
        XClientMessageEvent position_message;
        memset(&position_message, 0, sizeof(position_message));
        position_message.type = ClientMessage;
        position_message.display = display;
        position_message.window = active_window;
        position_message.message_type = XA_XdndPosition;
        position_message.format = 32;
        position_message.data.l[0] = our_window;
        position_message.data.l[1] = 0;
        position_message.data.l[2] =
            // (drop_file->event_info.server_drop.x << 16) | drop_file->event_info.server_drop.y;
            (x << 16) | y;
        position_message.data.l[3] =
            CurrentTime;  // Our data is not time dependent, so send a generic timestamp;
        position_message.data.l[4] = XA_XdndActionCopy;

        XSendEvent(display, active_window, False, NoEventMask, (XEvent*)&position_message);
        XFlush(display);
    } else {
        if (active_file_drag) {
            // DRAG ENDS

            // XDND 7 - Once we are done, we send active X11 window an XdndLeave message to indicate that
            // the XDND communication sequence is complete
            memset(&m, 0, sizeof(m));
            m.type = ClientMessage;
            m.display = display;
            m.window = active_window;
            m.message_type = XA_XdndLeave;
            m.format = 32;
            m.data.l[0] = our_window;

            XSendEvent(display, active_window, False, NoEventMask, (XEvent*)&m);
            XFlush(display);
        }
    }

    active_file_drag = is_dragging;

    return active_window;
}

#endif  // __linux__
