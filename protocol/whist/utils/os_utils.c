/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file os_utils.c
============================
Usage
============================
*/

#include "os_utils.h"

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#ifdef _WIN32
#elif __APPLE__
#else
#endif

//TODO This is returning from function stack. Need to either pass in a dst point 
//or use dynamic memory.
void get_keyboard_layout(char* dst, int size) {

  if(size <= WHIST_KB_LAYOUT_MAX_LENGTH) {
#ifdef _WIN32
    safe_strncpy(dst, WHIST_KB_DEFAULT_LAYOUT, size);
#elif __APPLE__
    safe_strncpy(dst, WHIST_KB_DEFAULT_LAYOUT, size);
#else
    safe_strncpy(dst, WHIST_KB_DEFAULT_LAYOUT, size);
#endif
  }
}

#define CMD_BUFFER_MAX_LENGHT 1024

void set_keyboard_layout(const char* requested_layout) {
    static char current_layout[WHIST_KB_LAYOUT_MAX_LENGTH] = WHIST_KB_DEFAULT_LAYOUT;

    if(requested_layout == NULL) {
        return;
    }

    if(strncmp(current_layout, requested_layout, strlen(current_layout) + 1) == 0) {
      return;
    }

    safe_strncpy(current_layout, requested_layout, WHIST_KB_LAYOUT_MAX_LENGTH);

    const char* cmd_format = "setxkbmap -layout %s";
    char cmd_buf[CMD_BUFFER_MAX_LENGHT];
    int bytes_written = snprintf(cmd_buf, CMD_BUFFER_MAX_LENGHT, cmd_format, current_layout);

    if (bytes_written < CMD_BUFFER_MAX_LENGHT) {
        runcmd(cmd_buf, NULL);
    }
}
