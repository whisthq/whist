import Store from "electron-store"

export const store = new Store({ watch: true })

export const persist = (obj: any) => {
    for (let key in obj) {
        let value = obj[key]
        if (value && value !== store.get(key)) store.set(key as string, value)
    }
}

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

export const persistLoad = () => ({
    ...store.store,
    email: store.store["email"] as string,
    accessToken: store.store["accessToken"] as string,
    configToken: store.store["configToken"] as string,
})
