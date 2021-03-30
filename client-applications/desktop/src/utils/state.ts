import { useEffect, useState } from "react"
import { isEqual, isEmpty, merge } from "lodash"
import { IpcRendererEvent, BrowserWindow } from "electron"
import { ChildProcess } from "child_process"
import { getWindows } from "@app/utils/windows"

export type State = {
    email: string
    password: string
    configToken: string
    accessToken: string
    refreshToken: string
    displayError: string
    loginRequests: number[]
    loginLoading: boolean
    loginWarning: string
    protocolLoading: boolean
    appWindowRequest: boolean
    protocolProcess?: ChildProcess
    windowAuth?: BrowserWindow
    windowError?: BrowserWindow
}
export type StateIPC = Omit<
    State,
    "protocolProcess" | "windowAuth" | "windowError"
>
export type Event = (setState: (state: Partial<State>) => void) => void
export type Effect = (
    s: State
) =>
    | Generator<Partial<State>, Partial<State> | void>
    | AsyncGenerator<Partial<State>, Partial<State> | void>

export const StateChannel = "MAIN_STATE_CHANNEL"

const ipcError = [
    "Before you call useMainState(),",
    "an ipcRenderer must be attached to the renderer window object to",
    "communicate with the main process.",
    "\n\nYou need to attach it in an Electron preload.js file using",
    "contextBridge.exposeInMainWorld. You must explicity attach the 'on' and",
    "'send' methods for them to be exposed.",
].join(" ")

export const useMainState = ():
    | [StateIPC, (s: Partial<StateIPC>) => void]
    | never => {
    // the window type doesn't have ipcRenderer, but we've manually
    // added that in preload.js with electron.contextBridge
    // so we ignore the type error in the next line
    //@ts-ignore
    const ipc = window.ipcRenderer
    if (!(ipc && ipc.on && ipc.send)) throw new Error(ipcError)

    const [mainState, setState] = useState({} as StateIPC)

    useEffect(() => {
        const listener = (_: IpcRendererEvent, state: StateIPC) =>
            setState(state)
        ipc.on(StateChannel, listener)
        return () => {
            ipc.removeListener && ipc.removeListener(StateChannel, listener)
        }
    }, [])

    const setMainState = (state: Partial<StateIPC>) => {
        ipc.send(StateChannel, state)
    }

    return [mainState, setMainState]
}

export const ipcBroadcast = (
    state: Partial<State>,
    windows: BrowserWindow[]
) => {
    const contents = windows.map((win) => win.webContents)
    contents.map((c) => {
        c.isLoading()
            ? c.on("did-finish-load", () => c.send(StateChannel, state))
            : c.send(StateChannel, state)
    })
}
