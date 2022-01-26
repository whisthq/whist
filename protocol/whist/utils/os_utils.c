/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file os_utils.c
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "os_utils.h"
#include <whist/core/whist.h>
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif
#ifdef __linux__
#include <X11/XKBlib.h>
#endif

/*
============================
Defines
============================
*/

#define WHIST_KB_DEFAULT_LAYOUT "us"

#ifdef __APPLE__
// If this array gets any longer than 1000 or so,
// we should probably swap to a C++ std::map<string, string>
static const char* const apple_keyboard_mappings[][2] = {
    {"com.apple.keylayout.USExtended", "us"},  {"com.apple.keylayout.US", "us"},
    {"com.apple.keylayout.Italian-Pro", "it"}, {"com.apple.keylayout.Italian", "it"},
    {"com.apple.keylayout.Arabic", "ara"},     {"com.apple.keylayout.ABC-QWERTZ", "de"},
    {"com.apple.keylayout.German", "de"},      {"com.apple.keylayout.Canadian-CSA", "ca"},
    {"com.apple.keylayout.ABC-AZERTY", "fr"},  {"com.apple.keylayout.French", "fr"},
    {"com.apple.keylayout.SwissFrench", "fr"}, {"com.apple.keylayout.LatinAmerican", "latam"},
    {"com.apple.keylayout.Spanish", "es"},     {"com.apple.keylayout.Hebrew", "il"},
    {"com.apple.keylayout.Canadian", "ca"},    {"com.apple.keylayout.DVORAK-QWERTYCMD", "dvorak"},
    {"com.apple.keylayout.ABC-India", "us"},   {"com.apple.keylayout.Dvorak", "dvorak"},
    {"com.apple.keylayout.British", "uk"}};

#define NUM_APPLE_KEYBOARD_MAPPINGS \
    ((int)sizeof(apple_keyboard_mappings) / (int)sizeof(apple_keyboard_mappings[0]))
#endif

#ifdef __linux__

static const char linux_supported_layouts[][5] = {"us", "it",    "ara", "de", "fr",
                                                  "es", "latam", "il",  "ca", "uk"};

#define NUM_LINUX_SUPPORTED_LAYOUTS \
    ((int)sizeof(linux_supported_layouts) / (int)sizeof(linux_supported_layouts[0]))
#endif

WhistKeyboardLayout get_keyboard_layout(void) {
    static WhistKeyboardLayout whist_layout = {0};
    // Set the default layout, in-case the code below fails to set the keyboard layout
    safe_strncpy(whist_layout.layout_name, WHIST_KB_DEFAULT_LAYOUT,
                 sizeof(whist_layout.layout_name));

#ifdef __APPLE__
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    // get input source id - kTISPropertyInputSourceID
    // get layout name - kTISPropertyLocalizedName
    CFStringRef layout_id = TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
    static char old_layout[128] = {0};
    char layout[128] = {0};
    Boolean res = CFStringGetCString(layout_id, layout, sizeof(layout), kCFStringEncodingUTF8);
    if (!res) {
        LOG_ERROR("CFStringGetCString failed!");
        return whist_layout;
    }
    // if the layout hasn't changed, just return whist_layout
    if (strncmp(layout, old_layout, sizeof(layout)) == 0) {
        return whist_layout;
    }

    // otherwise, look for the keyboard layout and set whist_layout
    bool found = false;
    for (int i = 0; i < NUM_APPLE_KEYBOARD_MAPPINGS; i++) {
        if (strncmp(apple_keyboard_mappings[i][0], layout, sizeof(layout)) == 0) {
            // Copy new name
            safe_strncpy(whist_layout.layout_name, apple_keyboard_mappings[i][1],
                         sizeof(whist_layout.layout_name));
            LOG_INFO("Recognized keyboard layout %s", whist_layout.layout_name);
            found = true;
            break;
        }
    }

    // if we couldn't find the new layout, log an error
    if (!found) {
        // Log the unrecognized keyboard layout,
        // so we can add support for it if we see usage of it
        LOG_ERROR("Mac Keyboard Layout was not recognized! %s", layout);
    }

    // then copy layout into old_layout
    safe_strncpy(old_layout, layout, sizeof(layout));
#elif __linux__
    // Convenience function to check compatibility and intitialize the xkb lib
    Display* dpy = XkbOpenDisplay(NULL, NULL, NULL, NULL, NULL, NULL);

    if (dpy == NULL) {
        return whist_layout;
    }

    XkbDescRec* kbd_desc_ptr = XkbAllocKeyboard();

    kbd_desc_ptr->dpy = dpy;

    XkbGetNames(dpy, XkbSymbolsNameMask, kbd_desc_ptr);

    char* symbols = XGetAtomName(dpy, kbd_desc_ptr->names->symbols);

    XkbStateRec xkb_state;
    XkbGetState(dpy, XkbUseCoreKbd, &xkb_state);

    // the current layout group
    int current_group_num = (int)xkb_state.group;

    char delims[] = "+_:(";
    char* tok = strtok(symbols, delims);

    int valid_token_count = 0;
    int found = 0;

    // tok comes from a string like this pc+fi(dvorak)+fi:2+ru:3+inet(evdev)+group(menu_toggle)
    // So to simplify things I just check each token against a supported layout list.
    // And we select the current_group_numth matching layout.
    while (tok != NULL && !found) {
        for (int i = 0; i < NUM_LINUX_SUPPORTED_LAYOUTS; i++) {
            if (strcmp(linux_supported_layouts[i], tok) == 0) {
                if (valid_token_count == current_group_num) {
                    safe_strncpy(whist_layout.layout_name, tok, sizeof(whist_layout.layout_name));
                    found = 1;
                    break;
                }
                valid_token_count++;
            }
        }
        tok = strtok(NULL, delims);
    }

    XkbFreeNames(kbd_desc_ptr, XkbSymbolsNameMask, 1);
    XFree(symbols);
    XFree(kbd_desc_ptr);
    XCloseDisplay(dpy);
#elif _WIN32
    // TODO: Implement on Windows
#endif

    // Return the whist layout
    return whist_layout;
}

void set_keyboard_layout(WhistKeyboardLayout requested_layout) {
    static char current_layout_name[WHIST_KB_LAYOUT_NAME_MAX_LENGTH] = WHIST_KB_DEFAULT_LAYOUT;

    if (requested_layout.layout_name[WHIST_KB_LAYOUT_NAME_MAX_LENGTH - 1] != '\0') {
        LOG_ERROR("Could not set layout name! last character was not NULL!");
        return;
    }

    // Don't set the keyboard if nothing changed
    if (memcmp(current_layout_name, requested_layout.layout_name, sizeof(current_layout_name)) ==
        0) {
        return;
    }

    // Otherwise, copy into current_layout_name and handle the new current_layout_name
    memcpy(current_layout_name, requested_layout.layout_name, sizeof(current_layout_name));

#ifdef __linux__
    char cmd_buf[1024];
    int bytes_written =
        snprintf(cmd_buf, sizeof(cmd_buf), "setxkbmap -layout %s", current_layout_name);

    if (bytes_written >= 0) {
        runcmd(cmd_buf, NULL);
    }
#else
    LOG_FATAL("Unimplemented on Mac/Windows!");
#endif
}
