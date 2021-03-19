import { Event, State, StateChannel } from "@app/utils/state"
import { ipcMain } from "electron"
import { app, BrowserWindow } from "electron"
import { store } from "@app/utils/persist"

export const listenState: Event = (setState) => {
    ipcMain.on(StateChannel, (_, state: Partial<State>) => setState(state))
}

export const listenAccess: Event = (setState) => {
    store.delete("accessToken")
    if (store.get("accessToken"))
        setState({ accessToken: store.get("accessToken", "") as string })

    store.onDidChange("accessToken", (newToken, _oldToken) => {
        setState({ accessToken: newToken as string })
    })
}

export const listenAppActivate: Event = (setState) => {
    app.on("activate", () => {
        // On macOS it's common to re-create a window in the app when the
        // dock icon is clicked and there are no other windows open.
        if (BrowserWindow.getAllWindows().length === 0) {
            setState({ appWindowRequested: true })
        }
    })
}

export const listenAppQuit: Event = (_setState) => {
    // Quit when all windows are closed.
    app.on("window-all-closed", () => {
        // On macOS it is common for applications and their menu bar
        // to stay active until the user quits explicitly with Cmd + Q
        if (process.platform !== "darwin") {
            app.quit()
        }
    })
}
