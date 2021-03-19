import { Event, State, StateChannel } from "@app/utils/state"
import { ipcMain } from "electron"
import { store } from "@app/utils/persist"
import { emailLogin } from "@app/utils/api"

// export const handleLogin: Handler = async (state, setState) => {
//     if (!state.loginRequested) return
//     if (!state.loginPromise)
//         return { loginPromise: emailLogin(state.email, state.password) }
//     if ()
//     setState({ loginRequested: false, loginPending: true })

//     const response = await emailLogin("neil@fractal.co", "Password1234")
//     if (!response.json.verified)
//         return setState({
//             loginError: "Invalid login credentials.",
//             loginPending: false,
//         })
//     if (!response.json.accessToken)
//         return setState({
//             loginError: "Unexpected login error.",
//             loginPending: false,
//         })

//     return setState({
//         accessToken: response.json.accessToken,
//         loginPending: false,
//     })
// }

const handleLoginRequest = async (state: State) => {
    if (!state.loginRequested) return state
    return {
        ...state,
        loginPromise: emailLogin("neil@fractal.co", "Password1234"),
    }
}

export const listenState: Event = (setState) => {
    ipcMain.on(StateChannel, (_, state: Partial<State>) => {
        // handleLogin(state, setState)
    })
}

export const listenAccessToken: Event = (setState) => {
    store.onDidChange("accessToken", (now, _old) =>
        setState({ accessToken: now })
    )
}
