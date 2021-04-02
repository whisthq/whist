import { app, ipcMain } from "electron"
import EventEmitter from "events"
import { fromEvent, Observable } from "rxjs"
import { map, share, first, pluck } from "rxjs/operators"
import { store } from "@app/utils/persist"
import { StateChannel } from "@app/utils/constants"

fromEvent(app as EventEmitter, "window-all-closed").subscribe(() => {
    // On macOS it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform !== "darwin") app.quit()
})

export const ipcState = fromEvent(ipcMain, StateChannel).pipe(
    map(([_event, state]) => state),
    share()
)

export const fromIPC = (...keys: string[]) =>
    ipcState.pipe(pluck(...keys), share())

export const fromPersisted = (key: string) =>
    new Observable((subscriber) => {
        if (store.get(key) !== undefined) subscriber.next(store.get(key))
        store.onDidChange(key, (newKey, _oldKey) => {
            subscriber.next(newKey)
        })
    }).pipe(share())

export const appReady = fromEvent(app as EventEmitter, "ready").pipe(first())

export const appActivations = fromEvent(app as EventEmitter, "activate")

export const appQuits = fromEvent(app as EventEmitter, "window-all-closed")
