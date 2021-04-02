import { app, ipcMain } from "electron"
import EventEmitter from "events"
import { fromEvent, Observable } from "rxjs"
import { map, share, first, pluck } from "rxjs/operators"
import { store } from "@app/utils/persist"
import { StateChannel } from "@app/utils/constants"

export const eventIPC = fromEvent(ipcMain, StateChannel).pipe(
    map(([_event, state]) => state),
    share()
)

export const fromEventIPC = (...keys: string[]) =>
    eventIPC.pipe(pluck(...keys), share())

export const fromEventPersist = (key: string) =>
    new Observable((subscriber) => {
        if (store.get(key) !== undefined) subscriber.next(store.get(key))
        store.onDidChange(key, (newKey, _oldKey) => {
            subscriber.next(newKey)
        })
    }).pipe(share())

export const eventAppReady = fromEvent(app as EventEmitter, "ready").pipe(
    first()
)

export const eventAppActivation = fromEvent(app as EventEmitter, "activate")

export const eventAppQuit = fromEvent(app as EventEmitter, "window-all-closed")
