//
// Initial key repeat values
// These correspond to the amount of time (in milliseconds) that the key is held down before the key starts repeating.
//

// Obtained experimentally by @philippemnoel & @gabrieleoliaro
//
// Key repeat delay values on macOS are frequencies; 1 unit is a key repeat each 1/60th of a second
// We convert repeat delay frequencies to repeat delays (in ms) by dividing by 1/60 and multiplying by 1000
// Min delay (fastest) on macOS is 15 units and converts to 250ms on Linux
// Max delay (slowest) on macOS is 120 units and converts to 2000ms on Linux, even though on Linux the actual max delay is infinite.
export const INITIAL_KEY_REPEAT_MAC_TO_MS = 1000 / 60

// Increasing the converted value by 20% has been found to give better results in practice. This is a subjective value.
export const INITIAL_KEY_REPEAT_MAC_TO_LINUX_CONVERSION_FACTOR =
  INITIAL_KEY_REPEAT_MAC_TO_MS * 1.2

//
// Key repeat values
// These correspond to the amount of time (in milliseconds) between key repeats.
//

// Obtained experimentally by @philippemnoel & @gabrieleoliaro
//
// The max allowed key repeat rate on Linux is 255. However, values higher than 30 do not work well in practice.
// This is a subjective value, and we could modify it up to 255.
export const MIN_KEY_REPEAT_LINUX = 1
export const MAX_KEY_REPEAT_LINUX = 30
export const MIN_KEY_REPEAT_MAC = 2
export const MAX_KEY_REPEAT_MAC = 120
export const MIN_KEY_REPEAT_WINDOWS = 0
export const MAX_KEY_REPEAT_WINDOWS = 31

// The range of values that the key repeat rate can take
export const KEY_REPEAT_RANGE_MAC = MAX_KEY_REPEAT_MAC - MIN_KEY_REPEAT_MAC
export const KEY_REPEAT_RANGE_LINUX =
  MAX_KEY_REPEAT_LINUX - MIN_KEY_REPEAT_LINUX
export const KEY_REPEAT_RANGE_WINDOWS =
  MAX_KEY_REPEAT_WINDOWS - MIN_KEY_REPEAT_WINDOWS

export const INITIAL_REPEAT_COMMAND_MAC =
  "defaults read NSGlobalDomain InitialKeyRepeat"
export const REPEAT_COMMAND_MAC = "defaults read NSGlobalDomain KeyRepeat"

export const INITIAL_REPEAT_COMMAND_LINUX = "xset -q | grep 'auto repeat delay'"
export const REPEAT_COMMAND_LINUX = "xset -q | grep 'auto repeat delay'"
