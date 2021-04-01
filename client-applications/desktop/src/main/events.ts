import { app, ipcMain } from "electron"
import EventEmitter from "events"
import { fromEvent } from "rxjs"
import { map, share } from "rxjs/operators"
import { StateChannel } from "@app/utils/constants"

fromEvent(app as EventEmitter, "window-all-closed").subscribe(() => {
    // On macOS it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform !== "darwin") app.quit()
})

export const appReady = fromEvent(app as EventEmitter, "ready")

export const ipcState = fromEvent(ipcMain, StateChannel).pipe(
    map(([_event, state]) => state),
    share()
)

export const appActivations = fromEvent(app as EventEmitter, "activate")
