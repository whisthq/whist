import {
  app,
  BrowserView,
  BrowserWindow,
  BrowserWindowConstructorOptions,
} from "electron"

import { base } from "@app/constants/windows"

// Helper functions

const replaceUserAgent = (userAgent: string) =>
  userAgent.replace(/Electron\/*/, "").replace(/Chrome\/*/, "")

const createView = (url: string, win?: BrowserWindow) => {
  const view = new BrowserView()
  view.webContents.loadURL(url).catch((err) => console.error(err))
  view.webContents.userAgent = replaceUserAgent(view.webContents.userAgent)

  if (win !== undefined) {
    win.setBrowserView(view)
    view.setBounds({
      x: 20,
      y: 20,
      width: win.getSize()[0] - 40,
      height: win.getSize()[1] - 40,
    })
  }

  return view
}

const getElectronWindow = (hash: string) => {
  for (const win of BrowserWindow.getAllWindows()) {
    if (win.webContents.getURL()?.split("show=")?.[1] === hash) return win
  }
  return undefined
}

// Main functions

const hideElectronWindow = (hash: string) => {
  getElectronWindow(hash)?.hide()
}

const showElectronWindow = (hash: string) => {
  getElectronWindow(hash)?.show()
}

const destroyElectronWindow = (hash: string) => {
  getElectronWindow(hash)?.destroy()
}

const isElectronWindowVisible = (hash: string) =>
  getElectronWindow(hash)?.isVisible() ?? false

const createElectronWindow = (args: {
  width: number
  height: number
  hash: string
  options?: Partial<BrowserWindowConstructorOptions>
  customURL?: string
  show?: boolean
}) => {
  // Create the BrowserWindow
  const win = new BrowserWindow({
    ...(base as BrowserWindowConstructorOptions),
    ...(args.options !== undefined && args.options),
    width: args.width,
    height: args.height,
    show: args.show ?? true,
  })

  // Some websites don't like Electron in the user agent, so we remove it
  win.webContents.userAgent = replaceUserAgent(win.webContents.userAgent)

  if (args.customURL !== undefined) {
    // If we want to load a URL into a BrowserWindow, we first load it into a
    // BrowserView and attach it to the BrowserWindow. This is our trick to make the
    // window both frameless and draggable.
    return { win, view: createView(args.customURL, win) }
  } else {
    // Electron doesn't have a API for passing data to renderer windows. We need
    // to pass "init" data for a variety of reasons, but mainly so we can decide
    // which React component to render into the window. We're forced to do this
    // using query parameters in the URL that we pass. The alternative would
    // be to use separate index.html files for each window, which we want to avoid.
    const params = `?show=${args.hash}`
    if (app.isPackaged) {
      win
        .loadFile("build/index.html", { search: params })
        .catch((err) => console.error(err))
    } else {
      win
        .loadURL(`http://localhost:8080${params}`)
        .catch((err) => console.error(err))
    }

    return { win, view: undefined }
  }
}

export {
  hideElectronWindow,
  showElectronWindow,
  destroyElectronWindow,
  createElectronWindow,
  isElectronWindowVisible,
}
