// This file manages creation of renderer process windows. It is called from the
// main process, and passes all the configuration needed to load files into
// Electron renderer windows.
import path from "path"
import { app, BrowserWindow, BrowserWindowConstructorOptions } from "electron"
import { WindowHashAuth, WindowHashUpdate } from "@app/utils/constants"
import config from "@app/config/environment"
import { FractalEnvironments } from "../../config/configs"
import { FractalError } from "@app/utils/error"

const { buildRoot } = config

export const base = {
  webPreferences: { preload: path.join(buildRoot, "preload.js") },
  resizable: false,
  titleBarStyle: "hidden",
}

export const width = {
  xs: { width: 16 * 24 },
  sm: { width: 16 * 32 },
  md: { width: 16 * 40 },
  lg: { width: 16 * 56 },
  xl: { width: 16 * 64 },
  xl2: { width: 16 * 80 },
  xl3: { width: 16 * 96 },
}

export const height = {
  xs: { height: 16 * 20 },
  sm: { height: 16 * 32 },
  md: { height: 16 * 40 },
  lg: { height: 16 * 56 },
  xl: { height: 16 * 64 },
  xl2: { height: 16 * 80 },
  xl3: { height: 16 * 96 },
}

export const getWindows = () => BrowserWindow.getAllWindows()

export const closeWindows = () => {
  BrowserWindow.getAllWindows().forEach((win) => win.close())
}

export const showAppDock = () => {
  // On non-macOS systems, app.dock is null, so we
  // do nothing here.
  app?.dock?.show().catch((err) => console.error(err))
}

export const hideAppDock = () => {
  // On non-macOS systems, app.dock is null, so we
  // do nothing here.
  app?.dock?.hide()
}

export const getWindowTitle = () => {
  const { deployEnv } = config
  if (deployEnv === FractalEnvironments.PRODUCTION) {
    return "Fractal"
  }
  return `Fractal (${deployEnv})`
}

// This is a "base" window creation function. It won't be called directly from
// the application, instead we'll use it to contain the common functionality
// that we want to share between all windows.
export const createWindow = (
  show: string,
  options: Partial<BrowserWindowConstructorOptions>,
  onReady?: (win: BrowserWindow) => any,
  onClose?: (win: BrowserWindow) => any
) => {
  const { title } = config
  // We don't want to show a new BrowserWindow right away. We should manually
  // show the window after it has finished loading.
  const win = new BrowserWindow({ ...options, show: false, title })

  // Electron doesn't have a API for passing data to renderer windows. We need
  // to pass "init" data for a variety of reasons, but mainly so we can decide
  // which React component to render into the window. We're forced to do this
  // using query parameters in the URL that we pass. The alternative would
  // be to use separate index.html files for each window, which we want to avoid.
  const params = "?show=" + show

  // We need to load index.html differently if we're developing. We want to
  // use Snowpack's development server to manage our reload, so we load from
  // the server's localhost port as opposed to our build folder.
  // Electron's API for passing query parameters is inconsistent, so we must
  // be careful to pass them correctly for each environment.
  if (app.isPackaged) {
    win
      .loadFile("build/index.html", { search: params })
      .catch((err) => console.log(err))
  } else {
    win
      .loadURL("http://localhost:8080" + params)
      .then(() => {
        // We manually open devTools, because we want to make sure that
        // both the main/renderer processes are in a "ready" state before we
        // show.
        win.webContents.openDevTools({ mode: "undocked" })
      })
      .catch((err) => console.error(err))
  }

  // We accept some callbacks in case the caller needs to run some additional
  // functions on open/close.
  win.webContents.on("did-finish-load", () =>
    onReady != null ? onReady(win) : win.show()
  )
  win.on("close", () => onClose?.(win))

  win.show()

  return win
}

export const createAuthWindow = () =>
  createWindow(WindowHashAuth, {
    ...base,
    ...width.sm,
    ...height.md,
  } as BrowserWindowConstructorOptions)

export const createAuthErrorWindow: CreateWindowFunction = () =>
  createWindow(WindowHashAuthError, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)

export const createContainerErrorWindowNoAccess: CreateWindowFunction = () =>
  createWindow(WindowHashCreateContainerErrorNoAccess, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)

export const createContainerErrorWindowUnauthorized: CreateWindowFunction =
  () =>
    createWindow(WindowHashCreateContainerErrorUnauthorized, {
      ...base,
      ...width.md,
      ...height.xs,
    } as BrowserWindowConstructorOptions)

export const createContainerErrorWindowInternal: CreateWindowFunction = () =>
  createWindow(WindowHashCreateContainerErrorInternal, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)

export const assignContainerErrorWindow: CreateWindowFunction = () =>
  createWindow(WindowHashAssignContainerError, {
    ...base,
    ...width.sm,
    ...height.md,
  } as BrowserWindowConstructorOptions)

export const createErrorWindow = (error: FractalError) => {
  createWindow(error.hash, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)
}
