// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#ifndef WHIST_BROWSER_HYBRID_BROWSER_EXTENSIONS_API_WHIST_API_NATIVE_H_
#define WHIST_BROWSER_HYBRID_BROWSER_EXTENSIONS_API_WHIST_API_NATIVE_H_

// The initial keyboard repeat delay, in ms
int native_get_keyboard_repeat_initial_delay(void);

// The keyboard repeat rate, in presses per second
int native_get_keyboard_repeat_rate(void);

// The system language, as a locale string
// buffer_size is in bytes, _including_ the null terminator, and must be >= 1
// A length-zero string will be returned if a locale could not be found
void native_get_system_language(char* locale_str_buffer, int buffer_size);

#endif  // WHIST_BROWSER_HYBRID_BROWSER_EXTENSIONS_API_WHIST_API_NATIVE_H_
