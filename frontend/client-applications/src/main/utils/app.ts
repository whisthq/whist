import { app, BrowserWindow } from "electron"
import { destroyElectronWindow } from "@app/main/utils/windows"

const relaunch = (options?: { sleep: boolean }) => {
  // First destroy all Electron windows
  BrowserWindow.getAllWindows().forEach((win) => {
    const hash = win.webContents.getURL()?.split("show=")?.[1]
    destroyElectronWindow(hash)
  })

  // Then quit/relaunch the app
  options?.sleep ?? false
    ? app.relaunch({ args: process.argv.slice(1).concat(["--sleep"]) })
    : app.relaunch()
  app.quit()
}

export { relaunch }
