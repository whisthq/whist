#ifndef DISPLAY_NOTIFS_H
#define DISPLAY_NOTIFS_H

/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file display_notifs.h
 * @brief stuff
============================
Usage
============================

TODO(kevin) write

*/

/*
============================
Includes
============================
*/

#include <stdbool.h>

/*
============================
Public Functions
============================
*/

int init_notif_bundle();
bool notif_bundle_initialized();
int deliver_notification(char *title, char *msg);

#endif  // DISPLAY_NOTIFS_H
