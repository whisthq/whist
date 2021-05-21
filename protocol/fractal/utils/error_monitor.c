// Include Sentry
#include <sentry.h>
#include <fractal/core/fractal.h>
#include "error_monitor.h"

#define SENTRY_DSN "https://8e1447e8ea2f4ac6ac64e0150fa3e5d0@o400459.ingest.sentry.io/5373388"

static char error_monitor_environment[FRACTAL_ARGS_MAXLEN + 1];
static char error_monitor_environment_set = false;
static bool error_monitor_initialized = false;

void error_monitor_set_environment(char *environment) {
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
    // If we haven't set the environment, we don't want our error manager.
    if (!error_monitor_initialized) return;

    sentry_value_t user = sentry_value_new_object();

    // Set the user to username, or None as a default.
    if (username) {
        sentry_value_set_by_key(user, "email", sentry_value_new_string(username));
    } else {
        sentry_value_set_by_key(user, "email", sentry_value_new_string("None"));
    }

    // Sentry doesn't document it, but this will free user.
    sentry_set_user(user);
}

void error_monitor_set_connection_id(int id) {
    // If we haven't set the environment, we don't want our error manager.
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
    // If we haven't set the environment, we don't want our error manager.
    if (!error_monitor_environment_set) return;

    if (error_monitor_initialized) {
        LOG_WARNING("Error monitor already initialized!");
        return;
    }

    sentry_options_t *options = sentry_options_new();

    // By default, sentry will use the SENTRY_DSN environment variable. We could use this instead
    // of baking it into the protocol like this, but should only do so after setting up sentry for
    // the callers (client app and container script).
    sentry_options_set_dsn(options, SENTRY_DSN);

    // Set the release name: of the form fractal-protocol@[git hash].
    char release[200];
    snprintf(release, sizeof(release), "fractal-protocol@%s", fractal_git_revision());
    sentry_options_set_release(options, release);

    // Set the environment that was set by error_monitor_set_environment();
    sentry_options_set_environment(options, error_monitor_environment);

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
    // If we haven't set the environment, we don't want our error manager.
    if (!error_monitor_initialized) return;

    sentry_shutdown();
    error_monitor_initialized = false;
}

// No calling logging inside!
void error_monitor_log_breadcrumb(const char *tag, const char *message) {
    // If we haven't set the environment, we don't want our error manager.
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

// No calling logging inside!
void error_monitor_log_error(const char *message) {
    // If we haven't set the environment, we don't want our error manager.
    if (!error_monitor_initialized) return;

    sentry_value_t event =
        sentry_value_new_message_event(SENTRY_LEVEL_ERROR, "protocol-errors", message);

    // Sentry doesn't document it, but this will free user.
    sentry_capture_event(event);
}
