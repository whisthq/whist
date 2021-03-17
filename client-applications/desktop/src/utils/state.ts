import { useEffect, useState } from "react"
import { IpcRenderer, IpcRendererEvent } from "electron"

export type State = any
export type Event = (f: (state: Partial<State>) => void) => void
export type Effect = (state: State) => void

export const StateChannel = "MAIN_STATE_CHANNEL"

export const useMainState = (ipcRenderer: IpcRenderer) => {
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
