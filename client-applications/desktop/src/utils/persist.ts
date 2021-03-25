import Store from "electron-store"

export const store = new Store({ watch: true })

export const persistKeys = <T>(obj: T, ...keys: Array<keyof T & string>) => {
    for (let key of keys) {
        if (obj[key] && obj[key] !== store.get(key)) {
            store.set(key as string, obj[key])
            // store.delete(key)
        }
    }
}

export const persistClear = () => {
    store.clear()
}
