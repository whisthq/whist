import Store from "electron-store"
import events from "events"
import { isEmpty, pickBy } from "lodash"

export const store = new Store({ watch: true })
export const persisted = new events.EventEmitter()

const cache = {
  accessToken: store.get("accessToken") ?? "",
  configToken: store.get("configToken") ?? "",
  email: store.get("email") ?? "",
}

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

export const persistClear = () => {
  store.clear()
}
