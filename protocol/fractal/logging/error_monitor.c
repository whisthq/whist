/**
 * Copyright Fractal Computers, Inc. 2021
 * @file error_monitor.c
 * @brief This file contains the functions to configure and report breadcrumbs
 *        and error events to tools such as Sentry.
============================
Usage
============================
The error monitor needs to have an environment configured before it can be
started up. This environment should be development/staging/production, and is
passed in as a command-line parameter. Once the value is known, we may call
`error_monitor_set_environment()` to configure it.

If no environment is set, the error monitor will fail to initialize. This is
because we don't want it to complain in personal setups and manual connections
when developing, but it's important to note the side-effect that we will not
report to the error monitoring service if environment isn't passed in.

To initialize the error monitor, we call `error_monitor_initialize()`. After
doing so, we can configure the error logging metadata with
`error_monitor_set_username()` and `error_monitor_set_connection_id()`.

At this point, calling `error_monitor_log_breadcrumb()` and
`error_monitor_log_error()` will report a trace of non-error events and report
a detailed breakdown for error-level events to the error monitor service,
respectively. Importantly, *you should almost never be calling this function by
itself*. Instead, simply use the `LOG_*()` functions from logging.h, which will
automatically send error monitor breadcrumbs and error reports as needed.

Because of this integration with our logging setup, we run into race conditions
in `error_monitor_shutdown()`, which may cause us to fail to report our last
few breadcrumbs and error events. In order to avoid this, we must call
`error_monitor_shutdown()` after calling `destroy_logger()`, to allow for any
pending error monitor log calls to be handled. Eventually, we should set up
a more robust solution for synchronizing these calls.
*/

/*
============================
Includes
============================
*/

#include <sentry/sentry.h>
#include <fractal/core/fractal.h>
#include "error_monitor.h"

/*
============================
Public Function Implementations
============================
*/

#define SENTRY_DSN "https://8e1447e8ea2f4ac6ac64e0150fa3e5d0@o400459.ingest.sentry.io/5373388"

static char error_monitor_environment[FRACTAL_ARGS_MAXLEN + 1];
static char error_monitor_environment_set = false;
static bool error_monitor_initialized = false;

void error_monitor_set_environment(char *environment) {
    /*
        Configures the error monitor to use the provided environment.

        Arguments:
            environment (char*): The environment to set: should be one of
                development/staging/production.

        Returns:
            None

        Note: This should only be called before calling
            `error_monitor_initialize()`.
    */
    if (error_monitor_initialized) {
        LOG_WARNING("Attempting to set the environment to %s after starting the error monitor!",
                    environment);
        return;
    }

    if (strcmp(environment, "production") == 0 || strcmp(environment, "staging") == 0 ||
        strcmp(environment, "development") == 0) {
        safe_strncpy(error_monitor_environment, environment, sizeof(error_monitor_environment));
        error_monitor_environment_set = true;
    } else {
        LOG_WARNING("Invalid environment %s set! Ignoring it.", environment);
    }
}

void error_monitor_set_username(char *username) {
    /*
        Configures the username tag for error monitor reports.

        Arguments:
            username (char*): The username to set. Setting NULL or "None" will
                unset the username instead.

        Returns:
            None

        Note: This should only be called after calling
            `error_monitor_initialize()`.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_initialized) return;

    // Set the user to username, or remove the user as a default.
    // Also remove the user if the username is "None".
    if (username && strcmp(username, "None")) {
        sentry_value_t user = sentry_value_new_object();
        sentry_value_set_by_key(user, "email", sentry_value_new_string(username));
        // Sentry doesn't document it, but this will free user.
        sentry_set_user(user);
    } else {
        sentry_remove_user();
    }
}

void error_monitor_set_connection_id(int id) {
    /*
        Configures the connection id tag for error monitor reports.

        Arguments:
            id (int): The connection id to set. Setting -1 will set the
                connection id to "waiting".

        Returns:
            None

        Note: This should only be called after calling
            `error_monitor_initialize()`.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_initialized) return;

    char connection_id[50];

    // Set the connection_id to the string of the id, or "waiting" as a default.
    if (id >= 0) {
        snprintf(connection_id, sizeof(connection_id), "%d", id);
    } else {
        safe_strncpy(connection_id, "waiting", sizeof(connection_id));
    }

    sentry_set_tag("connection_id", connection_id);
}

void error_monitor_initialize(bool is_client) {
    /*
        Initializes the error monitor.

        Arguments:
            is_client (bool): True if this is the client protocol, else false.

        Returns:
            None

        Note: This will do nothing if the environment has not already been set.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_environment_set) {
        LOG_INFO("Environment not set... not initializing error monitor");
        return;
    }

    if (error_monitor_initialized) {
        LOG_WARNING("Error monitor already initialized!");
        return;
    }

    sentry_options_t *options = sentry_options_new();

    // By default, sentry will use the SENTRY_DSN environment variable. We could use this instead
    // of baking it into the protocol like this, but should only do so after setting up sentry for
    // the callers (client app and mandelbox-images).
    sentry_options_set_dsn(options, SENTRY_DSN);

    // Set the release name: of the form fractal-protocol@[git hash].
    char release[200];
    snprintf(release, sizeof(release), "fractal-protocol@%s", fractal_git_revision());
    sentry_options_set_release(options, release);

    // Set the environment that was set by error_monitor_set_environment();
    sentry_options_set_environment(options, error_monitor_environment);

    // Set this to true to enable verbose Sentry debugging.
    sentry_options_set_debug(options, false);

    // For GDPR, etc., we will need to eventually prompt the user to give/revoke consent.
    // If user is set and consent is required but not given, sentry will drop all events.
    sentry_options_set_require_user_consent(options, false);

    // Sentry doesn't document it, but this will free options.
    sentry_init(options);

    // Tag all logs with an identifier for whether this is from the client or the server.
    if (is_client) {
        sentry_set_tag("protocol-side", "client");
    } else {
        sentry_set_tag("protocol-side", "server");
    }

    error_monitor_initialized = true;

    // Tag all logs with a connection id of "waiting", to be updated once an actual id arrives.
    error_monitor_set_connection_id(-1);

    // Tag all logs with a user of "None", to be updated once an actual username arrives.
    error_monitor_set_username(NULL);

    LOG_INFO("Error monitor initialized!");
}

void error_monitor_shutdown() {
    /*
        Shuts down the error monitor.

        Arguments:
            None

        Returns:
            None

        Note: This should only be called after calling logging.h's
            `destroy_logger()`. See the "Usage" section above for details.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_initialized) return;

    sentry_shutdown();
    error_monitor_initialized = false;
}

void error_monitor_log_breadcrumb(const char *tag, const char *message) {
    /*
        Logs a breadcrumb to the error monitor.

        Arguments:
            tag (char*): The level of the breadcrumb; for example, "WARNING" or
                "INFO".
            message (char*): The message associated with the breadcrumb.

        Returns:
            None

        Note: This is automatically called from the `LOG_*` functions from
            logging.h, and should almost never be called alone. Because of
            this, `LOG_*` functions should NEVER be called from here, or we
            risk entering an infinite recursion loop.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_initialized) return;

        // In the current sentry-native beta version, breadcrumbs can only be logged
        // from macOS and Linux.
#ifndef _WIN32
    sentry_value_t crumb = sentry_value_new_breadcrumb("default", message);
    sentry_value_set_by_key(crumb, "category", sentry_value_new_string("protocol-logs"));
    sentry_value_set_by_key(crumb, "level", sentry_value_new_string(tag));

    // Sentry doesn't document it, but this will free crumb.
    sentry_add_breadcrumb(crumb);
#endif
}

void error_monitor_log_error(const char *message) {
    /*
        Logs an error event to the error monitor.

        Arguments:
            message (char*): The message associated with the error event.

        Returns:
            None

        Note: This is automatically called from the `LOG_*` functions from
            logging.h, and should almost never be called alone. Because of
            this, `LOG_*` functions should NEVER be called from here, or we
            risk entering an infinite recursion loop.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_initialized) return;

    sentry_value_t event =
        sentry_value_new_message_event(SENTRY_LEVEL_ERROR, "protocol-errors", message);
    // Sentry doesn't document it, but this will free user.
    sentry_capture_event(event);
}
