import { BrowserView, BrowserWindow } from "electron"

const createBrowserView = (url: string) => {
  const browserView = new BrowserView({
    webPreferences: {
      partition: "persist:whist",
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: true,
      enableRemoteModule: false,
      plugins: true,
      nativeWindowOpen: true,
      webSecurity: true,
      javascript: true,
      worldSafeExecuteJavaScript: false,
    },
  })

  browserView.setBackgroundColor("#ffffff")
  browserView.webContents?.loadURL(url)

  return browserView
}

const attachBrowserView = (view: BrowserView, window: BrowserWindow) => {
  window.addBrowserView(view)
  view.setBounds({
    x: 0,
    y: 0,
    width: window.getSize()[0],
    height: window.getSize()[1] - 20,
  })
  view.setAutoResize({
    width: true,
    height: true,
    horizontal: true,
    vertical: true,
  })
}

const setTopBrowserView = (
  view: BrowserView,
  win: BrowserWindow | undefined
) => {
  if (win === undefined) return
  // Set view to top
  win.setTopBrowserView(view)
}

export { createBrowserView, attachBrowserView, setTopBrowserView }
