import { webContents, BrowserWindow } from "electron"
import { emailLogin } from "@app/utils/api"
import { decryptConfigToken } from "@app/utils/crypto"
import { createAuthWindow } from "@app/utils/windows"
import { fractalLoginWarning } from "@app/utils/constants"
import { Effect, StateChannel, isLoggedIn } from "@app/utils/state"
import { persistKeys } from "@app/utils/persist"
import * as proto from "@app/utils/protocol"

export const handleLogin: Effect = async function* (state) {
    if (!state.loginRequest) return
    if (state.loginLoading) return
    yield { loginLoading: true, loginWarning: "" }
    const { json } = await emailLogin(state.email, state.password)
    const loginWarning =
        json && json.access_token ? "" : fractalLoginWarning.INVALID
    yield { loginLoading: false, loginWarning }

    return {
        password: "",
        loginRequest: false,
        accessToken: json.access_token || "",
        refreshToken: json.refresh_token || "",
        configToken: json.encrypted_config_token
            ? decryptConfigToken(state.password, json.encrypted_config_token)
            : "",
    }
}

export const launchAuthWindow: Effect = function* (state) {
    // if (!state.appWindowRequest) return
    if (isLoggedIn(state)) return { appWindowRequest: false }
    if (BrowserWindow.getAllWindows().length === 0) {
        createAuthWindow()
    }
    return { appWindowRequest: false }
}

export const closeAllWindows: Effect = function* (state) {
    if (!isLoggedIn(state)) return
    for (let win of BrowserWindow.getAllWindows()) win.close()
}

export const broadcastState: Effect = function* (state) {
    const contents = webContents.getAllWebContents()
    contents.map((c) => {
        c.isLoading()
            ? c.on("did-finish-load", () => c.send(StateChannel, state))
            : c.send(StateChannel, state)
    })
}

export const launchProtocol: Effect = async function* (state) {
    if (!isLoggedIn(state)) return
    if (state.protocolLoading) return
    if (state.protocolProcess) return
    yield { protocolLoading: true }
    const taskID = await proto.createContainer(state.email, state.accessToken)
    const info = await proto.waitUntilReady(taskID, state.accessToken)
    return {
        protocolLoading: false,
        protocolProcess: proto.launchProtocol(info.output),
    }
}

export const persistState: Effect = function* (state) {
    persistKeys(state, "email", "accessToken")
}

export const logState: Effect = function* (state) {
    console.log(state)
}
