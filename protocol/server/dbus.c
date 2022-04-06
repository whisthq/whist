/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file dbus.c
 * @brief Contains utilities to capture and handle D-Bus messages.
 *
============================
Notes
============================

Implements logic to connect to an existing D-Bus session, listen for notifications
passed through the D-Bus, and send them via the Whist Protocol to the client.

Our approach is to register this application as a D-Bus monitor so that
we can snoop on all D-Bus messages and act on the ones we care about.
This limits us, as monitors are strictly forbidden from sending messages,
and therefore we cannot handle things like notification callbacks and
actions.

Also, certain D-Bus messages like Peer messages are automatically handled
by D-Bus, which sends replies. Because this violates the no-message rule,
D-Bus instantly closes our connection. To avoid this, we currently mark
all received D-Bus messages as handled, even though we don't handle them.
The medium-term solution is to follow the active pull request associated
with https://gitlab.freedesktop.org/dbus/dbus/-/issues/301.

TODO: We should take a look at https://codeberg.org/dnkl/fnott and implement
our D-Bus interface as a Notification server instead of a Monitor. This will
make things more robust and secure, and will allow us to handle things like
notification callbacks and actions.

TODO: As I've been working on this file, I've been thinking about
how awful the libdbus API is. Strangers on the internet seem to
agree with me, and heavily recommend using a higher-level library
like GDBus.

*/

#include <whist/core/whist.h>
#include <whist/logging/logging.h>
#include <whist/logging/log_statistic.h>
#include <whist/utils/command_line.h>

#include "dbus.h"

#ifndef __linux__

DBusHandlerContext* whist_create_dbus_handler(WhistServerState* server_state) {
    LOG_INFO("Cannot initialize D-Bus handler; feature only supported on Linux");
    return NULL;
}

void whist_destroy_dbus_handler(DBusHandlerContext* ctx) {
    LOG_INFO("Cannot destroy D-Bus handler; feature only supported on Linux");
}

#elif __linux__

/*
============================
Includes
============================
*/

#include <dbus/dbus.h>

/*
============================
Defines
============================
*/

/*
============================
Command-line options
============================
*/

// TODO: Not sure if we actually need this, or whether we can use
// `dbus_bus_get(DBUS_BUS_SESSION, ...)` instead.
static const char* dbus_addr;
COMMAND_LINE_STRING_OPTION(dbus_addr, 'b', "dbus-address", PATH_MAX,
                           "Set the address for the D-Bus connection.")

/*
============================
Private Functions
============================
*/

// Thread function
static int32_t multithreaded_handle_dbus(void* opaque);

static DBusHandlerResult notification_handler(DBusConnection* conn, DBusMessage* msg, void* opaque);
static DBusHandlerResult reply_suppressor(DBusConnection* conn, DBusMessage* msg, void* opaque);

/*
============================
Public Function Implementations
============================
*/

typedef struct DBusHandlerContext {
    WhistThread thread;
    WhistServerState* server_state;
    bool run_thread;
} DBusHandlerContext;

DBusHandlerContext* whist_create_dbus_handler(WhistServerState* server_state) {
    DBusHandlerContext* ctx = safe_malloc(sizeof(DBusHandlerContext));
    ctx->thread = whist_create_thread(multithreaded_handle_dbus, "multithreaded_handle_dbus", ctx);
    ctx->server_state = server_state;
    ctx->run_thread = true;
    return ctx;
}

void whist_destroy_dbus_handler(DBusHandlerContext* ctx) {
    if (ctx == NULL) {
        LOG_ERROR("Attempting to destroy a NULL DBusHandlerContext");
        return;
    }

    ctx->run_thread = false;
    whist_wait_thread(ctx->thread, NULL);
    free(ctx);
}

/*
============================
Private Function Implementations
============================
*/

int32_t multithreaded_handle_dbus(void* opaque) {
    DBusHandlerContext* ctx = (DBusHandlerContext*)opaque;

    // Potentially set thread priority to realtime here

    DBusError error;
    dbus_error_init(&error);

    // Private connection to prevent interference with other clients when
    // this connection becomes a monitor and starts erroring on message sends.
    DBusConnection* conn = dbus_connection_open_private(dbus_addr, &error);

    if (conn == NULL) {
        LOG_ERROR("Failed to connect to D-Bus: %s", error.message);
        dbus_error_free(&error);
        return -1;
    }

    LOG_INFO("Opened D-Bus connection to %s @ %p", dbus_addr, conn);

    dbus_connection_set_exit_on_disconnect(conn, false);

    // Register with "hello" message
    if (!dbus_bus_register(conn, &error)) {
        LOG_ERROR("Failed to register with D-Bus: %s", error.message);
        dbus_error_free(&error);
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
        return -1;
    }

    LOG_INFO("D-Bus registration of connection %p successful", conn);

    DBusMessage* monitor_msg = dbus_message_new_method_call(
        DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_MONITORING, "BecomeMonitor");
    // Arguments as per:
    // https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-become-monitor

    // I found a bug in the implementation of dbus-message.c::dbus_message_append_args_valist()!
    // Even if the array is empty, it still attempts to dereference the pointer to that empty array.
    // Hence, we need to construct an array.
    const char** empty = NULL;
    const uint32_t zero = 0;
    dbus_message_append_args(monitor_msg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &empty, 0,
                             DBUS_TYPE_UINT32, &zero, DBUS_TYPE_INVALID);
    DBusMessage* monitor_reply =
        dbus_connection_send_with_reply_and_block(conn, monitor_msg, -1, &error);
    if (monitor_reply == NULL) {
        LOG_ERROR("Failed to become a D-Bus monitor: %s", error.message);
        dbus_error_free(&error);
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
        return -1;
    }
    dbus_message_unref(monitor_msg);
    dbus_message_unref(monitor_reply);

    LOG_INFO("Became a D-Bus monitor");

    dbus_connection_add_filter(conn, notification_handler, ctx->server_state->client, NULL);
    dbus_connection_add_filter(conn, reply_suppressor, NULL, NULL);

    while (ctx->run_thread && dbus_connection_read_write_dispatch(conn, 300)) {
        // Check for read/write events with a 300 ms blocking timeout
    }

    dbus_connection_flush(conn);
    dbus_connection_close(conn);
    dbus_connection_unref(conn);

    return 0;
}

DBusHandlerResult notification_handler(DBusConnection* conn, DBusMessage* msg, void* opaque) {
    log_double_statistic(DBUS_MSGS_RECEIVED, 1.);

    if (!dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "Notify")) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    // Arguments list as per:
    // https://specifications.freedesktop.org/notification-spec/latest/ar01s09.html
    DBusError error;
    dbus_error_init(&error);

    // Again, the stupid dbus-message.c API forces us to pass in non-NULLs here, even when
    // we don't care about the values.
    const char* app_name = NULL;
    uint32_t replaces_id = 0;
    const char* app_icon = NULL;
    const char* title = NULL;
    const char* body = NULL;

    if (!dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &app_name, DBUS_TYPE_UINT32,
                               &replaces_id, DBUS_TYPE_STRING, &app_icon, DBUS_TYPE_STRING, &title,
                               DBUS_TYPE_STRING, &body, DBUS_TYPE_INVALID)) {
        LOG_ERROR("Failed to parse D-Bus message: %s", error.message);
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (title == NULL) {
        LOG_WARNING("Received a notification with a NULL title -- ignoring");
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    WhistNotification notification;
    package_notification(&notification, title, body);
    Client* client = (Client*)opaque;

    ClientLock* client_lock = client_active_trylock(client);
    if (client_lock != NULL) {
        // Send notification to client
        WhistServerMessage wmsg;
        wmsg.type = SMESSAGE_NOTIFICATION;
        wmsg.notif = notification;
        if (send_packet(&client->udp_context, PACKET_MESSAGE, &wmsg, sizeof(WhistServerMessage), 1,
                        false) < 0) {
            LOG_WARNING("Notification send failed");
        } else {
            LOG_INFO("Notification sent to client");
        }
        client_active_unlock(client_lock);
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult reply_suppressor(DBusConnection* conn, DBusMessage* msg, void* opaque) {
    // See Notes at the top of this file for details. Pretend to handle every message
    // so that D-Bus does not automatically reply to Peer messages.
    return DBUS_HANDLER_RESULT_HANDLED;
}

#endif  // defined(__linux__)
