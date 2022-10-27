/* Copyright (c) 2022 Whist Technologies, Inc. All rights reserved. */

extern "C" {
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
}

#include "whist_api_native.h"

#include <algorithm>

// These are default values used in MacOS.
#define DEFAULT_KEY_REPEAT_INIT_DELAY 68
#define DEFAULT_KEY_REPEAT_RATE 6

int native_get_keyboard_repeat_rate() {
    NSTimeInterval repeat_rate_ms = [NSEvent keyRepeatInterval];
    return std::clamp((int)(1.0 / repeat_rate_ms + 0.5), 0, 2000);
}

int native_get_keyboard_repeat_initial_delay() {
    NSTimeInterval repeat_initial_delay = [NSEvent keyRepeatDelay];
    return std::clamp((int)(1000.0 * repeat_initial_delay + 0.5), 0, 10000);
}

void native_get_system_language(char* locale_str_buffer, int buffer_size) {
    CFLocaleRef cflocale = CFLocaleCopyCurrent();
    CFStringRef country_code = (CFStringRef)CFLocaleGetValue(cflocale, kCFLocaleCountryCode);
    CFStringRef language_code = (CFStringRef)CFLocaleGetValue(cflocale, kCFLocaleLanguageCode);
    CFRelease(cflocale);

    // format as COUNTRYCODE_languagecode
    NSString* locale_str = [NSString stringWithFormat:@"%@_%@", language_code, country_code];
    [locale_str getCString:locale_str_buffer maxLength:buffer_size encoding:NSUTF8StringEncoding];
}
