import path from "path"
import { app, BrowserWindow, BrowserWindowConstructorOptions } from "electron"
import { WindowHashAuth, WindowHashUpdate } from "@app/utils/constants"
import config, { FractalCIEnvironment } from "@app/config/environment"

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
  if (deployEnv === FractalCIEnvironment.PRODUCTION) {
    return "Fractal"
  }
  return `Fractal (${deployEnv})`
}

export const createWindow = (
  show: string,
  options: Partial<BrowserWindowConstructorOptions>,
  onReady?: (win: BrowserWindow) => any,
  onClose?: (win: BrowserWindow) => any
) => {
  const { title } = config
  const win = new BrowserWindow({ ...options, show: false, title })

  const params = "?show=" + show

  if (app.isPackaged) {
    win
      .loadFile("build/index.html", { search: params })
      .catch((err) => console.log(err))
  } else {
    win
      .loadURL("http://localhost:8080" + params)
      .then(() => {
        win.webContents.openDevTools({ mode: "undocked" })
      })
      .catch((err) => console.error(err))
  }

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

export const createUpdateWindow = () =>
  createWindow(WindowHashUpdate, {
    ...base,
    ...width.sm,
    ...height.md,
  } as BrowserWindowConstructorOptions)
