import { webContents, BrowserWindow } from "electron"
import { createAuthWindow } from "@app/utils/windows"
import { Effect, StateChannel } from "@app/utils/state"
import { persistKeys } from "@app/utils/persist"
import * as proto from "@app/utils/protocol"

export const launchAuthWindow: Effect = function* (state) {
    if (state.accessToken && state.email) return state
    if (!state.appWindowRequested) return state
    if (BrowserWindow.getAllWindows().length === 0) {
        createAuthWindow()
    }
    return { ...state, appWindowRequested: false }
}

export const closeAllWindows: Effect = function* (state) {
    if (!(state.accessToken && state.email)) return state
    for (let win of BrowserWindow.getAllWindows()) win.close()
    return state
}

export const broadcastState: Effect = function* (state) {
    const contents = webContents.getAllWebContents()
    contents.map((c) => {
        c.isLoading()
            ? c.on("did-finish-load", () => c.send(StateChannel, state))
            : c.send(StateChannel, state)
    })
    return state
}

export const launchProtocol: Effect = async function* (state) {
    if (!(state.accessToken && state.email)) return state
    yield { ...state, protocolLoading: true }
    const taskID = await proto.createContainer(state.email, state.accessToken)
    const info = await proto.waitUntilReady(taskID, state.accessToken)
    yield { ...state, protocolLoading: false }
    return { ...state, protocolProcess: proto.launchProtocol(info.output) }
}

export const persistState: Effect = function* (state) {
    persistKeys(state, "email", "accessToken")
    return state
}

export const logState: Effect = function* (state) {
    console.log(state)
    return state
}
