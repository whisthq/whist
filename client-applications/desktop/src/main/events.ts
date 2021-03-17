import { Event, State, StateChannel } from "@app/utils/state"
import { ipcMain } from "electron"

const listenStateRenderer: Event = (setState) => {
    ipcMain.on(StateChannel, (_, state: Partial<State>) => setState(state))
}
