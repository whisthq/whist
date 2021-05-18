import { useEffect, useState } from "react"
import { every } from "lodash"
import { IpcRendererEvent, BrowserWindow } from "electron"
import { StateIPC } from "@app/@types/state"
import { ErrorIPC, StateChannel } from "@app/utils/constants"

export const useMainState = (): [any, (s: object) => void] | never => {
  // the window type doesn't have ipcRenderer, but we've manually
  // added that in preload.js with electron.contextBridge
  // so we ignore the type error in the next line
  // @ts-expect-error
  const ipc = window.ipcRenderer
  if (!every([ipc, ipc.on, ipc.send])) throw new Error(ErrorIPC)

  const [mainState, setState] = useState({} as object)

  useEffect(() => {
    const listener = (_: IpcRendererEvent, state: object) => setState(state)
    ipc.on(StateChannel, listener)
    return () => {
      ipc.removeListener?.(StateChannel, listener)
    }
  }, [])

  const setMainState = (state: Partial<StateIPC>) => {
    ipc.send(StateChannel, state)
  }

  return [mainState, setMainState]
}

export const ipcBroadcast = (
  state: Partial<StateIPC>,
  windows: BrowserWindow[]
) => {
  const contents = windows.map((win) => win.webContents)
  contents.forEach((c) => {
    c.isLoading()
      ? c.on("did-finish-load", () => c.send(StateChannel, state))
      : c.send(StateChannel, state)
  })
}
