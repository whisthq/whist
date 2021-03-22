import { webContents, BrowserWindow } from "electron"
import { createAuthWindow } from "@app/utils/windows"
import { Effect, State, StateChannel } from "@app/utils/state"
import { store } from "@app/utils/persist"

export const launchAuthWindow: Effect = (state: any) => {
    if (state.accessToken) return state
    if (!state.appWindowRequested) return state
    if (BrowserWindow.getAllWindows().length === 0) {
        createAuthWindow()
    }
    return { ...state, appWindowRequested: false }
}

export const closeAllWindows: Effect = (state: any) => {
    if (!state.accessToken) return
    for (let win of BrowserWindow.getAllWindows()) win.close()
    return state
}

export const broadcastState: Effect = (state: any) => {
    const contents = webContents.getAllWebContents()
    contents.map((c) => {
        c.isLoading()
            ? c.on("did-finish-load", () => c.send(StateChannel, state))
            : c.send(StateChannel, state)
    })
    return state
}

export const launchProtocol: Effect = async (state) => {
    if (!state.accessToken) return state

    console.log("LAUNCHING PROTOCOL WATCH OUT")
    return state
}

export const storeAccess: Effect = async (state) => {
    if (!state.accessToken) return state
    store.set("accessToken", state)
    return state
}

export const logState: Effect = (state) => {
    console.log(state)
    return state
}
