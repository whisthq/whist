import Store from 'electron-store'

export const store = new Store({ watch: true })

export const persist = (obj: Record<string, string>) => {
  for (const key in obj) {
    const value = obj[key] ?? ''
    if (value !== '' && value !== store.get(key)) store.set(key, value)
  }
}

export const persistKeys = (obj: Record<string, string>, ...keys: string[]) => {
  for (const key of keys) {
    const value = obj[key] ?? ''
    if (value !== '' && value !== store.get(key)) { store.set(key, value) }
  }
}

export const persistClear = () => {
  store.clear()
}
