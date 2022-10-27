// Copyright 2022 Whist Technologies, Inc. All rights reserved.
extern "C" {
#include <windows.h>
#include <winreg.h>
}
#include <algorithm>
#include "whist_api_native.h"

#include <iostream>

// These are the default values to be used on Linux.
#define DEFAULT_KEY_REPEAT_INIT_DELAY 500
#define DEFAULT_KEY_REPEAT_RATE 30

int native_get_keyboard_repeat_rate() {
    return DEFAULT_KEY_REPEAT_RATE;
}
    
int native_get_keyboard_repeat_initial_delay() {
    return DEFAULT_KEY_REPEAT_INIT_DELAY;
}

void native_get_system_language(char* locale_str_buffer, int buffer_size) {
    locale_str_buffer[0] = '\0';
}
