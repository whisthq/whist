/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.c
 * @brief Contains utilities to capture and send notifications to the client.
============================
Usage
============================

TODO(kmeng01) write some summary thing here

*/

/*
============================
Includes
============================
*/

#include "notifications.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <dbus/dbus.h>
#include <event.h>

#include <whist/logging/logging.h>
#include <whist/core/whist.h>
#include <whist/utils/whist_notification.h>
#include <whist/utils/threads.h>
#include "client.h"
#include "network.h"
#include "state.h"

/*
============================
Globals
============================
*/

struct dbus_ctx {
    DBusConnection *conn;
    struct event_base *evbase;
    struct event dispatch_ev;
    void *extra;
};

Client *server_state_client = NULL;

/*
============================
Private Functions
============================
*/

// TODO(kevin) fill this in

/*
============================
Private Function Implementations
============================
*/

static dbus_bool_t become_monitor(DBusConnection *connection) {
    DBusError error = DBUS_ERROR_INIT;
    DBusMessage *m;
    DBusMessage *r;
    dbus_uint32_t zero = 0;
    DBusMessageIter appender, array_appender;

    m = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_MONITORING,
                                     "BecomeMonitor");

    if (m == NULL) printf("becoming a monitor");

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
        fprintf(stderr,
                "dbus-monitor: unable to enable new-style monitoring, "
                "your dbus-daemon is too old. Falling back to eavesdropping.\n");
        dbus_error_free(&error);
    } else {
        fprintf(stderr,
                "dbus-monitor: unable to enable new-style monitoring: "
                "%s: \"%s\". Falling back to eavesdropping.\n",
                error.name, error.message);
        dbus_error_free(&error);
    }

    dbus_message_unref(m);

    return (r != NULL);
}

static void dispatch(int fd, short ev, void *x) {
    struct dbus_ctx *ctx = x;
    DBusConnection *c = ctx->conn;

    printf("dispatching\n");

    while (dbus_connection_get_dispatch_status(c) == DBUS_DISPATCH_DATA_REMAINS)
        dbus_connection_dispatch(c);
}

static void handle_new_dispatch_status(DBusConnection *c, DBusDispatchStatus status, void *data) {
    struct dbus_ctx *ctx = data;

    printf("new dbus dispatch status: %d\n", status);

    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 0,
        };
        event_add(&ctx->dispatch_ev, &tv);
    }
}

static void handle_watch(int fd, short events, void *x) {
    struct dbus_ctx *ctx = x;
    struct DBusWatch *watch = ctx->extra;

    unsigned int flags = 0;
    if (events & EV_READ) flags |= DBUS_WATCH_READABLE;
    if (events & EV_WRITE) flags |= DBUS_WATCH_WRITABLE;
    /*if (events & HUP)
        flags |= DBUS_WATCH_HANGUP;
    if (events & ERR)
        flags |= DBUS_WATCH_ERROR;*/

    printf("got dbus watch event fd=%d watch=%p ev=%d\n", fd, watch, events);
    if (dbus_watch_handle(watch, flags) == FALSE) printf("dbus_watch_handle() failed\n");

    handle_new_dispatch_status(ctx->conn, DBUS_DISPATCH_DATA_REMAINS, ctx);
}

static dbus_bool_t add_watch(DBusWatch *w, void *data) {
    if (!dbus_watch_get_enabled(w)) return TRUE;

    struct dbus_ctx *ctx = data;
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

    printf("added dbus watch fd=%d watch=%p cond=%d\n", fd, w, cond);
    return TRUE;
}

static void remove_watch(DBusWatch *w, void *data) {
    struct event *event = dbus_watch_get_data(w);

    if (event) event_free(event);

    dbus_watch_set_data(w, NULL, NULL);

    printf("removed dbus watch watch=%p\n", w);
}

static void toggle_watch(DBusWatch *w, void *data) {
    printf("toggling dbus watch watch=%p\n", w);

    if (dbus_watch_get_enabled(w))
        add_watch(w, data);
    else
        remove_watch(w, data);
}

static void handle_timeout(int fd, short ev, void *x) {
    struct dbus_ctx *ctx = x;
    DBusTimeout *t = ctx->extra;

    printf("got dbus handle timeout event %p\n", t);

    dbus_timeout_handle(t);
}

static dbus_bool_t add_timeout(DBusTimeout *t, void *data) {
    struct dbus_ctx *ctx = data;

    if (!dbus_timeout_get_enabled(t)) return TRUE;

    printf("adding timeout %p\n", t);

    struct event *event = event_new(ctx->evbase, -1, EV_TIMEOUT | EV_PERSIST, handle_timeout, t);
    if (!event) {
        printf("failed to allocate new event for timeout\n");
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

static void remove_timeout(DBusTimeout *t, void *data) {
    struct event *event = dbus_timeout_get_data(t);

    printf("removing timeout %p\n", t);

    event_free(event);

    dbus_timeout_set_data(t, NULL, NULL);
}

static void toggle_timeout(DBusTimeout *t, void *data) {
    printf("toggling timeout %p\n", t);

    if (dbus_timeout_get_enabled(t))
        add_timeout(t, data);
    else
        remove_timeout(t, data);
}

static DBusHandlerResult notification_handler(DBusConnection *connection, DBusMessage *message,
                                              void *user_data) {
    /** If we somehow are disconnected from the dbus, something bad
     * happened and we cannot recover */
    if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
        printf("D-Bus unexpectedly disconnected. Exiting...\n");
        return -1;
    }
    const char *msg_str = dbus_message_get_member(message);
    printf("\nSignal received: %s\n", msg_str);

    if (msg_str == NULL || strcmp(msg_str, "Notify") != 0) {
        printf("Did not detect notification body\n");
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    DBusMessageIter iter;
    dbus_message_iter_init(message, &iter);
    const char *title, *n_message;
    int str_counter = 0;
    do {
        // To recognize the argument type of the the value our iterator is on
        int type = dbus_message_iter_get_arg_type(&iter);

        // This is bad if it occurs, so we will need to leave.
        if (type == DBUS_TYPE_INVALID) {
            printf("Got invalid argument type from D-Bus server\n");
            return -1;
        }

        // Finally, the goods we are looking for
        if (type == DBUS_TYPE_STRING) {
            // As stated above, we only care about the 3rd and 4th string values
            // NOTE: If there is only 1 of those values present,
            // then the value present is the title.
            str_counter++;
            if (str_counter == 3) {
                dbus_message_iter_get_basic(&iter, &title);
            } else if (str_counter == 4) {
                dbus_message_iter_get_basic(&iter, &n_message);
            }
        }

    } while (dbus_message_iter_next(&iter));  // This just keeps iterating until it's all over

    WhistNotification notif;
    strcpy(notif.title, title);
    strcpy(notif.message, n_message);
    LOG_INFO("WhistNotification consists of: title=%s, message=%s\n", notif.title, notif.message);

    if (server_state_client->is_active &&
        send_packet(&server_state_client->udp_context, PACKET_NOTIFICATION, &notif,
                    sizeof(WhistNotification), 0) >= 0) {
        LOG_INFO("Notification packet sent");
    } else {
        LOG_INFO("Notification packet send failed");
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

struct dbus_ctx *dbus_init(struct event_base *eb, Client *init_server_state_client) {
    seteuid(1000);  // For d-bus to connect, set euid to that of the `whist` user

    server_state_client = init_server_state_client;

    DBusConnection *conn = NULL;
    DBusError error;
    dbus_error_init(&error);

    struct dbus_ctx *ctx = calloc(1, sizeof(struct dbus_ctx));
    if (!ctx) {
        printf("can't allocate dbus_ctx\n");
        goto fail;
    }

    // Connect to appropriate d-bus daemon
    const char *config_file = "/whist/dbus_config.txt";
    FILE *f_dbus_info = fopen(config_file, "r");
    if (f_dbus_info == NULL) {
        printf("Required d-bus configuration file %s not found!\n", config_file);
        goto fail;
    }

    char dbus_info[120];
    fscanf(f_dbus_info, "%s", dbus_info);
    fclose(f_dbus_info);
    printf("%s contains: %s\n", config_file, dbus_info);

    // This parsing strategy depends on the formatting of `config_file`
    size_t start_idx = (strchr(dbus_info, (int)'\'') - dbus_info) + 1;
    size_t final_len = strlen(dbus_info) - start_idx - 2;

    char *dbus_addr = malloc((final_len + 1) * sizeof(char));
    strncpy(dbus_addr, dbus_info + start_idx, final_len);
    dbus_addr[final_len] = '\0';

    conn = dbus_connection_open_private(dbus_addr, &error);
    if (conn == NULL) {
        printf("Connection to %s failed: %s\n", dbus_addr, error.message);
        goto fail;
    }
    printf("Connection to %s established: %p\n", dbus_addr, conn);
    free(dbus_addr);

    dbus_connection_set_exit_on_disconnect(conn, FALSE);

    // Register with "hello" message
    if (!dbus_bus_register(conn, &error)) {
        printf("Registration failed. Exiting...\n");
        goto fail;
    }
    printf("Registration of connection %p successful\n", conn);

    ctx->conn = conn;
    ctx->evbase = eb;
    event_assign(&ctx->dispatch_ev, eb, -1, EV_TIMEOUT, dispatch, ctx);

    if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, ctx,
                                             NULL)) {
        printf("dbus_connection_set_watch_functions() failed\n");
        goto fail;
    }

    if (!dbus_connection_set_timeout_functions(conn, add_timeout, remove_timeout, toggle_timeout,
                                               ctx, NULL)) {
        printf("dbus_connection_set_timeout_functions() failed\n");
        goto fail;
    }

    if (dbus_connection_add_filter(conn, notification_handler, ctx, NULL) == FALSE) {
        printf("dbus_connection_add_filter() failed\n");
        goto fail;
    }

    dbus_connection_set_dispatch_status_function(conn, handle_new_dispatch_status, ctx, NULL);

    if (!become_monitor(conn)) {
        printf("Monitoring failed. Exiting...\n");
        goto fail;
    }
    printf("Monitoring started!\n");

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

void dbus_close(struct dbus_ctx *ctx) {
    if (ctx && ctx->conn) {
        dbus_connection_flush(ctx->conn);
        dbus_connection_close(ctx->conn);
        dbus_connection_unref(ctx->conn);
        event_del(&ctx->dispatch_ev);
    }
    if (ctx) free(ctx);
}

/*
============================
Public Function Implementations
============================
*/

int32_t listen_and_process_notifications(void *opaque) {
    whist_server_state *state = (whist_server_state *)opaque;
    whist_set_thread_priority(WHIST_THREAD_PRIORITY_REALTIME);
    whist_sleep(500);

    add_thread_to_client_active_dependents();

    struct event_base *eb = event_base_new();
    struct dbus_ctx *ctx = dbus_init(eb, &state->client);

    if (ctx == NULL) {
        return -1;
    }

    event_base_loop(eb, 0);

    dbus_close(ctx);
    event_base_free(eb);

    return 0;
}
