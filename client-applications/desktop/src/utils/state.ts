import { useEffect, useState } from "react"
import { IpcRendererEvent } from "electron"
import { isEqual, isEmpty, merge } from "lodash"
import { ChildProcess } from "child_process"

export type State = {
    email: string
    password: string
    configToken: string
    accessToken: string
    refreshToken: string
    loginRequest: boolean
    loginLoading: boolean
    loginWarning: string
    protocolLoading: boolean
    appWindowRequest: boolean
    protocolProcess?: ChildProcess
}
export type StateIPC = Omit<State, "protocolProcess">
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

export const initState = async (
    init: Partial<State>,
    events: Event[],
    effects: Effect[]
) => {
    const cache = new Set()
    const state = {} as State

    const reduceEffects = async (
        effs: Effect[],
        setState: Function
    ): Promise<void> => {
        // we cache running effects so that async effects are not run
        // again before they return.
        // we filter out effects that are in the cache
        effs = effs.filter((eff) => !cache.has(eff))
        // add all remaining effects to the cache
        effs.forEach((eff) => cache.add(eff))
        // Call each effect with the current state
        const iters = effs.map((eff) => eff(state))

        let updates
        do {
            // start all effects and wait for return or first yield
            updates = await Promise.all(iters.map((i) => i.next()))
            // if effects have returned, remove them from cache
            updates.map((el, idx) => el.done && cache.delete(effs[idx]))
            // merge all the updated states into a single object
            let newState = updates.map((u) => u.value).reduce(merge, {})
            // only run setState if there's actually updates.
            // call setState so that the state updates are merged with
            // the current state value
            if (!isEmpty(newState)) setState(newState)
        } while (updates.some((u) => !u.done))
    }

    const setState = async (newState: Partial<State>) => {
        const prevState = { ...state }
        merge(state, newState) // mutate state object with new changes
        // effects run on every state update
        // setState will not be called unless there are actually
        // updates to merge
        // effects will run until the state is "settled"
        if (!isEqual(state, prevState)) reduceEffects(effects, setState)
    }

    // run event functions once on initialization
    for (let event of events) event(setState)

    // initialize the state with default values
    setState(init)
}

export const isLoggedIn = (state: State): boolean => {
    return state.accessToken && state.email && state.configToken ? true : false
}
