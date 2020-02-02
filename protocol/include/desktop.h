/*
 * This file contains the headers of simple functions to set the Windows
 * desktop of a specific thread or process using WinAPI.
 * 
 * Fractal Protocol version: 1.0
 * 
 * Last modified: 01/26/2020
 * 
 * By: Philippe Noël, Ming Ying
 *
 * Copyright Fractal Computers, Inc. 2019-2020
**/

#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include "fractal.h"

typedef struct DesktopContext{
  TCHAR desktop_name[1000];
  HDESK desktop_handle;
  bool ready;
} DesktopContext;

// @brief Attaches the current thread to the current input desktop.
// @details Uses SetThreadDesktop from WinAPI.
int setCurrentInputDesktop(HDESK currentInputDesktop);

// @brief Opens current or specified desktop, fills the DesktopContext, sets thread to desktop.
// @details Uses OpenInputDesktop and SetThreadDesktop from WinAPI.
DesktopContext OpenNewDesktop(char* desktop_name, bool get_name);

// @brief Opens a Windows station.
void OpenWindow();

// @brief Calls the above functions; opens a windows station and desktop.
void InitDesktop();


#endif // DESKTOP_H
