#include "notifications.h"

Client *client = NULL;

static dbus_bool_t
become_monitor (DBusConnection *connection) {
  DBusError error = DBUS_ERROR_INIT;
  DBusMessage *m;
  DBusMessage *r;
  dbus_uint32_t zero = 0;
  DBusMessageIter appender, array_appender;

  m = dbus_message_new_method_call (DBUS_SERVICE_DBUS,
      DBUS_PATH_DBUS, DBUS_INTERFACE_MONITORING, "BecomeMonitor");

  if (m == NULL)
    printf("becoming a monitor");

  dbus_message_iter_init_append (m, &appender);

  if (!dbus_message_iter_open_container (&appender, DBUS_TYPE_ARRAY, "s",
        &array_appender))
    printf("opening string array");

  if (!dbus_message_iter_close_container (&appender, &array_appender) ||
      !dbus_message_iter_append_basic (&appender, DBUS_TYPE_UINT32, &zero))
    printf("finishing arguments");

  r = dbus_connection_send_with_reply_and_block (connection, m, -1, &error);

  if (r != NULL)
    {
      dbus_message_unref (r);
    }
  else if (dbus_error_has_name (&error, DBUS_ERROR_UNKNOWN_INTERFACE))
    {
      fprintf (stderr, "dbus-monitor: unable to enable new-style monitoring, "
          "your dbus-daemon is too old. Falling back to eavesdropping.\n");
      dbus_error_free (&error);
    }
  else
    {
      fprintf (stderr, "dbus-monitor: unable to enable new-style monitoring: "
          "%s: \"%s\". Falling back to eavesdropping.\n",
          error.name, error.message);
      dbus_error_free (&error);
    }

  dbus_message_unref (m);

  return (r != NULL);
}

/**
 * @brief This function is called asynchronously whenever we call dbus_dispatch_read
 * if there are notifications in our notification queue
 *
 * @param connection connection to the server dbus
 * @param message notification message from dbus
 * @param user_data this is not used
 * @return DBusHandlerResult this is not used
 */
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

    /** There are many different interface member messages we will
     * recieve. However, only the messages with member "Notify"
     * are the ones having any info regarding the notifications,
     * so those are the only ones we care about */
    if (msg_str == NULL || strcmp(msg_str, "Notify") != 0) {
      printf("Did not detect notification body\n");
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    /** Now, at this point, we have a message from the dbus and we
     * know it's a notification. Time to parse...
     *
     * The notification info comes in a strange DBUS format that is
     * not JSON or anything remotely recognizable. Luckily, DBUS
     * gives APIs to parse this.
     *
     * We will need to initialize a message iterator and then iterate
     * through the message for the info we care about.
     *
     * At the time of writing this, we only care about the Title and
     * the message of the notification, which are both strings.
     *
     * Furthermore, those are the 3rd and 4th string members (no labeling
     * is given from dbus server), so really we only care about those.
     * */
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

    // Construct packet and send to client
    printf("Title: %s, Message: %s\n", title, n_message);

    WhistNotification notif;
    strcpy(notif.title, title);
    strcpy(notif.message, n_message);

    if (client->is_active && send_packet(&client->udp_context, PACKET_NOTIFICATION, &notif, sizeof(WhistNotification), 0) >= 0) {
        printf("Packet sent\n");
    } else{
        printf("Packet send failed\n");
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

int init_notif_watcher(Client *server_client) {
    seteuid(1000);

    client = server_client;

    DBusError error;
    dbus_error_init(&error);

    // Connect to appropriate d-bus daemon
    const char *config_file = "/whist/dbus_config.txt";
    FILE *f_dbus_info = fopen(config_file, "r");
    if (f_dbus_info == NULL) {
      printf("Required d-bus configuration file %s not found!\n", config_file);
      return -1;
    }

    char dbus_info[120];
    fscanf(f_dbus_info, "%s", dbus_info);
    fclose(f_dbus_info);
    printf("%s contains: %s\n", config_file, dbus_info);

    // This parsing strategy depends on the formatting of `config_file`
    size_t start_idx = (strchr(dbus_info, (int) '\'') - dbus_info) + 1;
    size_t final_len = strlen(dbus_info) - start_idx - 2;

    char *dbus_addr = malloc((final_len + 1) * sizeof(char));
    strncpy(dbus_addr, dbus_info + start_idx, final_len);
    dbus_addr[final_len] = '\0';

    DBusConnection *connection = dbus_connection_open(dbus_addr, &error);
    if (connection == NULL) {
        printf("Connection to %s failed: %s\n", dbus_addr, error.message);
        return -1;
    }
    printf("Connection to %s established: %p\n", dbus_addr, connection);
    free(dbus_addr);

    // Register with "hello" message
    if (!dbus_bus_register(connection, &error)) {
        printf("Registration failed: %s\n", error.message);
        return -1;
    }
    printf("Registration of connection %p successful\n", connection);

    // Set up initial filter
    if (!dbus_connection_add_filter (connection, notification_handler, NULL, NULL)) {
        printf("Basic filter-add failed!\n");
        return -1;
    }
    printf("Message handler registered\n");

    // Begin monitoring d-bus
    if (!become_monitor(connection)) {
        printf("Monitoring failed!\n");
        return -1;
    }
    printf("Monitoring started. Listening...\n");

    // while (dbus_connection_read_write_dispatch(connection, -1));

    dbus_error_free(&error);
    seteuid(0);
    
    return 0;
}
