/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file error_monitor.c
 * @brief This file contains the functions to configure and report breadcrumbs
 *        and error events to tools such as Sentry.
============================
Usage
============================
The error monitor needs to have an environment configured before it can be
started up. This environment should be development/staging/production, and is
passed in as a command-line parameter. Once the value is known, we may call
`whist_error_monitor_set_environment()` to configure it.

If no environment is set, the error monitor will fail to initialize. This is
because we don't want it to complain in personal setups and manual connections
when developing, but it's important to note the side-effect that we will not
report to the error monitoring service if environment isn't passed in.

To initialize the error monitor, we call `error_monitor_initialize()`. After
doing so, we can configure the error logging metadata with
`whist_error_monitor_set_username()` and `whist_error_monitor_set_connection_id()`.

At this point, calling `whist_error_monitor_log_breadcrumb()` and
`whist_error_monitor_log_error()` will report a trace of non-error events and report
a detailed breakdown for error-level events to the error monitor service,
respectively. Importantly, *you should almost never be calling this function by
itself*. Instead, simply use the `LOG_*()` functions from logging.h, which will
automatically send error monitor breadcrumbs and error reports as needed.

Because of this integration with our logging setup, we run into race conditions
in `whist_error_monitor_shutdown()`, which may cause us to fail to report our last
few breadcrumbs and error events. In order to avoid this, we must call
`whist_error_monitor_shutdown()` after calling `destroy_logger()`, to allow for any
pending error monitor log calls to be handled. Eventually, we should set up
a more robust solution for synchronizing these calls.
*/

/*
============================
Includes
============================
*/

#include <sentry/sentry.h>
#include <whist/core/whist.h>
#include "whist/utils/command_line.h"
#include "error_monitor.h"

/*
============================
Global Variables
============================
*/

#define SENTRY_DSN "https://8e1447e8ea2f4ac6ac64e0150fa3e5d0@o400459.ingest.sentry.io/5373388"
#define MAX_EVENTS_IN_PERIOD 2
#define MAX_EVENTS_PERIOD_MS 1000.0

static char error_monitor_environment[WHIST_ARGS_MAXLEN + 1];
static bool error_monitor_environment_set = false;
static char error_monitor_session_id[WHIST_ARGS_MAXLEN + 1] = {0};
static bool error_monitor_initialized = false;
static WhistTimer last_error_event_timer;

/*
============================
Command-line options
============================
*/

static bool set_environment_option(const WhistCommandLineOption *opt, const char *value) {
    whist_error_monitor_set_environment(value);
    return true;
}

static bool set_session_id_option(const WhistCommandLineOption *opt, const char *value) {
    whist_error_monitor_set_session_id(value);
    return true;
}

COMMAND_LINE_CALLBACK_OPTION(set_environment_option, 'e', "environment",
                             WHIST_OPTION_REQUIRED_ARGUMENT,
                             "The environment the protocol is running in, "
                             "e.g production, staging, development.  Default: none.")

COMMAND_LINE_CALLBACK_OPTION(set_session_id_option, 'd', "session-id",
                             WHIST_OPTION_REQUIRED_ARGUMENT,
                             "Set the session ID for the protocol's error logging.")

/*
============================
Private Function Implementations
============================
*/

/**
 * @brief Check whether we have sent more than `MAX_EVENTS_IN_PERIOD` errors
 *        in `MAX_EVENTS_PERIOD_MS` milliseconds.
 *
 * @returns (bool): `false` if too many events in most recent period, else `true`
 *
 * @note Because of `errors_sent`, this function should only be called before sending
 *       an error event. Since this is automatically called from `LOG_*` functions in
 *       logging.h, `LOG_*` functions should NEVER be called from here, or we risk
 *       entering an infinite recursion loop.
 */
static bool check_backoff(void) {
    static int errors_sent = 0;

    if (get_timer(&last_error_event_timer) > MAX_EVENTS_PERIOD_MS / MS_IN_SECOND) {
        errors_sent = 0;
        start_timer(&last_error_event_timer);
    }

    if (errors_sent >= MAX_EVENTS_IN_PERIOD) {
        return false;
    }

    // Increment `errors_sent` if error can be sent (not past threshold).
    errors_sent++;

    return true;
}

/*
============================
Public Function Implementations
============================
*/

void whist_error_monitor_set_environment(const char *environment) {
    if (error_monitor_initialized) {
        LOG_WARNING("Attempting to set the environment to %s after starting the error monitor!",
                    environment);
        return;
    }

    if (strcmp(environment, "prod") == 0 || strcmp(environment, "staging") == 0 ||
        strcmp(environment, "dev") == 0) {
        safe_strncpy(error_monitor_environment, environment, sizeof(error_monitor_environment));
        error_monitor_environment_set = true;
    } else if (strcmp(environment, "localdev") == 0 || strcmp(environment, "localdevwithdb") == 0) {
        LOG_INFO("On local development -- will not initialize error monitor.");
    } else {
        LOG_WARNING("Invalid environment %s set! Ignoring it.", environment);
    }
}

bool whist_error_monitor_environment_set(void) { return error_monitor_environment_set; }

char *get_error_monitor_environment(void) {
    if (!error_monitor_environment_set) {
        return NULL;
    }
    char *error_monitor_environment_copy =
        (char *)calloc(strlen(error_monitor_environment) + 1, sizeof(char));
    safe_strncpy(error_monitor_environment_copy, error_monitor_environment,
                 strlen(error_monitor_environment) + 1);
    return error_monitor_environment_copy;
}

char *get_error_monitor_session_id(void) {
    char *error_monitor_session_id_copy = (char *)calloc(WHIST_ARGS_MAXLEN + 1, sizeof(char));
    safe_strncpy(error_monitor_session_id_copy, error_monitor_session_id, WHIST_ARGS_MAXLEN + 1);
    return error_monitor_session_id_copy;
}

void whist_error_monitor_set_session_id(const char *session_id) {
    if (error_monitor_initialized) {
        LOG_WARNING("Attempting to set the session id to %s after starting the error monitor!",
                    session_id);
        return;
    }

    safe_strncpy(error_monitor_session_id, session_id, sizeof(error_monitor_session_id));
}

void whist_error_monitor_set_username(const char *username) {
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

void whist_error_monitor_set_connection_id(int id) {
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

bool whist_error_monitor_is_initialized(void) { return error_monitor_initialized; }

void whist_error_monitor_initialize(bool is_client) {
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

    // By default, Sentry will use the SENTRY_DSN environment variable. We could use this instead
    // of baking it into the protocol like this, but should only do so after setting up Sentry for
    // the callers (client app and mandelboxes).
    sentry_options_set_dsn(options, SENTRY_DSN);

    // Set the release name: of the form protocol@[git hash].
    char release[200];
    snprintf(release, sizeof(release), "protocol@%s", whist_git_revision());
    sentry_options_set_release(options, release);

    // Set the environment that was set by whist_error_monitor_set_environment();
    sentry_options_set_environment(options, error_monitor_environment);

    // Set this to true to enable verbose Sentry debugging.
    sentry_options_set_debug(options, false);

    // For GDPR, etc., we will need to eventually prompt the user to give/revoke consent.
    // If user is set and consent is required but not given, Sentry will drop all events.
    sentry_options_set_require_user_consent(options, false);

    // Sentry doesn't document it, but this will free options.
    sentry_init(options);

    // Tag all logs with an identifier for whether this is from the client or the server.
    if (is_client) {
        sentry_set_tag("protocol-side", "client");
    } else {
        sentry_set_tag("protocol-side", "server");
    }

    // Set the session id that was set by whist_error_monitor_set_session_id()
    if (error_monitor_session_id[0] != 0) {
        sentry_set_tag("session-id", error_monitor_session_id);
    } else {
        sentry_set_tag("session-id", "unknown");
    }

    // Start error event backoff timer
    start_timer(&last_error_event_timer);

    error_monitor_initialized = true;

    // Tag all logs with a connection id of "waiting", to be updated once an actual id arrives.
    whist_error_monitor_set_connection_id(-1);

    // Tag all logs with a user of "None", to be updated once an actual username arrives.
    whist_error_monitor_set_username(NULL);

    LOG_INFO("Error monitor initialized!");
}

void whist_error_monitor_shutdown(void) {
    /*
        Note: This should only be called after calling logging.h's
            `destroy_logger()`. See the "Usage" section above for details.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_initialized) return;

    sentry_shutdown();
    error_monitor_initialized = false;
}

void whist_error_monitor_log_breadcrumb(const char *tag, const char *message) {
    /*
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

    sentry_add_breadcrumb(crumb);
#endif
}

void whist_error_monitor_log_error(const char *message) {
    /*
        Note: This is automatically called from the `LOG_*` functions from
            logging.h, and should almost never be called alone. Because of
            this, `LOG_*` functions should NEVER be called from here, or we
            risk entering an infinite recursion loop.
    */

    // If we haven't set the environment, we don't want our error monitor.
    if (!error_monitor_initialized) return;

    // If too many errors have been sent in a period, then don't send this error.
    if (!check_backoff()) return;
    sentry_value_t event =
        sentry_value_new_message_event(SENTRY_LEVEL_ERROR, "protocol-errors", message);
    // Sentry doesn't document it, but this will free error.
    sentry_capture_event(event);
}
