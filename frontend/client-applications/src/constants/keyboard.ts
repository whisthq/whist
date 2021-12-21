// Initial key repeat values
export const INITIAL_KEY_REPEAT_MIN_MAC = 15
export const INITIAL_KEY_REPEAT_MIN_LINUX = 115
// Initial key repeat ranges
export const INITIAL_KEY_REPEAT_RANGE_MAC = 120 - INITIAL_KEY_REPEAT_MIN_MAC
export const INITIAL_KEY_REPEAT_RANGE_LINUX =
  2000 - INITIAL_KEY_REPEAT_MIN_LINUX
// Key repeat values
export const KEY_REPEAT_MIN_MAC = 2.0
export const KEY_REPEAT_MIN_LINUX = 9.0
// Key repeat ranges
export const KEY_REPEAT_RANGE_MAC = 120 - KEY_REPEAT_MIN_MAC
export const KEY_REPEAT_RANGE_LINUX = 1000 - KEY_REPEAT_MIN_LINUX
// There's no standard language across operating systems for keyboard language layouts,
// so this map translates the most common Mac keyboard layouts to X11 keyboard layouts
export const macToLinuxKeyboardMap = {
  "com.apple.keylayout.US": "us",
  "com.apple.keylayout.Italian-Pro": "it",
  "com.apple.keylayout.Italian": "it",
  "com.apple.keylayout.Arabic": "ara",
  "com.apple.keylayout.ABC-QWERTZ": "de",
  "com.apple.keylayout.German": "de",
  "com.apple.keylayout.Canadian-CSA": "ca",
  "com.apple.keylayout.ABC-AZERTY": "fr",
  "com.apple.keylayout.French": "fr",
  "com.apple.keylayout.SwissFrench": "fr",
  "com.apple.keylayout.LatinAmerican": "latam",
  "com.apple.keylayout.Spanish": "es",
} as { [key: string]: string }
