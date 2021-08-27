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

hmap<FractalKeycode, FractalKeycode> modmap = {
    // Swap gui and ctrl
    {FK_LGUI, FK_LCTRL},
    {FK_LCTRL, FK_LGUI},
    // Map right-modifier to left-modifier, to make things easier
    {FK_RGUI, FK_LCTRL}, // (still swapped)
    {FK_RCTRL, FK_LGUI},
    {FK_RALT, FK_LALT}
};

set<FractalKeycode> modifiers = {
    FK_LCTRL, FK_LSHIFT, FK_LALT, FK_LGUI,
    FK_RCTRL, FK_RSHIFT, FK_RALT, FK_RGUI
};

struct VectorHasher {
    int operator()(const vector<FractalKeycode> &V) const {
        int hash = V.size();
        for(auto &i : V) {
            hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

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

extern "C" int emit_mapped_key_event(InputDevice* input_device, KeyboardMapping* keyboard_mapping, FractalKeycode key_code, int pressed) {
    // Filter through the modmap first, if necessary 
    if (modmap.count(key_code)) {
        key_code = modmap[key_code];
    }

    // If a key is being released, just release it
    if (pressed == 0) {
        emit_key_event(input_device, key_code, 0);
        return 0;
    }

    // If a key is being pressed, we check against mapped key combinations

    // Get the current keyboard combination being pressed
    vector<FractalKeycode> currently_pressed;
    currently_pressed.reserve(KEYCODE_UPPERBOUND);
    for(int i = 0; i < KEYCODE_UPPERBOUND; i++) {
        FractalKeycode i_key = (FractalKeycode)i;
        if (i_key != key_code && get_keyboard_key_state(input_device, i_key) == 1) {
            currently_pressed.push_back(i_key);
        }
    }
    currently_pressed.push_back(key_code);

    // Put the modifiers at the end of the array
    sort(currently_pressed.begin(), currently_pressed.end(), 
    [](const FractalKeycode& a, const FractalKeycode& b) -> bool
    { 
        // In ascending order, with a massive weight on being a modifier
        return (int)a + KEYCODE_UPPERBOUND*modifiers.count(a) < (int)b + KEYCODE_UPPERBOUND*modifiers.count(b); 
    });

    // Check if this is a keyboard mapping
    if (keyboard_mappings.count(currently_pressed)) {
        vector<FractalKeycode> new_key_combination = keyboard_mappings[currently_pressed];

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

        // Release the keys in the new key combination, modifiers last
        reverse(new_key_combination.begin(), new_key_combination.end());
        for(FractalKeycode key : new_key_combination) {
            emit_key_event(input_device, key, 0);
        }

        // Press the original keys, modifiers first
        reverse(currently_pressed.begin(), currently_pressed.end());
        for(FractalKeycode key : currently_pressed) {
            emit_key_event(input_device, key, 1);
        }
    } else {
        // If it's not a key mapped combination, just press the key
        emit_key_event(input_device, key_code, 1);
    }

    // We'll say that it always succeeds
    return 0;
}
