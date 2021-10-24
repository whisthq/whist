#ifndef WINDOWS_UTILS_H
#define WINDOWS_UTILS_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file windows_utils.h
 * @brief This file contains all code that interacts directly with Windows
 *        desktops (Winlogon, the login screen, and regular desktops).
============================
Usage
============================

DesktopContext: This type represents a Windows desktop object.
    - This object can be used to represent a Windows desktop to set to (for
      instance, Winsta0, the default Windows desktop)
    - It can also be used to tell whether the desktop is ready (for instance,
      Winlogon)
*/

/*
============================
Includes
============================
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fractal/core/fractal.h>
#include "../input/input.h"

/*
============================
Custom types
============================
*/

typedef struct DesktopContext {
    TCHAR desktop_name[1000];
    HDESK desktop_handle;
    bool ready;
} DesktopContext;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Attaches the current thread to the specified
 *                                 current input desktop
 *
 * @param current_input_desktop    The Windows desktop that the current thread
 *                                 gets set to
 *
 * @returns                        Will return -2 on failure, will return 0 on
 *                                 success
 */
int set_current_input_desktop(HDESK current_input_desktop);

/**
 * @brief                          Open current or specified Desktop and set its
 *                                 information in DesktopContext, and set the
 *                                 current thread to the opened Desktop
 *
 * @param desktop_name             Name of the Windows Desktop to set to (i.e.
 *                                 "Winlogon")
 * @param get_name                 If this is true, the name of the Desktop set
 *                                 to gets saved in DesktopContext
 * @param set_thread               If this is true, the current thread gets set
 *                                 to the opened Desktop
 *
 * @returns                        Returns a DesktopContext with information on
 *                                 the opened Desktop
 */
DesktopContext open_new_desktop(WCHAR* desktop_name, bool get_name, bool set_thread);

/**
 * @brief                          Open a Windows window station (a handle to a
 *                                 Desktop session), like Winsta0 or Winlogon
 */
void open_window();

/**
 * @brief                          Call the above functions; logs from Winlogon
 *                                 to Winsta0 (the standard Windows desktop) by
 *                                 entering the Whist password
 *
 * @param input_device             The input device to use for setting the password
 * @param vm_password              The password to the VM
 *
 * @returns                        Return false is succeeded, else true
 */
bool init_desktop(InputDevice* input_device, char* vm_password);

#endif  // WINDOWS_UTILS_H
