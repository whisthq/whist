/*
 * Windows desktop sessions helper functions.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fractal.h"
#include "input.h"

typedef struct DesktopContext {
    TCHAR desktop_name[1000];
    HDESK desktop_handle;
    bool ready;
} DesktopContext;

// @brief Attaches the current thread to the current input desktop.
// @details Uses SetThreadDesktop from WinAPI.
int setCurrentInputDesktop(HDESK currentInputDesktop);

// @brief Opens current or specified desktop, fills the DesktopContext, sets thread to desktop.
// @details Uses OpenInputDesktop and SetThreadDesktop from WinAPI.
DesktopContext OpenNewDesktop(WCHAR* desktop_name, bool get_name, bool set_thread);

// @brief Opens a Windows station.
void OpenWindow();

// @brief Calls the above functions; opens a windows station and desktop.
void InitDesktop();

#endif // DESKTOP_H
