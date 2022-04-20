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

int file_drag_update(bool is_dragging, int x, int y, FileDragData* file_list) {
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

WhistMutex xdnd_mutex;

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

    xdnd_mutex = whist_create_mutex();

    return true;
}

void xdnd_send_drop(void);
void xdnd_send_selection_notify(char* xdnd_file_list, XEvent selection_request_event);
void xdnd_send_position(int x, int y);
int xdnd_own_and_send_enter(void);
void xdnd_send_leave(void);

void xdnd_send_drop(void) {
    XClientMessageEvent m;
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
}

void xdnd_send_selection_notify(char* xdnd_file_list, XEvent selection_request_event) {
    LOG_INFO("selection 1");
    Window requestor_window = selection_request_event.xselectionrequest.requestor;
    Atom selection_request_property = selection_request_event.xselectionrequest.property;

    LOG_INFO("selection 2");

    // Load file URL into XdndSelection
    XEvent s;
    s.xselection.type = SelectionNotify;
    s.xselection.requestor = requestor_window;  // Should be the active X11 window
    s.xselection.selection = XA_XdndSelection;
    s.xselection.target = selection_request_event.xselectionrequest.target;
    s.xselection.time = selection_request_event.xselectionrequest.time;
    s.xselection.property = selection_request_property;

    LOG_INFO("selection 3");

    XChangeProperty(display, requestor_window, selection_request_property, XA_text_uri_list,
                    8, PropModeReplace, (unsigned char*)xdnd_file_list,
                    strlen(xdnd_file_list));

    LOG_INFO("selection 4");

    XSendEvent(display, requestor_window, True, 0, &s);
    XFlush(display);

    LOG_INFO("selection 5");
}

void xdnd_send_position(int x, int y) {
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
        (x << 16) | y;
    position_message.data.l[3] =
        CurrentTime;  // Our data is not time dependent, so send a generic timestamp;
    position_message.data.l[4] = XA_XdndActionCopy;

    XSendEvent(display, active_window, False, NoEventMask, (XEvent*)&position_message);
    XFlush(display);
}

int xdnd_own_and_send_enter(void) {
    int revert;
    if (!display || !active_window || !our_window) {
        // Get our window and active X11 window
        unsigned long color = BlackPixel(display, DefaultScreen(display));
        our_window =
            XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, color, color);
        XGetInputFocus(display, &active_window, &revert);
    }

    if (!display || !active_window || !our_window) {
        return -1;
    }

    XSetSelectionOwner(display, XA_XdndSelection, our_window, CurrentTime);

    XClientMessageEvent m;
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

    return 0;
}

void xdnd_send_leave(void) {
    XClientMessageEvent m;
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

int drop_file_into_active_window(TransferringFile* drop_file) {
    /*
        Use the XDND protocol to execute a drag and drop of `drop_file`
        onto the active X11 window.

        Arguments:
            drop_file (TransferringFile*): the file to be dropped into the active X11 window.
                If NULL, then drop all the previous files at once. If not NULL, just add filename
                to list of files to be dropped together.

        Returns:
            (int): 0 on success, -1 on failure

        NOTE: This is only for servers running a parallel FUSE filesystem
    */

    static char* file_uri_list = NULL;
    static int file_uri_list_strlen = 0;
    static int drop_x = 0;
    static int drop_y = 0;

    XClientMessageEvent m;

    LOG_INFO("drag drop_file_into_active_window: %p", drop_file);

    if (drop_file) {
        // Try to wait for FUSE path to exist before initiating drop sequence
        WhistTimer fuse_ready_wait_timer;
        start_timer(&fuse_ready_wait_timer);
        char fuse_ready_path[128];
        snprintf(fuse_ready_path, 128, "/home/whist/.teleport/drag-drop/fuse_ready/%d", drop_file->id);
        while (access(fuse_ready_path, F_OK) != 0 &&
               get_timer(&fuse_ready_wait_timer) < 200.0 / MS_IN_SECOND) {
            whist_sleep(50);
        }

        // Cancel any possible pending drag events
        // file_drag_update(false, 0, 0, NULL);

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
        // char* xdnd_file_url = safe_malloc(xdnd_file_url_len);
        // memset(xdnd_file_url, 0, xdnd_file_url_len);
        // safe_strncpy(xdnd_file_url, "file://", 8);
        // safe_strncpy(xdnd_file_url + 7, fuse_path, strlen(fuse_path) + 1);

        file_uri_list_strlen += xdnd_file_url_len;
        if (file_uri_list) {
            file_uri_list = safe_realloc(file_uri_list, file_uri_list_strlen);
            file_uri_list[file_uri_list_strlen - xdnd_file_url_len - 1] = '\n';
        } else {
            file_uri_list = safe_malloc(file_uri_list_strlen);
        }

        char* new_file_uri_position = file_uri_list + file_uri_list_strlen - xdnd_file_url_len;
        // TODO: add new url to end of lis
        memset(new_file_uri_position, 0, xdnd_file_url_len);
        safe_strncpy(new_file_uri_position, "file://", 8);
        safe_strncpy(new_file_uri_position + 7, fuse_path, strlen(fuse_path) + 1);

        free(fuse_path);

        drop_x = drop_file->event_info.server_drop.x;
        drop_y = drop_file->event_info.server_drop.y;

        // Don't initiate XDND sequence until we receive a NULL TransferringFile
        return 0;
    }

    int retval = 0;

    LOG_INFO("file_drop file_uri_list: %s", file_uri_list);

    // Just in case a drag end event was sent before
    // if (file_drag_update(true, drop_file->event_info.server_drop.x, drop_file->event_info.server_drop.y) < 0) {
    LOG_INFO("file drag update false 1");
    file_drag_update(false, 0, 0, NULL);
    LOG_INFO("finished file drag update");
    if (!file_uri_list) {
        LOG_INFO("file_uri_list: %p", file_uri_list);
        retval = -1;
        goto reset_file_drop_statics;
    }

    LOG_INFO("starting XDND");
    whist_lock_mutex(xdnd_mutex);

    // The XDND communication exchange begins. Number steps are taken from
    // https://freedesktop.org/wiki/Specifications/XDND/

    // XDND 1 - We take ownership of XdndSelection
    // XDND 2 - We send XdndEnter to active X11 window
    // Get our XDND version
    if (xdnd_own_and_send_enter() < 0) {
        LOG_ERROR("Could not own XdndSelection and enter active X11 window");
        whist_unlock_mutex(xdnd_mutex);
        return -1;
    }

    // XDND 3 - TODO: related to the above TODO, if we support more than 3 types of
    // drag-and-droppable content, then we will need to
    //     request XdndTypeList and call
    //     XChangeProperty(disp, w, XdndTypeList, XA_ATOM, 32, PropModeReplace, (unsigned
    //     char*)&targets[0], targets.size()); beforehand. Since we only support one type right now,
    //     we can skip this step in the XDND exchange.

    XLockDisplay(display);
    // XDND 4 - Send XdndPosition to active X11 window
    xdnd_send_position(drop_x, drop_y);

    WhistTimer active_window_response_loop_timer;
    start_timer(&active_window_response_loop_timer);
    XEvent e;
    bool accepted_drop = false;
    // Wait for up to 2 seconds for the XDND exchange to successfully complete, and then abort.
    while (get_timer(&active_window_response_loop_timer) < 2.0) {
        XNextEvent(display, &e);

        if (e.type == ClientMessage && e.xclient.message_type == XA_XdndStatus) {
            if ((e.xclient.data.l[1] & 1) == 0) {
                LOG_INFO("NOT ACCEPTING DROP");
                // XDND 4.5 - Active X11 window is not accepting the drop yet,
                //     so we wait and resend our XdndPosition message
                whist_sleep(50);
                xdnd_send_position(drop_x, drop_y);
            } else {
                LOG_INFO("ACCEPTING DROP");
                // Active X11 window is accepting the drop
                // XDND 6 - Send XdndDrop to active X11 window
                xdnd_send_drop();
                accepted_drop = true;
            }
        } else if (e.type == SelectionRequest) {
            LOG_INFO("SELECTION REQUEST");
            xdnd_send_selection_notify(file_uri_list, e);
        } else if (e.type == ClientMessage && e.xclient.message_type == XA_XdndFinished) {
            LOG_INFO("DONE WITH SEQUENCE");
            // The active X11 window has indicated that it is done with the drag and drop sequence,
            // so we can break
            break;
        }
    }
    XUnlockDisplay(display);

    // XDND 7 - Once we are done, we send active X11 window an XdndLeave message to indicate that
    // the XDND communication sequence is complete
    xdnd_send_leave();

    whist_unlock_mutex(xdnd_mutex);

    if (!accepted_drop) {
        // TODO: update how we log this since we are dropping many files
        LOG_WARNING(
            "XDND exchange for file ID %d was rejected by target and completed unsuccessfully", -1);
            // drop_file->id);
    }

    LOG_INFO("file drag update false 2");
    // Just to make sure that the file drag event ends
    file_drag_update(false, 0, 0, NULL);

    // TODO: update how we log this since we are dropping many files
    // LOG_INFO("XDND exchange for file ID %d complete", drop_file->id);
    LOG_INFO("XDND exchange for file ID %d complete", -1);

reset_file_drop_statics:
    if (file_uri_list) {
        free(file_uri_list);
    }
    file_uri_list = NULL;
    file_uri_list_strlen = 0;
    drop_x = 0;
    drop_y = 0;

    LOG_INFO("returning from drop file func");

    return retval;
}

const char* file_drop_prepare(int id, FileMetadata* file_metadata) {
    /*
        Create directories to house the file data and metadata, and write
        necessary metadata (for example, the file size) to the metadata file.

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
    whist_destroy_mutex(xdnd_mutex);
}

int file_drag_update(bool is_dragging, int x, int y, FileDragData* file_list) {
    /*
        Update the file drag indicator
    */

    static char* xdnd_file_list = NULL;
    static int xdnd_file_list_len = 0;
    // const char* drag_path_middle_template = "drag-drop/temp_dragging/%d_";
    const char* drag_path_middle_template = "drag-drop/temp%d/";

    const char* drag_path_template = "file:///home/whist/%s";
    // const char* create_path_template = "/home/whist/.teleport/%s";

    static bool active_file_drag = false;

    if (is_dragging) {
        if (!file_list && !xdnd_file_list) {
            return -1;
        }
        // When drag first begins, peer should send a file_list of filenames being dragged
        if (file_list) {
            const char* delimiter = "\n";
            char* strtok_context = NULL;
            char* file_list_token = strtok_r(file_list->data, delimiter, &strtok_context);
            char drag_path_middle[64];
            int id = 0;
            LOG_INFO("file list begin");
            while (file_list_token) {
                snprintf(drag_path_middle, 64, drag_path_middle_template, id);
                int file_path_end_size = strlen(drag_path_middle) + strlen(file_list_token) + 1;
                char* file_path_end = malloc(file_path_end_size);
                memset(file_path_end, 0, file_path_end_size);
                safe_strncpy(file_path_end, drag_path_middle, strlen(drag_path_middle) + 1);
                safe_strncpy(file_path_end + strlen(drag_path_middle), file_list_token, strlen(file_list_token) + 1);

                int drag_path_size = strlen(drag_path_template) + file_path_end_size + 2;
                char* drag_path = malloc(drag_path_size);
                snprintf(drag_path, drag_path_size, drag_path_template, file_path_end);

                // int create_path_size = strlen(create_path_template) + file_path_end_size + 2;
                // char* create_path = malloc(create_path_size);
                // snprintf(create_path, create_path_size, create_path_template, file_path_end);

                free(file_path_end);

                if (xdnd_file_list) {
                    xdnd_file_list = safe_realloc(xdnd_file_list, xdnd_file_list_len + drag_path_size);
                    xdnd_file_list[xdnd_file_list_len - 1] = '\n';
                } else {
                    xdnd_file_list = safe_malloc(drag_path_size);
                }
                // memset(xdnd_file_list + xdnd_file_list_len, 0, addon_size);
                // safe_strncpy(xdnd_file_list + xdnd_file_list_len, drag_path_start, strlen(drag_path_start) + 1);
                // safe_strncpy(xdnd_file_list + xdnd_file_list_len + strlen(drag_path_start),
                //     file_list_token, strlen(file_list_token) + 1);

                safe_strncpy(xdnd_file_list + xdnd_file_list_len, drag_path, strlen(drag_path) + 1);

                free(drag_path);

                // // Create temp file
                // FILE* temp_file_fd = fopen(create_path, "wb");
                // LOG_INFO("CREATE FILE: %s", create_path);
                // if (temp_file_fd) {
                //     fclose(temp_file_fd);
                // } else {
                //     LOG_ERROR("COULD NOT OPEN TEMP FILE");
                // }

                // free(create_path);

                file_list_token = strtok_r(NULL, delimiter, &strtok_context);
                xdnd_file_list_len += drag_path_size;
                id++;
            }
            LOG_INFO("file list end");
        }

        LOG_INFO("xdnd_file_list: %s", xdnd_file_list);
    }

    whist_lock_mutex(xdnd_mutex);

    if (is_dragging) {
        if (!active_file_drag) {
            // DRAG BEGINS

            LOG_INFO("DRAG BEGINS");

            // The XDND communication exchange begins. Number steps are taken from
            // https://freedesktop.org/wiki/Specifications/XDND/

            // XDND 1 - We take ownership of XdndSelection
            // XDND 2 - We send XdndEnter to active X11 window
            // Get our XDND version
            if (xdnd_own_and_send_enter() < 0) {
                whist_unlock_mutex(xdnd_mutex);
                return -1;
            }
            active_file_drag = true;

            // XDND 3 - TODO: related to the above TODO, if we support more than 3 types of
            // drag-and-droppable content, then we will need to
            //     request XdndTypeList and call
            //     XChangeProperty(disp, w, XdndTypeList, XA_ATOM, 32, PropModeReplace, (unsigned
            //     char*)&targets[0], targets.size()); beforehand. Since we only support one type right now,
            //     we can skip this step in the XDND exchange.
        }
        LOG_INFO("DRAG SEND POSITION");

        XLockDisplay(display);
        // XDND 4 - Send XdndPosition to active X11 window
        xdnd_send_position(x, y);

        LOG_INFO("before xnextevent");
        XEvent e;
        XNextEvent(display, &e);
        XUnlockDisplay(display);
        LOG_INFO("after xnextevent");

        if (e.type == ClientMessage && e.xclient.message_type == XA_XdndStatus) {
            if ((e.xclient.data.l[1] & 1) == 0) {
                // XDND 4.5 - Active X11 window is not accepting the drop yet,
                //     so we wait and resend our XdndPosition message
                // xdnd_send_position(x, y);
            }
            // Don't actually deal with any of the logistics of dropping the file here
            //    - we're using fake paths!
        } else if (e.type == SelectionRequest) {
            // When we receive a SelectionRequest from the active X11 window, we send a
            // SelectionNotify event back to
            //     the active X11 window with all of the information about the drop.
            xdnd_send_selection_notify(xdnd_file_list, e);
        } else if (e.type == ClientMessage && e.xclient.message_type == XA_XdndFinished) {
            // The active X11 window has indicated that it is done with the drag and drop sequence,
            // so we can break
            xdnd_send_leave();
        }
    } else {
        if (active_file_drag) {
            // DRAG ENDS
            LOG_INFO("DRAG ENDS");

            // XDND 7 - Once we are done, we send active X11 window an XdndLeave message to indicate that
            // the XDND communication sequence is complete
            xdnd_send_leave();

            free(xdnd_file_list);
            xdnd_file_list = NULL;
            xdnd_file_list_len = 0;
        }
    }

    whist_unlock_mutex(xdnd_mutex);

    active_file_drag = is_dragging;

    LOG_INFO("returning from file_drag_update");

    // return active_window;
    return 0;
}

#endif  // __linux__
