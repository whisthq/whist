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

#define WHIST_KB_DEFAULT_LAYOUT "xkb:us::eng"

#ifdef __APPLE__

#define MAX_APPLE_KB_NAME_LENGTH 75
// If this array gets any longer than 1000 or so,
// we should probably swap to a C++ std::map<string, string>

struct AppleKeyboardMapping {
    char apple_name[MAX_APPLE_KB_NAME_LENGTH];
    WhistKeyboardLayout mapped_layout;
};

// To add a new keyboard input method:
// 1) Launch a mandelbox and run `ibus-daemon &` (to restart ibus), followed by `ibus list-engine`
// 2) From the list of foreign characters, find the layout you want to use
// 3) Use https://wiki.archlinux.org/title/Input_method or something similar to locate a package
// 4) Use sudo apt-get install METHOD in the Dockerfile to install the package
// 5) Add the ibus engine to this mapping.
//
// If additional configuration is needed, e.g. switching input methods within the same keyboard, use
// the third command
static struct AppleKeyboardMapping apple_keyboard_mappings[] = {
    {"com.apple.keylayout.USExtended", {"xkb:us::eng", ""}},
    {"com.apple.keylayout.US", {"xkb:us::eng", ""}},
    {"com.apple.keylayout.USInternational-PC", {"xkb:us::eng", ""}},
    {"com.apple.keylayout.Italian-Pro", {"xkb:it::ita", ""}},
    {"com.apple.keylayout.Italian", {"xkb:it::ita", ""}},
    {"com.apple.keylayout.Arabic", {"xkb:ara::ara", ""}},
    {"com.apple.keylayout.ABC-QWERTZ", {"xkb:de:nodeadkeys:ger", ""}},
    {"com.apple.keylayout.German", {"xkb:de::ger", ""}},
    {"com.apple.keylayout.Austrian", {"xkb:de::ger", ""}},  // Same as German on Linux
    {"com.apple.keylayout.Canadian-CSA", {"xkb:ca:eng:eng", ""}},
    {"com.apple.keylayout.ABC-AZERTY", {"xkb:fr::fra", ""}},
    {"com.apple.keylayout.French", {"xkb:fr::fra", ""}},
    {"com.apple.keylayout.SwissFrench", {"xkb:ch:fr:fra", ""}},
    {"com.apple.keylayout.LatinAmerican", {"xkb:latam::spa", ""}},
    {"com.apple.keylayout.Spanish", {"xkb:es::spa", ""}},
    {"com.apple.keylayout.Hebrew", {"xkb:il::heb", ""}},
    {"com.apple.keylayout.Canadian", {"xkb:ca:eng:eng", ""}},
    {"com.apple.keylayout.DVORAK-QWERTYCMD", {"xkb:us:dvorak:eng", ""}},
    {"com.apple.keylayout.ABC-India", {"xkb:us:intl:eng", ""}},
    {"com.apple.keylayout.Dvorak", {"xkb:us:dvorak:eng", ""}},
    {"com.apple.keylayout.British", {"xkb:gb:extd:eng", ""}},
    {"com.apple.keylayout.Irish",
     {"xkb:gb:extd:eng", ""}},  // According to: https://forums.linuxmint.com/viewtopic.php?t=297738
    {"com.apple.keylayout.Russian", {"xkb:ru::rus", ""}},
    {"com.apple.keylayout.ABC", {"xkb:us:intl:eng", ""}},
    {"com.apple.inputmethod.SCIM.ITABC", {"pinyin", ""}},
    {"com.sogou.inputmethod.sogou.pinyin", {"pinyin", ""}},
    {"com.apple.inputmethod.Kotoeri.RomajiTyping.Japanese",
     {"anthy", "gsettings set org.freedesktop.ibus.engine.anthy.common input-mode 0"}},
    {"com.apple.inputmethod.Kotoeri.RomajiTyping.Japanese.Katakana",
     {"anthy", "gsettings set org.freedesktop.ibus.engine.anthy.common input-mode 1"}},
    {"com.apple.inputmethod.Kotoeri.RomajiTyping.Roman",
     {"anthy", "gsettings set org.freedesktop.ibus.engine.anthy.common input-mode 3"}},
    {"com.apple.inputmethod.Korean.2SetKorean", {"hangul", ""}},
};

#define NUM_APPLE_KEYBOARD_MAPPINGS \
    ((int)sizeof(apple_keyboard_mappings) / (int)sizeof(apple_keyboard_mappings[0]))
#endif

#ifdef __linux__

static const char linux_supported_layouts[][5] = {"us", "it",    "ara", "de", "fr",
                                                  "es", "latam", "il",  "ca", "uk"};

void run_as_whist_user(const char *command);

#define NUM_LINUX_SUPPORTED_LAYOUTS \
    ((int)sizeof(linux_supported_layouts) / (int)sizeof(linux_supported_layouts[0]))
#endif

/*
============================
Public Function Implementations
============================
*/

WhistKeyboardLayout get_keyboard_layout(void) {
    static WhistKeyboardLayout whist_layout = {0};

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
        // Use the default layout if we can't get the layout
        safe_strncpy(whist_layout.layout_name, WHIST_KB_DEFAULT_LAYOUT,
                     sizeof(whist_layout.layout_name));
        safe_strncpy(whist_layout.additional_command, "", sizeof(whist_layout.additional_command));
        return whist_layout;
    }
    // if the layout hasn't changed, just return our old whist_layout
    if (strncmp(layout, old_layout, sizeof(layout)) == 0) {
        return whist_layout;
    }

    // otherwise, look for the keyboard layout and set whist_layout
    bool found = false;
    for (int i = 0; i < NUM_APPLE_KEYBOARD_MAPPINGS; i++) {
        if (strncmp(apple_keyboard_mappings[i].apple_name, layout, sizeof(layout)) == 0) {
            // memcpy the discovered layout to whist_layout
            memcpy(&whist_layout, &apple_keyboard_mappings[i].mapped_layout, sizeof(whist_layout));
            LOG_INFO("Recognized keyboard layout %s", whist_layout.layout_name);
            found = true;
            break;
        }
    }

    // if we couldn't find the new layout, log an error
    if (!found) {
        // Log the unrecognized keyboard layout,
        // so we can add support for it if we see usage of it
        safe_strncpy(whist_layout.layout_name, WHIST_KB_DEFAULT_LAYOUT,
                     sizeof(whist_layout.layout_name));
        safe_strncpy(whist_layout.additional_command, "", sizeof(whist_layout.additional_command));
        LOG_ERROR("Mac Keyboard Layout %s was not recognized! Defaulting to %s", layout,
                  WHIST_KB_DEFAULT_LAYOUT);
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
    static WhistKeyboardLayout current_layout = {WHIST_KB_DEFAULT_LAYOUT, ""};

    if (requested_layout.layout_name[WHIST_KB_LAYOUT_NAME_MAX_LENGTH - 1] != '\0') {
        LOG_ERROR("Could not set layout name! last character was not NULL!");
        return;
    }

    // Don't set the keyboard if nothing changed
    if (memcmp(&current_layout, &requested_layout, sizeof(WhistKeyboardLayout)) == 0) {
        return;
    }

    // Otherwise, copy into current_layout and handle the new current_layout
    memcpy(&current_layout, &requested_layout, sizeof(WhistKeyboardLayout));
    LOG_INFO("Current layout: %s; Additional Command: %s", current_layout.layout_name,
             current_layout.additional_command);

#ifdef __linux__
    char cmd_buf[1024];
    int bytes_written;
    // first, run the additional command
    if (strlen(current_layout.additional_command) != 0) {
        run_as_whist_user(current_layout.additional_command);
        run_as_whist_user("ibus restart");
    }
    // then, change the engine
    bytes_written =
        snprintf(cmd_buf, sizeof(cmd_buf), "/usr/share/whist/run-as-whist-user.sh 'ibus engine %s'",
                 current_layout.layout_name);

    if (bytes_written >= 0) {
        LOG_DEBUG("Running %s", cmd_buf);
        runcmd(cmd_buf, NULL);
    }
#else
    LOG_FATAL("Unimplemented on Mac/Windows!");
#endif
}

#ifndef __APPLE__

int display_notification(WhistNotification notif) {
    LOG_WARNING("Notification display not implemented on this OS");
    return -1;
}

#endif

void package_notification(WhistNotification *notif, const char *title, const char *message) {
    // The logic is currently very simple, but this function is factored out in case
    // we would like to add more complicated parsing rules in the future.
    safe_strncpy(notif->title, title, MAX_NOTIF_TITLE_LEN);
    safe_strncpy(notif->message, message, MAX_NOTIF_MSG_LEN);
}

#ifdef __linux__
void run_as_whist_user(const char *command) {
    // Run a shell command as the whist user
    static char cmd_buf[1024];
    int bytes_written =
        snprintf(cmd_buf, sizeof(cmd_buf), "/usr/share/whist/run-as-whist-user.sh '%s'", command);
    if (bytes_written >= 0) {
        LOG_DEBUG("Running command %s", cmd_buf);
        runcmd(cmd_buf, NULL);
    }
}
#endif
