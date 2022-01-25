import path from "path"
import {
  app,
  BrowserView,
  BrowserWindow,
  BrowserWindowConstructorOptions,
} from "electron"

import config from "@app/config/environment"

// Helper functions
const base = {
  webPreferences: {
    preload: path.join(config.buildRoot, "preload.js"),
  },
  resizable: false,
  titleBarStyle: "hidden",
  frame: false,
  border: false,
  show: false,
  opacity: 0.0,
}

const replaceUserAgent = (userAgent: string) =>
  userAgent.replace(/Electron\/*/, "").replace(/Chrome\/*/, "")

const createView = (url: string, win?: BrowserWindow) => {
  const view = new BrowserView()
  view.webContents.loadURL(url).catch((err) => console.error(err))
  view.webContents.userAgent = replaceUserAgent(view.webContents.userAgent)

  if (win !== undefined) {
    win.setBrowserView(view)
    view.setBounds({
      x: 0,
      y: 30,
      width: win.getSize()[0],
      height: win.getSize()[1] - 30,
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

const fadeElectronWindowIn = (
  win: BrowserWindow,
  step = 0.1,
  fadeEveryXSeconds = 25
) => {
  win.show()
  if (win.isMinimized()) win.restore()

  // Get the opacity of the window.
  let opacity = win.getOpacity()

  // Increase the opacity of the window by `step` every `fadeEveryXSeconds`
  // seconds.
  const interval = setInterval(() => {
    // Stop fading if window's opacity is 1 or greater.
    if (opacity >= 1) clearInterval(interval)
    win.setOpacity(opacity)
    opacity += step
  }, fadeEveryXSeconds)
}

const fadeElectronWindowOut = (
  win: BrowserWindow,
  step = 0.1,
  fadeEveryXSeconds = 25
) => {
  // Get the opacity of the window.
  let opacity = win.getOpacity()

  // Increase the opacity of the window by `step` every `fadeEveryXSeconds`
  // seconds.
  const interval = setInterval(() => {
    // Stop fading if window's opacity is 1 or greater.
    if (opacity <= 0) {
      win.hide()
      clearInterval(interval)
    }
    win.setOpacity(opacity)
    opacity -= step
  }, fadeEveryXSeconds)
}

// Main functions

const hideElectronWindow = (hash: string) => {
  const win = getElectronWindow(hash)
  if (win !== undefined) fadeElectronWindowOut(win)
}

const showElectronWindow = (hash: string) => {
  const win = getElectronWindow(hash)
  if (win !== undefined) fadeElectronWindowIn(win)
}

const destroyElectronWindow = (hash: string) => {
  try {
    const win = getElectronWindow(hash)
    win?.destroy()
  } catch (err) {
    console.error(err)
  }
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
  const winAlreadyExists = getElectronWindow(args.hash)
  if (winAlreadyExists !== undefined) {
    fadeElectronWindowIn(winAlreadyExists)
    return { win: winAlreadyExists, view: undefined }
  }

  // Create the BrowserWindow
  const win = new BrowserWindow({
    ...(base as BrowserWindowConstructorOptions),
    ...(args.options !== undefined && args.options),
    width: args.width,
    height: args.height,
    show: false, // If this is not set to false, the window will show too early as a white scren
  })

  // Some websites don't like Electron in the user agent, so we remove it
  win.webContents.userAgent = replaceUserAgent(win.webContents.userAgent)

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

  // When the window is ready to be shown, show it
  win.once(
    "ready-to-show",
    () => (args?.show ?? true) && fadeElectronWindowIn(win)
  )

  // If we want to load a URL into a BrowserWindow, we first load it into a
  // BrowserView and attach it to the BrowserWindow. This is our trick to make the
  // window both frameless and draggable.
  if (args.customURL !== undefined)
    return { win, view: createView(args.customURL, win) }

  return { win, view: undefined }
}

export {
  hideElectronWindow,
  showElectronWindow,
  destroyElectronWindow,
  createElectronWindow,
  isElectronWindowVisible,
}
