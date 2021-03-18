import { useEffect, useState } from "react"
import { IpcRenderer, IpcRendererEvent } from "electron"

export type State = any
export type Event = (setState: (state: Partial<State>) => void) => void
export type Effect = (state: State) => void

export const StateChannel = "MAIN_STATE_CHANNEL"

const ipcError = [
    "A ipcRenderer must be attached to the window object to commmunicate",
    "with the main process.",
    "You need to attach it in a Electron preload.js file using",
    "contextBridge.exposeInMainWorld. You must explicity attach the 'on' and",
    "'send' methods for them to be exposed.",
].join(" ")

export const useMainState = (ipcRenderer: IpcRenderer) => {
    const ipc = window.ipcRenderer
    if (!(ipc && ipc.on && ipc.send)) throw new Error("Y")

    const [mainState, setState] = useState(null)

    useEffect(() => {
        const listener = (_: IpcRendererEvent, state: State) => setState(state)
        ipcRenderer.on(StateChannel, listener)
        return () => {
            ipcRenderer.removeListener(StateChannel, listener)
        }
    }, [])

    const setMainState = (state: State) => {
        ipcRenderer.send(StateChannel, state)
    }

    return [mainState, setMainState]
}

export const initState = (init: State, events: Event[], effects: Effect[]) => {
    let state = { ...init }
    const setState = (newState: Partial<State>) => {
        state = { ...state, ...newState }
        for (let effect of effects) effect({ ...state })
    }
    events.map((e) => e(setState))
}
