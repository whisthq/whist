/**
 * Copyright Fractal Computers, Inc. 2021
 * @file persist.ts
 * @brief This file contains utility functions for interacting with electron-store, which is how
 * we persist data across sessions. This file manages the low-level details of how we'll persist
 * user data in between Electron sessions. From the perspective of the rest of the app, we should\
 * just able to call these functions and not worry about any of the implementation details.
 * 
 * Persistence requires access to the file system, so it can only be used by the main process. Only
 * serializable data can be persisted. Essentially, anything that can be converted to JSON.
 */

import Store from "electron-store"
import events from "events"
import { isEmpty, pickBy, keys } from "lodash"

export const store = new Store({ watch: true })
export const persisted = new events.EventEmitter()

interface Cache {
  [k: string]: string
}

const cache = {
  accessToken: store.get("accessToken") ?? "",
  configToken: store.get("configToken") ?? "",
  refreshToken: store.get("refreshToken") ?? "",
  email: store.get("email") ?? "",
  sub: store.get("sub") ?? "",
} as Cache

export const emitCache = () => {
  if (isEmpty(pickBy(cache, (x) => x === ""))) {
    persisted.emit("data-persisted", cache)
  } else {
    persisted.emit("data-not-persisted")
  }
}

export const persist = (key: string, value: string) => {
  store.set(key, value)
}

export const persistClear = (args?: { exclude?: string[] }) => {
  const excludedSet = new Set(args?.exclude)
  keys(cache).forEach((key) => {
    if (!excludedSet.has(key)) {
      // Delete key if not in excluded
      store.delete(key)
    }
  })
}

export const persistGet = (key: keyof Cache) => cache[key]
