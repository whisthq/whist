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

import Store from "electron-store";
import events from "events";

const store = new Store({ watch: true });
const persisted = new events.EventEmitter();

interface Cache {
  [k: string]: any;
}

const cacheName = "data";

const persistSet = (key: string, value: any) => {
  store.set(`${cacheName}.${key}`, value);
};

const persistGet = (key: keyof Cache) => (store.get(cacheName) as Cache)?.[key];

const persistClear = (keys: Array<keyof Cache>) => {
  keys.forEach((key) => {
    store.delete(`${cacheName}.${key as string}`);
  });
};

const emitCache = () => {
  persisted.emit("did-change", store.get("data"));

  store.onDidAnyChange(() => {
    persisted.emit("did-change", store.get("data"));
  });
};

export { store, persisted, persistSet, persistGet, persistClear, emitCache };
