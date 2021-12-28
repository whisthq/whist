// This is the application's main entry point

import { app, BrowserWindow } from "electron"

app.on("ready", () => {
  const win = new BrowserWindow({
    fullscreen: true,
  })

  win.loadURL("https://www.google.com")
  win.show()
})
