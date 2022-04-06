import { app, BrowserWindow } from "electron"
import remove from "lodash.remove"

import {
  destroyElectronWindow,
  getElectronWindowHash,
} from "@app/main/utils/windows"

const relaunch = (options: { sleep: boolean }) => {
  if (process.platform === "win32") app.exit()

  // First destroy all Electron windows
  BrowserWindow.getAllWindows().forEach((win) => {
    const hash = getElectronWindowHash(win)
    if (hash !== undefined) destroyElectronWindow(hash)
  })

  let args = process.argv.slice(1)
  remove(args, (arg) => arg === "--sleep")
  args = options.sleep ? args.concat(["--sleep"]) : args

  // Then quit/relaunch the app
  app.relaunch({ args })

  // Because the omnibar is not closable by the user, we call app.exit() to force it to close as a
  // last resort
  app.exit()
}

export { relaunch }
