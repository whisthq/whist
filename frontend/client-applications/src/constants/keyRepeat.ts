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
// The max allowed key repeat rate on Linux is 255. However, values higher than 55 do not work well in practice. 
// This is a subjective value, and we could modify it up to 255.
export const KEY_REPEAT_RATE_MIN_LINUX = 1
export const KEY_REPEAT_RATE_MAX_LINUX = 55
export const KEY_REPEAT_RATE_MIN_MAC = 2
export const KEY_REPEAT_RATE_MAX_MAC = 120

// The range of values that the key repeat rate can take
export const KEY_REPEAT_RATE_RANGE_MAC =
  KEY_REPEAT_RATE_MAX_MAC - KEY_REPEAT_RATE_MIN_MAC
export const KEY_REPEAT_RATE_RANGE_LINUX =
  KEY_REPEAT_RATE_MAX_LINUX - KEY_REPEAT_RATE_MIN_LINUX
