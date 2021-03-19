import { webContents } from "electron"
import { Effect, State, StateChannel } from "@app/utils/state"

export const broadcastState: Effect = (state: any) => {
    const contents = webContents.getAllWebContents()
    contents.map((c) => {
        c.isLoading()
            ? c.on("did-finish-load", () => c.send(StateChannel, state))
            : c.send(StateChannel, state)
    })
}

export const launchProtocol: Effect = (_state: State) => {}
