#ifndef ERROR_MONITOR_H
#define ERROR_MONITOR_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file error_monitor.h
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

#include <fractal/core/fractal.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Configures the error monitor to use the
 *                                 provided environment.
 *
 * @param environment              The environment to set: should be one of
 *                                 development/staging/production.
 *
 * @note                           This should only be called before calling
 *                                 `error_monitor_initialize()`.
 */
void error_monitor_set_environment(char *environment);

/**
 * @brief                          Configures the username tag for error
 *                                 monitor reports.
 *
 * @param username                 The username to set. Setting NULL or "None"
 *                                 will unset the username instead.
 *
 * @note                           This should only be called after calling
 *                                 `error_monitor_initialize()`.
 */
void error_monitor_set_username(char *username);

/**
 * @brief                          Configures the connection id tag for error
 *                                 monitor reports.
 *
 * @param id                       The connection id to set. Setting -1 will
 *                                 set the connection id to "waiting".
 *
 * @note                           This should only be called after calling
 *                                 `error_monitor_initialize()`.
 */
void error_monitor_set_connection_id(int id);

/**
 * @brief                          Initializes the error monitor.
 *
 * @param is_client                True if this is the client protocol, else
 *                                 false.
 *
 * @note                           This will do nothing if the environment
 *                                 has not already been set.
 */
void error_monitor_initialize(bool is_client);

/**
 * @brief                          Shuts down the error monitor.
 *
 * @note                           This should only be called after calling
 *                                 logging.h's `destroy_logger()`. See the
 *                                 "Usage" section above for details.
 */
void error_monitor_shutdown();

/**
 * @brief                          Logs a breadcrumb to the error monitor.
 *
 * @param tag                      The level of the breadcrumb; for example,
 *                                 "WARNING" or "INFO".
 *
 * @param message                  The message associated with the breadcrumb.
 *
 * @note                           This is automatically called from the
 *                                 `LOG_*` functions from logging.h, and should
 *                                 almost never be called alone. Because of
 *                                 this, `LOG_*` functions should NEVER be
 *                                 called from here, or we risk entering an
 *                                 infinite recursion loop.
 */
void error_monitor_log_breadcrumb(const char *tag, const char *message);

/**
 * @brief                          Logs an error event to the error monitor.
 *
 * @param message                  The message associated with the error event.
 *
 * @note                           This is automatically called from the
 *                                 `LOG_*` functions from logging.h, and should
 *                                 almost never be called alone. Because of
 *                                 this, `LOG_*` functions should NEVER be
 *                                 called from here, or we risk entering an
 *                                 infinite recursion loop.
 */
void error_monitor_log_error(const char *message);

#endif  // ERROR_MONITOR_H
