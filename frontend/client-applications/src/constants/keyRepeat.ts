//
// Initial key repeat values
// These correspond to the amount of time (in milliseconds) that the key is held down before the key repeat starts.
//

// Key repeat values on Mac are frequencies; 1 unit is a key repeat each 1/60th of a second
// We convert repeat frequencies to repeat delays by dividing by 1/60
export const KEY_REPEAT_MAC_TO_SECONDS = 1 / 60
export const SECONDS_TO_MS = 1000
export const KEY_REPEAT_MAC_TO_MS = KEY_REPEAT_MAC_TO_SECONDS * SECONDS_TO_MS
