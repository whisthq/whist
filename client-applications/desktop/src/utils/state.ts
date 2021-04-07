import { useEffect, useState } from 'react'
import { IpcRendererEvent, BrowserWindow } from 'electron'
import { StateIPC } from '@app/utils/types'
import { ErrorIPC, StateChannel } from '@app/utils/constants'

export const useMainState = ():
| [StateIPC, (s: Partial<StateIPC>) => void]
| never => {
  // the window type doesn't have ipcRenderer, but we've manually
  // added that in preload.js with electron.contextBridge
  // so we ignore the type error in the next line
  // @ts-expect-error
  const ipc = window.ipcRenderer
  if (!(ipc && ipc.on && ipc.send)) throw new Error(ErrorIPC)

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

export const ipcBroadcast = (
  state: Partial<StateIPC>,
  windows: BrowserWindow[]
) => {
  const contents = windows.map((win) => win.webContents)
  contents.map((c) => {
    c.isLoading()
      ? c.on('did-finish-load', () => c.send(StateChannel, state))
      : c.send(StateChannel, state)
  })
}
