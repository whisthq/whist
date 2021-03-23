import Store from "electron-store"

export const store = new Store({ watch: true })

export const persistKeys = <T>(obj: T, ...keys: Array<keyof T>) => {
    for (let key of keys)
        if (obj[key] !== store.get(key as string))
            store.set(key as string, obj[key])
}

export const persistClear = () => {
    store.clear()
}
