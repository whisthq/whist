// This file manages the low-level details of how we'll persist user data in
// between Electron sessions. From the perspective of the rest of the app,
// we should just able to call these functions and not worry about any of the
// implementation details.
//
// Persistence requires access to the file system, so it can only be used by
// the main process.
//
// Only serializable data can be persisted. Essentially, anything that can be
// converted to JSON.
import Store from "electron-store"
import { get } from "lodash"

import { StateIPC } from "@app/@types/state"

export const store = new Store({ watch: true })

// This is the function that should most often be used to persist data.
// Just pass it an object, and as long as its values are serializable to JSON
// it will save.
export const persist = (obj: Partial<StateIPC>) => {
  for (const key in obj) {
    const value = get(obj, key, "")
    if (value !== "" && value !== store.get(key)) store.set(key, value)
  }
}

// This is a helper function to use when you want to save only a subset of an
// object. It's useful when dealing with an object that may have null or
// undefined values. electron-store prefers that you delete keys instead of
// assigning them to undefined.
export const persistKeys = (obj: Partial<StateIPC>, ...keys: string[]) => {
  for (const key of keys) {
    const value = get(obj, key, "")
    if (value !== "" && value !== store.get(key)) {
      store.set(key, value)
    }
  }
}

// With no arguments, delete everything in persistence. We should extend this
// function to accept arguments so that we can delete specific keys.
export const persistClear = () => {
  store.clear()
}
