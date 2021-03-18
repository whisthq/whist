import { useEffect, useState } from "react"
import { IpcRenderer, IpcRendererEvent } from "electron"

export type State = any
export type Event = (setState: (state: Partial<State>) => void) => void
export type Effect = (state: State) => void

export const StateChannel = "MAIN_STATE_CHANNEL"

const ipcError = [
    "Before you call useMainState(),",
    "an ipcRenderer must be attached to the renderer window object to",
    "communicate with the main process.",
    "\n\nYou need to attach it in an Electron preload.js file using",
    "contextBridge.exposeInMainWorld. You must explicity attach the 'on' and",
    "'send' methods for them to be exposed.",
].join(" ")

export const useMainState = (): [State, (s: State) => void] | never => {
    // the window type doesn't have ipcRenderer, but we've manually
    // added that in preload.js with electron.contextBridge
    // so we ignore the type error in the next line
    //@ts-ignore
    const ipc = window.ipcRenderer
    if (!(ipc && ipc.on && ipc.send)) throw new Error(ipcError)

    const [mainState, setState] = useState({})

    useEffect(() => {
        const listener = (_: IpcRendererEvent, state: State) => setState(state)
        ipc.on(StateChannel, listener)
        return () => {
            ipc.removeListener && ipc.removeListener(StateChannel, listener)
        }
    }, [])

    const setMainState = (state: State) => {
        ipc.send(StateChannel, state)
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
    setState(state)
}
