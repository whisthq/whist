import { Event, State, StateChannel } from "@app/utils/state"
import { ipcMain } from "electron"

export const listenState: Event = (setState) => {
    ipcMain.on(StateChannel, (_, state: Partial<State>) => setState(state))
}
