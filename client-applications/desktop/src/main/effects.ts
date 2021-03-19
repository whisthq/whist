import { webContents } from "electron"
import { Effect, State, StateChannel } from "@app/utils/state"
import { emailLogin, tokenValidate } from "@app/utils/api"
import { checkEmail, checkPassword } from "@app/utils/auth"
import { store } from "@app/utils/persist"
import { log } from "@app/utils/logging"

export const broadcastState: Effect = (state: any) => {
    const contents = webContents.getAllWebContents()
    contents.map((c) => {
        c.isLoading()
            ? c.on("did-finish-load", () => c.send(StateChannel, state))
            : c.send(StateChannel, state)
    })
    return state
}

export const launchProtocol: Effect = async (state: State) => {
    console.log(state)
    if (!state.accessToken) return state

    const response = await tokenValidate(state.accessToken)
    console.log("launch response", response)
    if (!response.json.verified) return state

    console.log("LAUNCHING PROTOCOL WATCH OUT")
    return state
}

export const storeAccess: Effect = async (state: State) => {
    const r = await emailLogin("neil@fractal.co", "Password1234")
    // setTimeout(() => store.set("accessToken", "empty"), 1000)
    // setTimeout(() => store.set("accessToken", "full"), 2000)
    store.set("accessToken", r.json.access_token)
    // const t = store.get("accessToken")
    // console.log(await tokenValidate(r.json.access_token))
    return state
}

export const handleLogin: Effect = async (state) => {
    const newState = {
        loginRequested: false,
        loginPromise: null,
        loginError: null,
    }

    if (!state.loginRequested) return

    if (!(checkEmail(state.email) && checkPassword(state.password)))
        return { ...newState, loginError: "Invalid credentials on login." }

    if (!state.loginPromise)
        return { loginPromise: emailLogin(state.email, state.password) }

    const response = await state.loginPromise

    if (response.response.status !== 200)
        return { ...newState, loginError: "Unexpected server error." }

    if (!response.json.verified)
        return { ...newState, loginError: "Invalid credentials" }

    if (!response.json.access_token)
        return { ...newState, loginError: "Unexpected server error." }

    // store.set("accessToken", response.json.access_token)
    return { ...newState, accessToken: response.json.access_token }
}

export const logState: Effect = (state: State) => {
    log.info(state)
    return state
}
