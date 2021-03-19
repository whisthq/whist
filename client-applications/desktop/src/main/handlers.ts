// import { webContents } from "electron"
// import { ipcMain } from "electron"
// import { Handler, State, StateChannel } from "@app/utils/state"
// import { store } from "@app/utils/persist"
// import { emailLogin } from "@app/utils/api"
// import { checkEmail, checkPassword } from "@app/utils/auth"

// export const handleIPC: Handler = (state, setState) => {
//     if (!state.initialized) {
//         ipcMain.on(StateChannel, (_, now: Partial<State>) => setState(now))
//     }

//     const contents = webContents.getAllWebContents()
//     contents.map((c) => {
//         c.isLoading()
//             ? c.on("did-finish-load", () => c.send(StateChannel, state))
//             : c.send(StateChannel, state)
//     })
// }

// export const handleAccessToken: Handler = (state, setState) => {
//     if (state.initialized) return
//     store.set("accessToken", "brains")
//     console.log(store.get("accessToken"))
//     setState({ accessToken: store.get("accessToken") })
//     // store.onDidChange("accessToken", (now, _old) =>
//     //     setState({ accessToken: now })
//     // )
// }

// export const handleLogin: Handler = async (state, setState) => {
//     if (!state.loginRequested) return
//     if (state.loginPending) return

//     const newState = {
//         loginPending: false,
//         loginRequested: false,
//         loginError: null,
//     }

//     if (!(checkEmail(state.email) && checkPassword(state.password)))
//         setState({ ...newState, loginError: "Invalid credentials on login." })

//     setState({ loginPending: true })
//     const response = await emailLogin(state.email, state.password)

//     if (response.response.status !== 200)
//         return setState({ ...newState, loginError: "Unexpected server error." })

//     if (!response.json.verified)
//         return setState({ ...newState, loginError: "Invalid credentials" })

//     if (!response.json.access_token)
//         return setState({ ...newState, loginError: "Unexpected server error." })

//     store.set("accessToken", response.json.access_token)

//     return setState({ ...newState })
// }

// export const logState: Handler = (state, _) => {
//     console.log(state)
// }
