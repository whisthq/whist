import { app, BrowserWindow } from "electron"
import remove from "lodash.remove"

import { destroyElectronWindow } from "@app/main/utils/windows"

const relaunch = (options: { sleep: boolean }) => {
  // First destroy all Electron windows
  BrowserWindow.getAllWindows().forEach((win) => {
    const hash = win.webContents.getURL()?.split("show=")?.[1]
    destroyElectronWindow(hash)
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
