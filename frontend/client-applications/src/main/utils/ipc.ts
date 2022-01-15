/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file ipc.ts
 * @brief This file contains utility functions for IPC communication between the main and renderer
 * thread. This file is imported both by the main and the renderer process, so it's
 * important that it doesn't use any functionality that can't be bundled into the browser.
 */

import { BrowserWindow } from "electron"
import { StateIPC } from "@app/@types/state"
import { StateChannel } from "@app/constants/ipc"

// This function should only be called by the main process. We use it to pass
// a "state" object over to a list of BrowserWindows through IPC.
export const ipcBroadcast = (
  state: Partial<StateIPC>,
  windows: BrowserWindow[]
) => {
  const contents = windows.map((win) => win.webContents)
  // Sometimes we might broadcast an update while a window is being created.
  // If the window hasn't finished loading, the React component inside it will
  // not receive the update. If we see that the window object is still loading,
  // we wait for it to finish before sending the update.
  contents.forEach((c) => {
    c.isLoading()
      ? c.on("did-finish-load", () => c.send(StateChannel, state))
      : c.send(StateChannel, state)
  })
}
