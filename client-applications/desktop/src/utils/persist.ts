/**
 * Copyright Fractal Computers, Inc. 2021
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
import events from "events"
import { isEmpty, pickBy } from "lodash"

import { loggingBaseFilePath } from "@app/config/environment"

app.setPath("userData", loggingBaseFilePath)

export const store = new Store({ watch: true })
export const persisted = new events.EventEmitter()

interface Cache {
  [k: string]: string | boolean
}

type CacheName = "auth" | "data"

const persistedAuth = store.get("auth") as Cache

export const emitAuthCache = () => {
  const authCache = {
    accessToken: persistedAuth?.accessToken ?? "",
    configToken: persistedAuth?.configToken ?? "",
    refreshToken: persistedAuth?.refreshToken ?? "",
    userEmail: persistedAuth?.userEmail ?? "",
  } as Cache

  if (isEmpty(pickBy(authCache, (x) => x === ""))) {
    persisted.emit("data-persisted", authCache)
  } else {
    persisted.emit("data-not-persisted")
  }
}

export const persist = (
  key: string,
  value: string | boolean,
  cache?: CacheName
) => {
  store.set(`${cache ?? "auth"}.${key}`, value)
}

export const persistClear = (cache?: CacheName) => {
  store.delete(cache ?? "auth")
}

export const persistGet = (key: keyof Cache, cache?: CacheName) =>
  (store.get(cache ?? "auth") as Cache)?.[key]
