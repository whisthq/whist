#ifndef OSUTILS_H
#define OSUTILS_H
/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file os_utils.h
============================
Usage
============================
*/

/*
============================
Defines
============================
*/


/*
============================
Public Functions
============================
*/

/**
 * @brief                          Get keyboard layout
 */
void get_keyboard_layout(char* dst, int size);

/**
 * @brief                          Set keyboard layout
 */
void set_keyboard_layout(const char* requested_layout);


#endif
