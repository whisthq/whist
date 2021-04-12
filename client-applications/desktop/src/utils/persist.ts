import Store from 'electron-store'

import { StateIPC } from "@app/utils/types"

export const store = new Store({ watch: true })

export const persist = (obj: Partial<StateIPC>) => {
  for (const key in obj) {
    const value = obj[key] ?? null
    if (value !== '' && value !== store.get(key)) store.set(key, value)
  }
}

export const persistKeys = (obj: Partial<StateIPC>, ...keys: string[]) => {
    for (const key of keys) {
        const value = obj[key] ?? null
        if (value !== "" && value !== store.get(key)) {
            store.set(key, value)
        }
    }
}

export const persistClear = () => {
  store.clear()
}
