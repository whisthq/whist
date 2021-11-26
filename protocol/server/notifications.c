/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file notifications.c
 * @brief This file contains the code that monitors and processes notifications
============================
Usage
============================

TODO

*/

/*
============================
Includes
============================
*/

#include "notifications.h"
#ifndef _WIN32

/*
============================
HAND-OFF
============================

Please replace this entire file with the code below when the dbus is installed
The Dbus method is way better, faster, and is browser independent

// #include "notifications.h"

// DBusConnection *connection;

// /**
//  * @brief There is a BecomeMonitor method that the dbus allows which will
//  * enable our bus connection to eavesdrop into any other server-client
//  * connection that is present in the DBus.
//  *
//  * This is awesome, as we would like to monitor the calls between the dbus server
//  * and the org.freedesktop.Notifications interface
//  *
//  * @param connection dbus connection
//  * @param numFilters number of filters
//  * @param filters The filters that will narrow our monitoring scope
//  * @return dbus_bool_t true if success, false if failure
//  */
// static dbus_bool_t become_monitor(DBusConnection *connection, int numFilters,
//                                   const char *const *filters) {
//     DBusError error = DBUS_ERROR_INIT;
//     DBusMessage *m;
//     DBusMessage *r;
//     int i;
//     dbus_uint32_t zero = 0;
//     DBusMessageIter appender, array_appender;

//     /* Creates the initial message we are going to send to the DBus to become
//         a monitor */
//     m = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_MONITORING,
//                                      "BecomeMonitor");

//     if (m == NULL) {
//         LOG_FATAL("BecomeMonitor method does not exist on this machine\n");
//         return false;
//     }

//     /* We will need to add a filter to our original message so we only monitor
//     signals from Notifications*/
//     dbus_message_iter_init_append(m, &appender);

//     /* BecomeMonitor  requires two arguments:
//        1. an array of match strings (calling them filters here)
//        2. the number 0 (note, I didn't make this API, it just requires you put a zero here, idk why)
//     */

//     /* To create the array parameter, we will open a container (AKA, an array) */
//     if (!dbus_message_iter_open_container(&appender, DBUS_TYPE_ARRAY, "s", &array_appender)) {
//         LOG_FATAL("Could not open message container\n");
//         return false;
//     }

//     /* Time to add our filters into that array */
//     for (i = 0; i < numFilters; i++) {
//         if (!dbus_message_iter_append_basic(&array_appender, DBUS_TYPE_STRING, &filters[i])) {
//             LOG_FATAL("Could not add in filter to message array\n");
//             return false;
//         }
//     }

//     /* Now close the container (AKA close the array) and add that silly zero */
//     if (!dbus_message_iter_close_container(&appender, &array_appender) ||
//         !dbus_message_iter_append_basic(&appender, DBUS_TYPE_UINT32, &zero)) {
//         LOG_FATAL("Could not close the array OR append that 0 to the message\n");
//         return false;
//     }

//     /* Now, send the message over to the DBus that we want to become a monitor
//     and wait for the reply for error purposes.

//     This is a blocking method, so technically we can wait here forever.

//     TODO(abecohen): Find a non-blocking alternative*/
//     r = dbus_connection_send_with_reply_and_block(connection, m, -1, &error);

//     /* We do not care about the reply, so let's dereference it */
//     if (r != NULL) {
//         dbus_message_unref(r);
//         /* The case where things go wrong and we know why */
//     } else if (dbus_error_has_name(&error, DBUS_ERROR_UNKNOWN_INTERFACE)) {
//         LOG_FATAL("DBUS daemon is too old to have BecomeMonitor interface\n");
//         dbus_error_free(&error);
//         return false;
//         /* The case where things go wrong and no-one knows why */
//     } else {
//         LOG_FATAL("BecomeMonitor has failed for mysterious reason\n");
//         dbus_error_free(&error);
//         return false;
//     }

//     /* We will not ever use this message again, so free it */
//     dbus_message_unref(m);

//     return (r != NULL);
// }

// /**
//  * @brief This function is called asynchronously whenever we call dbus_dispatch_read
//  * if there are notifications in our notification queue
//  *
//  * @param connection connection to the server dbus
//  * @param message notification message from dbus
//  * @param user_data this is not used
//  * @return DBusHandlerResult this is not used
//  */
// static DBusHandlerResult notification_handler(DBusConnection *connection, DBusMessage *message,
//                                               void *user_data) {
//     /** If we somehow are disconnected from the dbus, something bad
//      * happened and we cannot recover */
//     if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
//         LOG_FATAL("DBUS unexpectedly disconnected\n");
//         return -1;
//     }

//     /** There are many different interface member messages we will
//      * recieve. However, only the messages with member "Notify"
//      * are the ones having any info regarding the notifications,
//      * so those are the only ones we care about */
//     if (strcmp(dbus_message_get_member(message), "Notify") != 0) return DBUS_HANDLER_RESULT_HANDLED;

//     /** Now, at this point, we have a message from the dbus and we
//      * know it's a notification. Time to parse...
//      *
//      * The notification info comes in a strange DBUS format that is
//      * not JSON or anything remotely recognizable. Luckily, DBUS
//      * gives APIs to parse this.
//      *
//      * We will need to initialize a message iterator and then iterate
//      * through the message for the info we care about.
//      *
//      * At the time of writing this, we only care about the Title and
//      * the message of the notification, which are both strings.
//      *
//      * Furthermore, those are the 3rd and 4th string members (no labeling
//      * is given from dbus server), so really we only care about those.
//      *
//      * TODO(abecohen): Get the audio data and icon pixmaps.
//      * */
//     DBusMessageIter iter;
//     dbus_message_iter_init(message, &iter);
//     char *title, *n_message;
//     int str_counter = 0;
//     do {
//         // To recognize the argument type of the the value our iterator is on
//         int type = dbus_message_iter_get_arg_type(&iter);

//         // This is bad if it occurs, so we will need to leave.
//         if (type == DBUS_TYPE_INVALID) {
//             LOG_FATAL("Got invalid argument type from DBUS server\n");
//             return -1;
//         }

//         // Finally, the goods we are looking for
//         if (type == DBUS_TYPE_STRING) {
//             // As stated above, we only care about the 3rd and 4th string values
//             // NOTE: If there is only 1 of those values present,
//             // then the value present is the title.
//             str_counter++;
//             if (str_counter == 3) {
//                 dbus_message_iter_get_basic(&iter, &title);
//             } else if (str_counter == 4) {
//                 dbus_message_iter_get_basic(&iter, &n_message);
//             }
//         }

//     } while (dbus_message_iter_next(&iter));  // This just keeps iterating until it's all over

//     // Here we would construct the packet and send it
//     LOG_INFO("Title: %s, Message: %s\n", title, n_message);

//     return DBUS_HANDLER_RESULT_HANDLED;
// }

// int inititialze_notification_dbus() {
//     DBusError error;

//     /** ignore below up to line 210, I'm refactoring */

//     int i = 1, j = 0;

//     char **filters = NULL;

//     unsigned int filter_len;
//     /* Prepend a rule (and a comma) to enable the monitor to eavesdrop.
//      * Prepending allows the user to add eavesdrop=false at command line
//      * in order to disable eavesdropping when needed */
//     filter_len = strlen(EAVESDROPPING_RULE) + 1 + strlen(NOTIFICATION_FILTER) + 1;
//     filters = (char **)safe_realloc(filters, sizeof(char *));
//     filters[j] = (char *)safe_malloc(filter_len);
//     snprintf(filters[j], filter_len, "%s,%s", EAVESDROPPING_RULE, NOTIFICATION_FILTER);

//     /** ignore up to line 194, I'm refactoring */

//     dbus_error_init(&error);
//     connection = dbus_bus_get(DBUS_BUS_SESSION, &error);

//     /* Stolen from dbus-monitor.c, don't understand it, but keeping it
//      * Receive o.fd.Peer messages as normal messages, rather than having
//      * libdbus handle them internally, which is the wrong thing for
//      * a monitor */
//     dbus_connection_set_route_peer_messages(connection, TRUE);

//     if (!dbus_connection_add_filter(connection, notification_handler,
//                                     _DBUS_INT_TO_POINTER(BINARY_MODE_NOT), NULL)) {
//         LOG_FATAL("Could not add filter to dbus connection\n");
//         return -1;
//     }

//     if (become_monitor(connection, 1, (const char *const *)filters)) {
//         LOG_FATAL("BecomeMonitor method failed\n");
//         return -1;
//     }

//     return 0;
// }


// void process_notification_queue() {
//     dbus_connection_read_write_dispatch(connection, 10);
// }

/*
============================
END HAND-OFF
============================
*/

/*
============================
Private data
============================
*/

// File and watch descriptors for inotify
int notifications_log_fd;
int notifications_watch_descriptor;

// Stealing comments from inotify manual...
/* Some systems cannot read integer variables if they are not
              properly aligned. On other systems, incorrect alignment may
              decrease performance. Hence, the buffer used for reading from
              the inotify file descriptor should have the same alignment as
              struct inotify_event. */
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
char buf[EVENT_BUF_LEN];
const struct inotify_event *event;

/*
============================
Public function implementations
============================
*/


int inititialze_notification_watcher() {

    // Initialize using inotify_init1 so we can set the fd as non-blocking
    notifications_log_fd = inotify_init1(IN_NONBLOCK);
    if (notifications_log_fd < 0) {
        LOG_FATAL(
            "notifications::initialize_notification_watcher: failed to initialize notification "
            "watcher");
        return -1;
    }

    // Initialize the watch descriptor to tell us whenever the log file has been written to
    notifications_watch_descriptor =
        inotify_add_watch(notifications_log_fd, CHROMIUM_NOTIFICATION_LOG, IN_MODIFY);
    if (notifications_watch_descriptor < 0) {
        LOG_INFO("NOTIFCATIONS: Errno: %s", strerror(errno));
        LOG_FATAL(
            "notifications::initialize_notification_watcher: failed to add watch to notification "
            "log");
        return -1;
    }

    LOG_INFO("NOTIFCATIONS: Sucessfully initialzed the notification watcher");

    return 0;
}

int check_for_notifications() {
    // Since this is non-blocking, if there are no notifications, this
    // will return a len of 0, so we can use this instead of polling
    ssize_t len = read(notifications_log_fd, buf, sizeof(buf));

    // EAGAIN is set when we call read and there's no data. Since this is
    // non-blocking (as set in init_inotify1), this is fine behaviour
    if (len == -1 && errno != EAGAIN) {
        LOG_FATAL("notifications::notifications_available: read failed!");
        return -1;
    }

    // Case where there is no new data
    if (len <= 0) return 0;

    LOG_INFO("NOTIFICATIONS: Recieved!");
    // Case we have data
    return 1;
}

int process_notifications(Client client) {
    LOG_INFO("NOTIFICATIONS: sending notification");
    FractalNotification notification = {.title = "Test title", .message = "Test message"};
    if (client.is_active) {
        send_packet(&client.udp_context, PACKET_NOTIFICATION, &notification,
                    sizeof(FractalNotification), 0);
    }
    return 0;
}



#endif
