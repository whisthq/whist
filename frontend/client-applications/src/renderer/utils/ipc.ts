/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file ipc.ts
 * @brief This file contains utility functions for IPC communication between the main and renderer
 * thread. This file is imported both by the main and the renderer process, so it's
 * important that it doesn't use any functionality that can't be bundled into the browser.
 */

import { useEffect, useState } from "react"
import { IpcRendererEvent } from "electron"
import { StateIPC } from "@app/@types/state"
import { StateChannel } from "@app/constants/ipc"

// This function should only be called from the renderer process. It's a React
// hook that simplifies our IPC coordination with the main process.
export const useMainState = ():
  | [StateIPC, (s: Partial<StateIPC>) => void]
  | never => {
  // The window type doesn't have ipcRenderer, but we've manually
  // added that in preload.js with electron.contextBridge
  // so we ignore the type error in the next line
  // @ts-expect-error
  const ipc = window.ipcRenderer

  // Things are looking good! We'll proceed by making a standard "state" React
  // Effect, which will perform the ipc communication that we need.
  const [mainState, setState] = useState({} as StateIPC)

  useEffect(() => {
    // We're using a single IPC channel for all communication in this app.
    // We import it here as a constant and pass it to the event listener.
    const listener = (_: IpcRendererEvent, state: StateIPC) => setState(state)
    ipc.on(StateChannel, listener)

    // useEffect allows you to return a function that will be called to perform
    // "cleanup". In our case, we'll use it to remove our event listener.
    return () => {
      ipc.removeListener?.(StateChannel, listener)
    }
    // useEffect needs us to pass this empty list as a second argument, which
    // signifies that we only want this function to run once when the component
    // mounts.
  }, [])

  // This is the function that we'll use to "send back" data to the main process.
  // We expect to pass a partial "state" object so that we can closely mimic the
  // setState API that the developer is used to.
  const setMainState = (state: Partial<StateIPC>) => {
    ipc.send(StateChannel, state)
  }

  return [mainState, setMainState]
}
