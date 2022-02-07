/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file notifications.c
 * @brief Contains utilities to capture and send notifications to the client.

Only works on Linux! Relies on D-Bus and LibEvent.

Implements logic to connect to an existing D-Bus session, listen for notifications
passed through the D-Bus, and send them via the Whist Protocol to the client.

*/

#include "notifications.h"
#include <whist/logging/logging.h>

#ifndef __linux__

NotificationsHandler *init_notifications_handler(whist_server_state *server_state) {
    LOG_INFO("Cannot initialize notifications thread; feature only supported on Linux");
    return NULL;
}

void destroy_notifications_handler(NotificationsHandler *notifications_handler) {
    LOG_INFO("Cannot destroy notifications thread; feature only supported on Linux");
}

#elif __linux__

/*
============================
Includes
============================
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dbus/dbus.h>
#include <event.h>

#include <whist/logging/log_statistic.h>
#include <whist/core/whist.h>
#include <whist/utils/os_utils.h>
#include <whist/utils/threads.h>
#include "client.h"
#include "server_statistic.h"
#include "network.h"

/*
============================
Defines
============================
*/

typedef struct DbusCtx {
    DBusConnection *conn;
    struct event_base *evbase;
    struct event dispatch_ev;
    void *extra;
} DbusCtx;

typedef struct NotificationsHandler {
    whist_server_state *state;
    struct event_base *eb;
    WhistThread thread;
} NotificationsHandler;

/*
============================
Private Functions
============================
*/

// Thread function
static int32_t multithreaded_process_notifications(void *opaque);

// Main d-bus connection + notification handling logic
DbusCtx *dbus_init(struct event_base *eb, whist_server_state *server_state);
static void dbus_close(DbusCtx *ctx);
static DBusHandlerResult notification_handler(DBusConnection *connection, DBusMessage *message,
                                              void *user_data);

// Functions required for D-Bus listening
static dbus_bool_t become_monitor(DBusConnection *connection);

static void dispatch(int fd, short ev, void *x);
static void handle_new_dispatch_status(DBusConnection *c, DBusDispatchStatus status, void *data);

static dbus_bool_t add_watch(DBusWatch *w, void *data);
static void handle_watch(int fd, short events, void *x);
static void remove_watch(DBusWatch *w, void *data);
static void toggle_watch(DBusWatch *w, void *data);

static dbus_bool_t add_timeout(DBusTimeout *t, void *data);
static void handle_timeout(int fd, short ev, void *x);
static void remove_timeout(DBusTimeout *t, void *data);
static void toggle_timeout(DBusTimeout *t, void *data);

/*
============================
Public Function Implementations
============================
*/

NotificationsHandler *init_notifications_handler(whist_server_state *state) {
    NotificationsHandler *handler = (NotificationsHandler *)malloc(sizeof(NotificationsHandler));
    handler->state = state;
    handler->eb = event_base_new();
    handler->thread = whist_create_thread(multithreaded_process_notifications,
                                          "multithreaded_process_notifications", (void *)handler);

    return handler;
}

void destroy_notifications_handler(NotificationsHandler *handler) {
    event_base_loopbreak(handler->eb);
    whist_wait_thread(handler->thread, NULL);
    free(handler);
}

/*
============================
Private Function Implementations
============================
*/

/**
 * @brief           Implements a thread function that can be passed to `create_whist_thread`.
 *
 * @param opaque    Data passed to the function.
 * @return int32_t  0 upon successful completion, -1 if failure.
 */
int32_t multithreaded_process_notifications(void *opaque) {
    NotificationsHandler *args = (NotificationsHandler *)opaque;
    whist_server_state *state = args->state;
    struct event_base *eb = args->eb;

    whist_set_thread_priority(WHIST_THREAD_PRIORITY_REALTIME);
    whist_sleep(500);

    add_thread_to_client_active_dependents();

    DbusCtx *ctx = dbus_init(eb, state);

    if (ctx == NULL) {
        return -1;
    }

    event_base_loop(eb, 0);

    dbus_close(ctx);
    event_base_free(eb);

    return 0;
}

/**
 * @brief                           Connects to the D-Bus. Looks for the appropriate address
 *                                  in /whist/dbus_config.txt. Will fail if file does not exist.
 *
 * @param eb                        Event base that controls event handling.
 * @param server_state              Whist server state: useful for configs and packet sending.
 *
 * @return DbusCtx*                 A d-bus connection struct allocated on the heap. Make sure
 *                                  to free when program is finished.
 */
DbusCtx *dbus_init(struct event_base *eb, whist_server_state *server_state) {
    seteuid(1000);  // For d-bus to connect, set euid to that of the `whist` user

    DBusConnection *conn = NULL;
    DBusError error;
    dbus_error_init(&error);

    DbusCtx *ctx = (DbusCtx *)calloc(1, sizeof(DbusCtx));
    if (!ctx) {
        LOG_ERROR("Can't allocate DbusCtx");
        goto fail;
    }

    // Connect to appropriate d-bus daemon by reading configs
    // Scope is created to avoid "jump to label crosses initialization"
    {
        const char *dbus_addr = server_state->config->dbus_address;
        LOG_INFO("Read D-Bus address from env vars: %s", dbus_addr);

        // Use parsed address to open a private connection
        conn = dbus_connection_open_private(dbus_addr, &error);
        if (conn == NULL) {
            LOG_ERROR("D-Bus connection to %s failed: %s", dbus_addr, error.message);
            goto fail;
        }
        LOG_INFO("D-Bus connection to %s established: %p", dbus_addr, conn);
    }

    dbus_connection_set_exit_on_disconnect(conn, FALSE);

    // Register with "hello" message
    if (!dbus_bus_register(conn, &error)) {
        LOG_ERROR("D-Bus registration failed. Exiting...");
        goto fail;
    }
    LOG_INFO("D-Bus registration of connection %p successful", conn);

    // Configure watch, timeout, and filter
    ctx->conn = conn;
    ctx->evbase = eb;
    event_assign(&ctx->dispatch_ev, eb, -1, EV_TIMEOUT, dispatch, ctx);

    if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, ctx,
                                             NULL)) {
        LOG_ERROR("dbus_connection_set_watch_functions() failed");
        goto fail;
    }

    if (!dbus_connection_set_timeout_functions(conn, add_timeout, remove_timeout, toggle_timeout,
                                               ctx, NULL)) {
        LOG_ERROR("dbus_connection_set_timeout_functions() failed");
        goto fail;
    }

    if (dbus_connection_add_filter(conn, notification_handler, &server_state->client, NULL) ==
        FALSE) {
        LOG_ERROR("dbus_connection_add_filter() failed");
        goto fail;
    }

    dbus_connection_set_dispatch_status_function(conn, handle_new_dispatch_status, ctx, NULL);

    // Explicitly begin monitoring the connection
    if (!become_monitor(conn)) {
        LOG_ERROR("D-Bus monitoring failed");
        goto fail;
    }
    LOG_INFO("D-Bus monitoring started");

    seteuid(0);  // Set euid back to root

    return ctx;

fail:
    if (conn) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
    }
    if (ctx) free(ctx);

    seteuid(0);  // Set euid back to root

    return NULL;
}

/**
 * @brief       Closes the connection and frees heap-allocated memory.
 *
 * @param ctx   The connection object to close.
 */
void dbus_close(DbusCtx *ctx) {
    if (ctx && ctx->conn) {
        dbus_connection_flush(ctx->conn);
        dbus_connection_close(ctx->conn);
        dbus_connection_unref(ctx->conn);
        event_del(&ctx->dispatch_ev);
    }
    if (ctx) free(ctx);
}

/**
 * @brief                       A callback that is invoked whenever D-Bus receives an
 *                              incoming message.
 *
 * @param connection            D-Bus connection.
 * @param message               Payload of the message.
 * @param user_data             Any data the user would like to provide to this function.
 *                              We pass the Whist protocol client here.
 *
 * @return DBusHandlerResult    Always returns DBUS_HANDLER_RESULT_HANDLED.
 */
DBusHandlerResult notification_handler(DBusConnection *connection, DBusMessage *message,
                                       void *user_data) {
    // Handle case of disconnect
    if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
        LOG_ERROR("D-Bus unexpectedly disconnected");
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    const char *msg_str = dbus_message_get_member(message);
    LOG_INFO("D-Bus signal received: %s", msg_str);
    log_double_statistic(DBUS_MSGS_RECEIVED, 1.);

    if (msg_str == NULL || strcmp(msg_str, "Notify") != 0) {
        LOG_INFO("Did not detect notification body; skipping current D-Bus signal");
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    // If there is a notification, parse it out
    DBusMessageIter iter;
    dbus_message_iter_init(message, &iter);
    const char *title, *n_message;
    int str_counter = 0;
    do {
        // To recognize the argument type of the the value our iterator is on
        int type = dbus_message_iter_get_arg_type(&iter);

        // This is bad if it occurs, so we will need to leave.
        if (type == DBUS_TYPE_INVALID) {
            LOG_ERROR("Got invalid argument type from D-Bus server");
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        if (type == DBUS_TYPE_STRING) {
            // We only want the 3rd and 4th string values.
            // If there is only 1 of those values present, then the value present is the title.
            str_counter++;
            if (str_counter == 3) {
                dbus_message_iter_get_basic(&iter, &title);
            } else if (str_counter == 4) {
                dbus_message_iter_get_basic(&iter, &n_message);
            }
        }

    } while (dbus_message_iter_next(&iter));

    if (!title) {
        LOG_WARNING("D-Bus Notify signal received without a title - ignoring!");
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    // Create notification. Careful not to access OOB memory!
    WhistNotification notif;
    package_notification(&notif, title, n_message);
    LOG_INFO("WhistNotification consists of: title=%s, message=%s", notif.title, notif.message);

    // Parse protocol client from void pointer
    Client *server_state_client = (Client *)user_data;

    // Send notification over server
    WhistServerMessage wsmsg_notif;
    wsmsg_notif.type = SMESSAGE_NOTIFICATION;
    wsmsg_notif.notif = notif;
    if (server_state_client->is_active &&
        broadcast_udp_packet(server_state_client, PACKET_MESSAGE, &wsmsg_notif,
                             sizeof(WhistServerMessage), 1) >= 0) {
        LOG_INFO("Notification packet sent");
    } else {
        LOG_ERROR("Notification packet send failed");
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

/**
 * @brief               Register current connection as a monitor for the session bus.
 *
 * @param connection    D-Bus connection.
 * @return dbus_bool_t  Boolean indicating whether the operation was successful.
 */
dbus_bool_t become_monitor(DBusConnection *connection) {
    DBusError error = DBUS_ERROR_INIT;
    DBusMessage *m;
    DBusMessage *r;
    dbus_uint32_t zero = 0;
    DBusMessageIter appender, array_appender;

    m = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_MONITORING,
                                     "BecomeMonitor");
    dbus_message_iter_init_append(m, &appender);

    if (!dbus_message_iter_open_container(&appender, DBUS_TYPE_ARRAY, "s", &array_appender))
        printf("opening string array");

    if (!dbus_message_iter_close_container(&appender, &array_appender) ||
        !dbus_message_iter_append_basic(&appender, DBUS_TYPE_UINT32, &zero))
        printf("finishing arguments");

    r = dbus_connection_send_with_reply_and_block(connection, m, -1, &error);

    if (r != NULL) {
        dbus_message_unref(r);
    } else if (dbus_error_has_name(&error, DBUS_ERROR_UNKNOWN_INTERFACE)) {
        LOG_WARNING(
            "dbus-monitor: unable to enable new-style monitoring, "
            "your dbus-daemon is too old. Falling back to eavesdropping.");
        dbus_error_free(&error);
    } else {
        LOG_WARNING(
            "dbus-monitor: unable to enable new-style monitoring: "
            "%s: \"%s\". Falling back to eavesdropping.",
            error.name, error.message);
        dbus_error_free(&error);
    }

    dbus_message_unref(m);

    return (r != NULL);
}

/**
 * @brief       Reads new data from the file descriptor that we are watching.
 *
 * @param fd    (Unused)
 * @param ev    (Unused)
 * @param x     Data that the user can specify; we use it to pass the D-Bus
 *              Connection struct.
 */
void dispatch(int fd, short ev, void *x) {
    DbusCtx *ctx = (DbusCtx *)x;
    DBusConnection *c = ctx->conn;

    while (dbus_connection_get_dispatch_status(c) == DBUS_DISPATCH_DATA_REMAINS)
        dbus_connection_dispatch(c);
}

/**
 * @brief           If there is incoming D-Bus data that we have not processed,
 *                  create a new event to process it immediately.
 *
 * @param c         D-Bus connction.
 * @param status    New status.
 * @param data      Data that the user can specify; we use it to pass in the
 *                  D-Bus connection struct.
 */
void handle_new_dispatch_status(DBusConnection *c, DBusDispatchStatus status, void *data) {
    DbusCtx *ctx = (DbusCtx *)data;

    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 0,
        };
        event_add(&ctx->dispatch_ev, &tv);
    }
}

/**
 * @brief           Handles the case where a DBusWatch is marked ready for read/write.
 *                  This usually means there is data to be read.
 *
 * @param fd        File descriptor being watched.
 * @param events    Information about the event that triggered this function.
 * @param x         User data; contains the D-Bus Connection.
 */
void handle_watch(int fd, short events, void *x) {
    DbusCtx *ctx = (DbusCtx *)x;
    struct DBusWatch *watch = (struct DBusWatch *)ctx->extra;

    unsigned int flags = 0;
    if (events & EV_READ) flags |= DBUS_WATCH_READABLE;
    if (events & EV_WRITE) flags |= DBUS_WATCH_WRITABLE;

    LOG_INFO("Got d-bus watch event fd=%d watch=%p ev=%d", fd, watch, events);
    if (dbus_watch_handle(watch, flags) == FALSE) LOG_ERROR("dbus_watch_handle() failed");

    handle_new_dispatch_status(ctx->conn, DBUS_DISPATCH_DATA_REMAINS, ctx);
}

/**
 * @brief                   Indicates to libevent that we would like to monitor a UNIX file
 *                          descriptor. In our case this descriptor is a D-Bus address.
 *
 * @param w                 Contains information about the descriptor we want to watch.
 * @param data              User data; contains the D-Bus connection.
 * @return dbus_bool_t      TRUE for success, FALSE for failure.
 */
dbus_bool_t add_watch(DBusWatch *w, void *data) {
    if (!dbus_watch_get_enabled(w)) return TRUE;

    DbusCtx *ctx = (DbusCtx *)data;
    ctx->extra = w;

    int fd = dbus_watch_get_unix_fd(w);
    unsigned int flags = dbus_watch_get_flags(w);
    short cond = EV_PERSIST;
    if (flags & DBUS_WATCH_READABLE) cond |= EV_READ;
    if (flags & DBUS_WATCH_WRITABLE) cond |= EV_WRITE;

    struct event *event = event_new(ctx->evbase, fd, cond, handle_watch, ctx);
    if (!event) return FALSE;

    event_add(event, NULL);

    dbus_watch_set_data(w, event, NULL);

    LOG_INFO("Added d-bus watch fd=%d watch=%p cond=%d", fd, w, cond);
    return TRUE;
}

/**
 * @brief           Function to remove watch.
 *
 * @param w         The watch event we want to remove.
 * @param data      User data; contains the D-Bus Connection.
 */
void remove_watch(DBusWatch *w, void *data) {
    struct event *event = (struct event *)dbus_watch_get_data(w);

    if (event) event_free(event);

    dbus_watch_set_data(w, NULL, NULL);

    LOG_INFO("Removed d-bus watch watch=%p", w);
}

/**
 * @brief           Toggles watch/unwatch of the specified event.
 *
 * @param w         The event whose status we want to toggle.
 * @param data      User data; contains the D-Bus Connection.
 */
void toggle_watch(DBusWatch *w, void *data) {
    LOG_INFO("Toggling d-bus watch watch=%p", w);

    if (dbus_watch_get_enabled(w))
        add_watch(w, data);
    else
        remove_watch(w, data);
}

/**
 * @brief       This function is called when a timeout occurs. It then
 *              invokes `dbus_timeout_handle`, which deals with this situation.
 *
 * @param fd    (Unused)
 * @param ev    (Unused)
 * @param x     User data; contains the D-Bus Connection.
 */
void handle_timeout(int fd, short ev, void *x) {
    DbusCtx *ctx = (DbusCtx *)x;
    DBusTimeout *t = (DBusTimeout *)ctx->extra;

    LOG_INFO("Got d-bus handle timeout event %p", t);

    dbus_timeout_handle(t);
}

/**
 * @brief                   Specify a new timeout specification.
 *
 * @param t                 The timeout to add.
 * @param data              User data; contains the D-Bus Connection.
 * @return dbus_bool_t      TRUE on success, FALSE on failure.
 */
dbus_bool_t add_timeout(DBusTimeout *t, void *data) {
    DbusCtx *ctx = (DbusCtx *)data;

    if (!dbus_timeout_get_enabled(t)) return TRUE;

    LOG_INFO("Adding d-bus timeout %p", t);

    struct event *event = event_new(ctx->evbase, -1, EV_TIMEOUT | EV_PERSIST, handle_timeout, t);
    if (!event) {
        LOG_ERROR("Failed to allocate new event for timeout");
        return FALSE;
    }

    int ms = dbus_timeout_get_interval(t);
    struct timeval tv = {
        .tv_sec = ms / 1000,
        .tv_usec = (ms % 1000) * 1000,
    };
    // event_add(event, &tv);

    dbus_timeout_set_data(t, event, NULL);

    return TRUE;
}

/**
 * @brief           Remove timeout event.
 *
 * @param t         Timeout to remove.
 * @param data      User data; contains the D-Bus Connection.
 */
void remove_timeout(DBusTimeout *t, void *data) {
    struct event *event = (struct event *)dbus_timeout_get_data(t);

    LOG_INFO("Removing d-bus timeout %p", t);

    event_free(event);

    dbus_timeout_set_data(t, NULL, NULL);
}

/**
 * @brief           Toggles timeout event between on/off.
 *
 * @param t         Timeout event to toggle.
 * @param data      User data; contains the D-Bus Connection.
 */
void toggle_timeout(DBusTimeout *t, void *data) {
    LOG_INFO("Toggling d-bus timeout %p", t);

    if (dbus_timeout_get_enabled(t))
        add_timeout(t, data);
    else
        remove_timeout(t, data);
}

#endif  // __linux__
