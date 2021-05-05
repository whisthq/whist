import Store from "electron-store"
import { get } from "lodash"

import { StateIPC } from "@app/@types/state"

export const store = new Store({ watch: true })

export const persist = (obj: Partial<StateIPC>) => {
  for (const key in obj) {
    const value = get(obj, key, "")
    if (value !== "" && value !== store.get(key)) store.set(key, value)
  }
}

export const persistKeys = (obj: Partial<StateIPC>, ...keys: string[]) => {
  for (const key of keys) {
    const value = get(obj, key, "")
    if (value !== "" && value !== store.get(key)) {
      store.set(key, value)
    }
  }
}

export const persistClear = () => {
  store.clear()
}

export const clearKeys = (...keys: string[]) => {
  store.reset(...keys)
}
