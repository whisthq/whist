#ifndef ERROR_MONITOR_H
#define ERROR_MONITOR_H
#include <fractal/core/fractal.h>

void error_monitor_set_environment(char *environment);

void error_monitor_set_username(char *username);

void error_monitor_set_connection_id(int id);

void error_monitor_initialize(bool is_client);

void error_monitor_shutdown();

// No calling logging inside!
void error_monitor_log_breadcrumb(const char *tag, const char *message);

// No calling logging inside!
void error_monitor_log_error(const char *message);
#endif  // ERROR_MONITOR_H
