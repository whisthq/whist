import { webContents, BrowserWindow } from "electron"
import { createAuthWindow } from "@app/utils/windows"
import { Effect, StateChannel } from "@app/utils/state"
import { persistKeys } from "@app/utils/persist"
import * as proto from "@app/utils/protocol"

export const launchAuthWindow: Effect = (state: any) => {
    if (state.accessToken && state.email) return state
    if (!state.appWindowRequested) return state
    if (BrowserWindow.getAllWindows().length === 0) {
        createAuthWindow()
    }
    return { ...state, appWindowRequested: false }
}

export const closeAllWindows: Effect = (state: any) => {
    if (!(state.accessToken && state.email)) return
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
    if (!(state.accessToken && state.email)) return state
    const taskID = await proto.createContainer(state.email, state.accessToken)
    const info = await proto.waitUntilReady(taskID, state.accessToken)
    const protocol = proto.launchProtocol(info.output)
    // proto.signalProtocolInfo(protocol, info)
    console.log(info)
    return state
}

export const persistState: Effect = async (state) => {
    persistKeys(state, "email", "accessToken")
    return state
}

export const logState: Effect = (state) => {
    console.log(state)
    return state
}
