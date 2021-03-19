import { useEffect, useState } from "react"
import { IpcRendererEvent } from "electron"
import { isEqual, merge } from "lodash"

export type State = {
    accessToken: string | ""
    appWindowRequested: boolean | false
}
export type Event = (setState: (state: Partial<State>) => void) => void
export type Effect = (
    state: Partial<State>
) => Partial<State> | Promise<Partial<State>>

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
    init: Partial<State>,
    events: Event[],
    effects: Effect[]
) => {
    let cached = {}
    let state = { ...init }

    const setState = (newState: Partial<State>) => {
        merge(state, newState)
    }

    const reduceEffects = async (effs: Effect[], newState: Partial<State>) => {
        for (let eff of effs) newState = await eff(newState)
        return newState
    }

    for (let event of events) event(setState)

    while (true) {
        await sleep(100)
        if (!isEqual(cached, state)) {
            console.log({ ...state })
            setState(await reduceEffects(effects, { ...state }))
            cached = { ...state }
        }
    }
}
