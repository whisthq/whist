import { useEffect, useState } from "react"
import { IpcRenderer, IpcRendererEvent } from "electron"
import { isEqual, merge } from "lodash"

export type State = any
export type Event = (setState: (state: Partial<State>) => void) => void
export type Effect = (state: State) => Partial<State> | Promise<Partial<State>>
// export type Handler = (
//     state: Partial<State>,
//     setState: (s: Partial<State>) => void
// ) => void

export const StateChannel = "MAIN_STATE_CHANNEL"

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms))

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

export const initState = async (
    init: State,
    events: Event[],
    effects: Effect[]
) => {
    let cached = {}
    let state = { ...init }

    const setState = async (now: Partial<State> | Promise<Partial<State>>) => {
        merge(state, await now)
    }
    for (let event of events) event(setState)

    while (true) {
        await sleep(100)
        if (!isEqual(cached, state)) {
            for (let effect of effects) setState(effect({ ...state }))
            cached = { ...state }
        }
    }
}

// //
// export const initState = (init: State, ...handlers: Handler[]) => {
//     let state = { initialized: false }
//     const setState = (newState: Partial<State>): void => {
//         if (!isEqual(state, { ...state, newState })) {
//             state = { ...state, ...newState }
//             for (let handler of handlers)
//                 handler({ ...state, initialized: true }, setState)
//         }
//     }
//     setState({ ...state, ...init })
// }
