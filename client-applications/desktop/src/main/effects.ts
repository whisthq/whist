import { webContents, BrowserWindow } from "electron"
import { emailLogin } from "@app/utils/api"
import { decryptConfigToken, createConfigToken } from "@app/utils/crypto"
import { createAuthWindow } from "@app/utils/windows"
import { fractalLoginWarning } from "@app/utils/constants"
import { Effect, StateChannel, isLoggedIn } from "@app/utils/state"
import { persistKeys } from "@app/utils/persist"
import * as proto from "@app/utils/protocol"

export const handleLogin: Effect = async function* (state) {
    // Only run if a login has been requested,
    // and if we're not already processing a login request
    if (!state.loginRequest) return
    if (state.loginLoading) return
    yield { loginLoading: true, loginWarning: "" }
    const { json } = await emailLogin(state.email, state.password)
    // Check for a json response and an accessToken
    // Set a warning for the user the response is invalid
    const loginWarning =
        json && json.access_token ? "" : fractalLoginWarning.INVALID
    if (loginWarning)
        return { loginLoading: false, loginRequest: false, loginWarning }

    return {
        password: "",
        loginWarning,
        loginLoading: false,
        loginRequest: false,
        accessToken: json.access_token || "",
        refreshToken: json.refresh_token || "",
        configToken: json.encrypted_config_token
            ? decryptConfigToken(state.password, json.encrypted_config_token)
            : await createConfigToken(),
    }
}

export const launchAuthWindow: Effect = function* (state) {
    // Only run if the auth window has been requested
    if (!state.appWindowRequest) return
    // If we're already logged in, do not show the app window
    if (isLoggedIn(state)) return { appWindowRequest: false }
    // Only show the auth window if there's not windows showing.
    if (BrowserWindow.getAllWindows().length === 0) {
        createAuthWindow()
    }
    // Clear auth window request after launching
    return { appWindowRequest: false }
}

export const closeAllWindows: Effect = function* (state) {
    // Any time we're logged in, we close all the windows.
    // This will need to change as soon as we introduce new
    // windows for errors or loading.
    if (!isLoggedIn(state)) return
    for (let win of BrowserWindow.getAllWindows()) win.close()
}

export const broadcastState: Effect = function* (state) {
    // We run this effect every time to make sure that all renderer
    // threads are receiving an updated state.
    const contents = webContents.getAllWebContents()
    contents.map((c) => {
        c.isLoading()
            ? c.on("did-finish-load", () => c.send(StateChannel, state))
            : c.send(StateChannel, state)
    })
}

export const launchProtocol: Effect = async function* (state) {
    // Only launch the protocol if we're logged in.
    if (!isLoggedIn(state)) return
    // Only launch the protocol if we're not already loading.
    if (state.protocolLoading) return
    // Only launch the protocol if one is not already open.
    if (state.protocolProcess) return
    yield { protocolLoading: true }
    // Make a request to create a container, and poll the task
    // endpoint until we receive a response that the container
    // is ready.
    const taskID = await proto.createContainer(state.email, state.accessToken)
    const info = await proto.waitUntilReady(taskID, state.accessToken)
    return {
        protocolLoading: false,
        protocolProcess: proto.launchProtocol(info.output),
    }
}

export const persistState: Effect = function* (state) {
    // Persist select keys to storage on every state update
    persistKeys(state, "email", "accessToken", "configToken")
}

export const logState: Effect = function* (state) {
    // Print every state update out to the console
    console.log(state)
}
