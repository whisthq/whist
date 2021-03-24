import { useEffect, useState } from "react"
import { IpcRendererEvent } from "electron"
import { isEqual, merge } from "lodash"
import { ChildProcess } from "child_process"

export type State = {
    email: string
    accessToken: string
    refreshToken: string
    protocolLoading: boolean
    appWindowRequested: boolean
    protocolProcess?: ChildProcess
}
export type Event = (setState: (state: Partial<State>) => void) => void
export type Effect = (
    s: State
) => Generator<State, State> | AsyncGenerator<State, State>

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
    | [Partial<State>, (s: Partial<State>) => void]
    | never => {
    // the window type doesn't have ipcRenderer, but we've manually
    // added that in preload.js with electron.contextBridge
    // so we ignore the type error in the next line
    //@ts-ignore
    const ipc = window.ipcRenderer
    if (!(ipc && ipc.on && ipc.send)) throw new Error(ipcError)

    const [mainState, setState] = useState({} as Partial<State>)

    useEffect(() => {
        const listener = (_: IpcRendererEvent, state: Partial<State>) =>
            setState(state)
        ipc.on(StateChannel, listener)
        return () => {
            ipc.removeListener && ipc.removeListener(StateChannel, listener)
        }
    }, [])

    const setMainState = (state: Partial<State>) => {
        ipc.send(StateChannel, state)
    }

    return [mainState, setMainState]
}

export const initState = async (
    init: State,
    events: Event[],
    effects: Effect[]
) => {
    let cached = new Map()
    let state = {}

    const consumeEffect = async (eff: Effect, set: Function) => {
        const iter = eff(state as State) //start the generator
        for await (let newState of iter) // update state with yield objects
            set(newState) // use passed function to avoid mutual recursion
        cached.delete(eff) // make effect available to run again
    }

    const setState = (newState: Partial<State>) => {
        const prevState = { ...state }
        merge(state, newState) // mutate state object with new changes
        if (isEqual(prevState, state)) return
        for (let eff of effects) // run each of the effects
            if (!cached.has(eff))
                // don't re-run running effects
                cached.set(eff, consumeEffect(eff, setState))
    }
    // run event functions once on initialization
    for (let event of events) event(setState)
    // initialize the state with default values
    setState(init)
}
