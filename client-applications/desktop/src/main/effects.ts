import { webContents } from "electron"
import { Effect, State, StateChannel } from "@app/utils/state"

export const broadcastStateRenderer: Effect = (state: any) => {
    const contents = webContents.getAllWebContents()
    contents.map((c) => c.send(StateChannel, state))
}

export const launchProtocol: Effect = (_state: State) => {}
