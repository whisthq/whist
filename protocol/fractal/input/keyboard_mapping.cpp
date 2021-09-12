/**
 * Copyright Fractal Computers, Inc. 2020
 * @file input.c
 * @brief This file defines general input-processing functions and toggles
 *        between Windows and Linux servers.
============================
Usage
============================

// This will emit a mapped keyboard event, using the mapping of
// the specified OS
emit_mapped_key_event(input_device, FRACTAL_APPLE, key_code, pressed);

// This will synchronize the keyboard via the mapping associated
// with the specified OS
update_mapped_keyboard_state(input_device, FRACTAL_APPLE, keyboard_state);

*/

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <set>

using namespace std;
#define hmap unordered_map

extern "C" {
#include <fractal/core/fractal.h>
#include "keyboard_mapping.h"
#include "input_driver.h"
#include "input.h"
}

typedef struct {
} InternalKeyboardMapping;

// Set of modifiers, for detecting if a key is a modifier
set<FractalKeycode> modifiers = {
    FK_LCTRL, FK_LSHIFT, FK_LALT, FK_LGUI,
    FK_RCTRL, FK_RSHIFT, FK_RALT, FK_RGUI
};

// Will hash vectors so that we can make an hmap out of them
struct VectorHasher {
    int operator()(const vector<FractalKeycode> &v) const {
        int hash = (int)v.size();
        for(auto &i : v) {
            hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

// The modmap that occurs before the full keyboard mapping
hmap<FractalKeycode, FractalKeycode> modmap = {
    // Swap gui and ctrl
    {FK_LGUI, FK_LCTRL},
    {FK_LCTRL, FK_LGUI},
    // Map right-modifier to left-modifier, to make things easier
    {FK_RGUI, FK_LCTRL}, // (still swapped)
    {FK_RCTRL, FK_LGUI},
    {FK_RALT, FK_LALT},
    {FK_RSHIFT, FK_LSHIFT}
};

// The full keyboard mapping
hmap<vector<FractalKeycode>, vector<FractalKeycode>, VectorHasher> keyboard_mappings = {
    // { {FK_LCTRL, FK_COMMA}, {FK_} },
    // Access history with ctrl+y
    {
        {FK_LCTRL, FK_Y},
        {FK_LCTRL, FK_H},
    },
    // Make ctrl+h not access history
    {
        {FK_LCTRL, FK_H},
        {},
    },
    // Go forward/back on the browser
    {
        {FK_LCTRL, FK_LEFT},
        {FK_LALT, FK_LEFT},
    },
    {
        {FK_LCTRL, FK_RIGHT},
        {FK_LALT, FK_RIGHT},
    },
    // Move cursor forward/backward a word
    {
        {FK_LALT, FK_LEFT},
        {FK_LCTRL, FK_LEFT},
    },
    {
        {FK_LALT, FK_RIGHT},
        {FK_LCTRL, FK_RIGHT},
    },
    // Select forward/backward a word
    {
        {FK_LALT, FK_LSHIFT, FK_LEFT},
        {FK_LCTRL, FK_LSHIFT, FK_LEFT},
    },
    {
        {FK_LALT, FK_LSHIFT, FK_RIGHT},
        {FK_LCTRL, FK_LSHIFT, FK_RIGHT},
    },
    // Delete a whole word
    {
        {FK_LALT, FK_BACKSPACE},
        {FK_LCTRL, FK_BACKSPACE},
    },
    {
        {FK_LALT, FK_DELETE},
        {FK_LCTRL, FK_DELETE},
    },
    // Open a new tab with the currently selected text
    {
        {FK_LCTRL, FK_ENTER},
        {FK_LALT, FK_ENTER},
    },
    // Switch tabs with Ctrl+Tab/Ctrl+Shift+Tab
    // (we use GUI here because we swapped CTRL and GUI above)
    {
        {FK_LGUI, FK_TAB},
        {FK_LCTRL, FK_TAB},
    },
    {
        {FK_LGUI, FK_LSHIFT, FK_TAB},
        {FK_LCTRL, FK_LSHIFT, FK_TAB},
    },
};

auto key_sorter = [](const FractalKeycode& a, const FractalKeycode& b) -> bool {
    // In ascending order, with a massive weight on being a modifier
    return (int)a + KEYCODE_UPPERBOUND*modifiers.count(a) < (int)b + KEYCODE_UPPERBOUND*modifiers.count(b);
};

void init_keyboard_mapping() {
    // Sort the keyboard mappings in a way that can be compared to later,
    // Particularly, with modifiers at the end of the array
    hmap<vector<FractalKeycode>, vector<FractalKeycode>, VectorHasher> new_keyboard_map;
    for(const auto& kv : keyboard_mappings) {
        vector<FractalKeycode> key = kv.first;
        sort(key.begin(), key.end(), key_sorter);
        new_keyboard_map[key] = kv.second;
    }
    keyboard_mappings = new_keyboard_map;
}

bool initialized = false;

// Global state, when a keymap is being held
bool holding_keymap = false;
vector<FractalKeycode> currently_pressed;
vector<FractalKeycode> new_key_combination;

extern "C" int emit_mapped_key_event(InputDevice* input_device, FractalOSType os_type, FractalKeycode key_code, int pressed) {
    // Initialize keyboard mapping if it hasn't been already
    if (!initialized) {
        init_keyboard_mapping();
        initialized = true;
    }

    // Don't map stateful keys, and don't map from non-apple OS's
    if (key_code == FK_CAPSLOCK || key_code == FK_NUMLOCK || os_type != FRACTAL_APPLE) {
        emit_key_event(input_device, key_code, pressed);
        return 0;
    }

    // Filter through the modmap first, if necessary
    if (modmap.count(key_code)) {
        key_code = modmap[key_code];
    }

    // If we're currently holding a keymap,
    if (holding_keymap) {
        // and if there's a discrepancy between what was previously being pressed,
        // and the new input event,
        if (std::count(currently_pressed.begin(), currently_pressed.end(), key_code) != pressed) {
            // then we should disengage from the keymap as a result of this modifying input event

            // Release the keys in the new key combination, modifiers last
            reverse(new_key_combination.begin(), new_key_combination.end());
            for(FractalKeycode key : new_key_combination) {
                emit_key_event(input_device, key, 0);
            }

            // Press the original keys, modifiers first
            reverse(currently_pressed.begin(), currently_pressed.end());
            for(FractalKeycode key : currently_pressed) {
                if (pressed == 0 && key == key_code) {
                    // But we shouldn't press the key that we just decided to release
                    continue;
                }
                emit_key_event(input_device, key, 1);
            }

            // Mark keymap as not being held anymore
            holding_keymap = false;
        } else {
            // If the key is already in that state, we can just ignore the event
            return 0;
        }
    }

    // If a key is being released, just release it,
    // We currently don't support releasing into a keymap,
    // but we can if we want
    if (pressed == 0) {
        emit_key_event(input_device, key_code, 0);
        return 0;
    }

    // If a key is being pressed, we check against mapped key combinations

    // Get the current keyboard combination being pressed
    currently_pressed.clear(); // Start with an empty array
    for(int i = 0; i < KEYCODE_UPPERBOUND; i++) {
        FractalKeycode i_key = (FractalKeycode)i;
        if (i_key != key_code && get_keyboard_key_state(input_device, i_key) == 1) {
            currently_pressed.push_back(i_key);
        }
    }
    currently_pressed.push_back(key_code);

    // Put the modifiers at the end of the array
    sort(currently_pressed.begin(), currently_pressed.end(), key_sorter);

    // Check if this is a known keyboard mapping
    if (keyboard_mappings.count(currently_pressed)) {
        new_key_combination = keyboard_mappings[currently_pressed];

        // Release the original keys, modifiers last
        for(FractalKeycode key : currently_pressed) {
            emit_key_event(input_device, key, 0);
        }

        // Press the keys in the new key combination, modifiers first
        for(FractalKeycode key : new_key_combination) {
            emit_key_event(input_device, key, 1);
        }

        //----
        // Key Combination has been sent!
        //----

        // Mark the keymap as being held, because it is being held now
        holding_keymap = true;
    } else {
        // If it's not a key mapped combination, just press the key
        emit_key_event(input_device, key_code, 1);
    }

    // We'll say that it always succeeds
    return 0;
}

extern "C" void update_mapped_keyboard_state(InputDevice* input_device, FractalOSType os_type, FractalKeyboardState keyboard_state) {
    bool server_caps_lock = get_keyboard_modifier_state(input_device, FK_CAPSLOCK);
    bool server_num_lock = get_keyboard_modifier_state(input_device, FK_NUMLOCK);

    bool client_caps_lock_holding = keyboard_state.keyboard_state[FK_CAPSLOCK];
    bool client_num_lock_holding = keyboard_state.keyboard_state[FK_NUMLOCK];

    // Modmap all keycode values
    bool mapped_keys[KEYCODE_UPPERBOUND] = {0};
    FractalKeycode origin_mapping[KEYCODE_UPPERBOUND] = {FK_UNKNOWN};
    for (int fractal_keycode = 0; fractal_keycode < KEYCODE_UPPERBOUND;
         ++fractal_keycode) {
        FractalKeycode mapped_key = (FractalKeycode)fractal_keycode;
        if (os_type == FRACTAL_APPLE && modmap.count(mapped_key)) {
            mapped_key = modmap[mapped_key];
        }
        // If any origin key is pressed, then the modmap'ed key is considered pressed
        mapped_keys[mapped_key] |= fractal_keycode < keyboard_state.num_keycodes
            && (bool)keyboard_state.keyboard_state[fractal_keycode];
        if (mapped_keys[mapped_key]) {
            LOG_INFO("Syncing with %d pressed! From %d origin!", mapped_key, fractal_keycode);
        }
        // Remember which origin key that refers to
        origin_mapping[mapped_key] = (FractalKeycode)fractal_keycode;
    }

    for (int fractal_keycode = 0; fractal_keycode < KEYCODE_UPPERBOUND;
         ++fractal_keycode) {
        // If no origin key maps to this keycode, we can skip this entry
        if (origin_mapping[fractal_keycode] == FK_UNKNOWN) {
            continue;
        }
        if (ignore_key_state(input_device, (FractalKeycode)fractal_keycode, keyboard_state.active_pinch)) {
            continue;
        }
        int is_pressed = holding_keymap ?
            (int)std::count(currently_pressed.begin(), currently_pressed.end(), (FractalKeycode)fractal_keycode)
            : (int)get_keyboard_key_state(input_device, (FractalKeycode)fractal_keycode);
        if (!mapped_keys[fractal_keycode] &&
            is_pressed) {
            LOG_INFO("Discrepancy found at %d (%d), unpressing!", fractal_keycode, origin_mapping[fractal_keycode]);
            // Reverse map the key, since emit_mapped_key_event will remap it
            emit_mapped_key_event(input_device, os_type, origin_mapping[fractal_keycode], 0);
        } else if (mapped_keys[fractal_keycode] &&
                   !is_pressed) {
            LOG_INFO("Discrepancy found at %d (%d), pressing!", fractal_keycode, origin_mapping[fractal_keycode]);
            // Reverse map the key, since emit_mapped_key_event will remap it
            emit_mapped_key_event(input_device, os_type, origin_mapping[fractal_keycode], 1);

            if (fractal_keycode == FK_CAPSLOCK) {
                server_caps_lock = !server_caps_lock;
            }
            if (fractal_keycode == FK_NUMLOCK) {
                server_num_lock = !server_num_lock;
            }
        }
    }

    if (!!server_caps_lock != !!keyboard_state.caps_lock) {
        LOG_INFO("Caps lock out of sync, updating! From %s to %s\n",
                 server_caps_lock ? "caps" : "no caps",
                 keyboard_state.caps_lock ? "caps" : "no caps");
        if (client_caps_lock_holding) {
            // Release and repress
            emit_mapped_key_event(input_device, os_type, FK_CAPSLOCK, 0);
            emit_mapped_key_event(input_device, os_type, FK_CAPSLOCK, 1);
        } else {
            // Press and release
            emit_mapped_key_event(input_device, os_type, FK_CAPSLOCK, 1);
            emit_mapped_key_event(input_device, os_type, FK_CAPSLOCK, 0);
        }
    }

    if (!!server_num_lock != !!keyboard_state.num_lock) {
        LOG_INFO("Num lock out of sync, updating! From %s to %s\n",
                 server_num_lock ? "num lock" : "no num lock",
                 keyboard_state.num_lock ? "num lock" : "no num lock");
        if (client_num_lock_holding) {
            // Release and repress
            emit_mapped_key_event(input_device, os_type, FK_NUMLOCK, 0);
            emit_mapped_key_event(input_device, os_type, FK_NUMLOCK, 1);
        } else {
            // Press and release
            emit_mapped_key_event(input_device, os_type, FK_NUMLOCK, 1);
            emit_mapped_key_event(input_device, os_type, FK_NUMLOCK, 0);
        }
    }
}

