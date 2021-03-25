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
    // let cached = new Set()
    let state = {} as State

    // const consumeEffectAsync = async (eff: Effect, set: Function) => {
    //     cached.add(eff)
    //     // update state with yield objects
    //     for await (let newState of eff(state as State)) set(newState) // use passed function to avoid mutual recursion
    //     cached.delete(eff) // make effect available to run again
    // }

    // const consumeEffect = (eff: Effect, set: Function) => {
    //     for (let newState of eff(state as State) as Generator) set(newState)
    // }

    const reduceEffects = async (effects: Effect[], setState: Function) => {
        let nexts
        const iters = effects.map((eff) => eff(state))
        do {
            nexts = await Promise.all(iters.map((i) => i.next()))
            let newState = nexts.map((n) => n.value).reduce(merge, {})
            // console.log("newState", newState)
            if (!isEmpty(newState)) setState(newState)
        } while (nexts.some((n) => !n.done))
    }

    const setState = (newState: Partial<State>) => {
        const prevState = { ...state }
        merge(state, newState) // mutate state object with new changes
        // console.log("OLDSTATE", prevState, "STATE", state, "NEWSTATE", newState)
        // if (isEqual(prevState, state)) return
        reduceEffects(effects, setState)
        // run each of the effects
        // don't re-run running effects
        // for (let eff of effects.filter((eff) => !cached.has(eff)))
        //     if (eff.constructor.name.startsWith("Async"))
        //         consumeEffectAsync(eff, setState)
        //     else consumeEffect(eff, setState)
    }

    // run event functions once on initialization
    for (let event of events) event(setState)

    // initialize the state with default values
    setState(init)
}

export const isLoggedIn = (state: State): boolean => {
    return state.accessToken && state.email ? true : false
}
