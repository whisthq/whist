import { screen, BrowserWindow, BrowserView } from "electron"

const createBrowserWindow = () => {
  /*
    Create the primary "base" browser window
    Args:
        none
    Returns:
        win: BrowserWindow
   */
  const { width, height } = screen.getPrimaryDisplay().workAreaSize

  const browserWindow = new BrowserWindow({
    width,
    height,
    backgroundColor: "#ffffff",
    frame: false,
    webPreferences: {
      partition: "persist:fractal",
      plugins: true,
      enableRemoteModule: true,
      worldSafeExecuteJavaScript: false,
    },
  })

  return browserWindow
}

export { createBrowserWindow }
