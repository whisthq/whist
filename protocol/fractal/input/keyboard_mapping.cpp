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
    int operator()(const vector<FractalKeycode> &V) const {
        int hash = V.size();
        for(auto &i : V) {
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
    {FK_RALT, FK_LALT}
};

// The full keyboard mapping
hmap<vector<FractalKeycode>, vector<FractalKeycode>, VectorHasher> keyboard_mappings = {
    // { {FK_LCTRL, FK_COMMA}, {FK_} },
    {
        {FK_LCTRL, FK_Y},
        {FK_LCTRL, FK_H},
    },
    {
        {FK_LCTRL, FK_LEFT},
        {FK_LALT, FK_LEFT},
    },
    {
        {FK_LCTRL, FK_RIGHT},
        {FK_LALT, FK_RIGHT},
    },
    {
        {FK_LCTRL, FK_ENTER},
        {FK_LALT, FK_ENTER},
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
bool keymap_being_held = false;
vector<FractalKeycode> currently_pressed;
vector<FractalKeycode> new_key_combination;

extern "C" int emit_mapped_key_event(InputDevice* input_device, KeyboardMapping* keyboard_mapping, FractalKeycode key_code, int pressed) {
    // Initialize keyboard mapping if it hasn't been already
    if (!initialized) {
        init_keyboard_mapping();
        initialized = true;
    }

    // Filter through the modmap first, if necessary
    if (modmap.count(key_code)) {
        key_code = modmap[key_code];
    }

    // If we're currently holding a keymap,
    if (keymap_being_held) {
        // and If there's a discrepancy between what was being pressed,
        // and the new input event,
        if (std::count(currently_pressed.begin(), currently_pressed.end(), key_code) != pressed) {
            // then we should disengage from the keymap

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

            // Mark keymap as not being held
            keymap_being_held = false;
        }
    }

    // If a key is being released, just release it,
    // We currently don't support releasing into a keymap
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

        // Mark the keymap as being held, because it is being held
        keymap_being_held = true;
    } else {
        // If it's not a key mapped combination, just press the key
        emit_key_event(input_device, key_code, 1);
    }

    // We'll say that it always succeeds
    return 0;
}
