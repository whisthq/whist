/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file persist.ts
 * @brief This file contains utility functions for interacting with electron-store, which is how
 * we persist data across sessions. This file manages the low-level details of how we'll persist
 * user data in between Electron sessions. From the perspective of the rest of the app, we should
 * just able to call these functions and not worry about any of the implementation details.
 * Persistence requires access to the file system, so it can only be used by the main process. Only
 * serializable data can be persisted. Essentially, anything that can be converted to JSON.
 */

import { app } from "electron"
import Store from "electron-store"
import { loggingBaseFilePath } from "@app/config/environment"
import event from "events"

app.setPath("userData", loggingBaseFilePath)

export const storeEmitter = new event.EventEmitter()

export const store = new Store({
  watch: true,
  // This is caused by some mistakes in a `conf` PR.
  // TODO: We can remove this after https://github.com/sindresorhus/conf/pull/159 is merged.
  configFileMode: 0o600,
})

const persistSet = (key: string, value: string | boolean) => {
  store.set(key, value)
}

const persistGet = (key: string) => store.get(key)

const persistClear = (keys?: string[]) => {
  if (keys === undefined) store.clear()

  keys?.forEach((key) => {
    store.delete(key)
  })
}

store.onDidAnyChange(() => {
  storeEmitter.emit("store-did-change")
})

export { persistSet, persistClear, persistGet }
