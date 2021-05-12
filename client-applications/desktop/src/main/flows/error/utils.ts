import path from "path"
import { app, BrowserWindow, BrowserWindowConstructorOptions } from "electron"
import config, { FractalCIEnvironment } from "@app/config/environment"

export type FractalError = {
  hash: string,
  title: string,
  message: string
}

export const NoAccessError: FractalError = {
  hash: "CONTAINER_ERROR_NO_ACCESS",
  title: "Your account does not have access to Fractal.",
  message: "Access to Fractal is currently invite-only. Please contact support@fractal.co for help.",
}

export const UnauthorizedError: FractalError = {
  hash: "CONTAINER_ERROR_UNAUTHORIZED",
  title: "There was an error authenticating you with Fractal.",
  message: "Please try logging in again or contact support@fractal.co for help.",
}

export const ProtocolError: FractalError = {
  hash: "PROTOCOL_ERROR",
  title: "The Fractal browser encountered an unexpected error.",
  message:
    "Please try again in a few minutes or contact support@fractal.co for help.",
}

export const ContainerError: FractalError = {
  hash: "CONTAINER_ERROR",
  title: "There was an error connecting to the Fractal servers.",
  message:
    "Please try again in a few minutes or contact support@fractal.co for help.",
}

export const AuthError: FractalError = {
  hash: "AUTH_ERROR",
  title: "There was an error logging you in",
  message:
    "Please try logging in again or contact support@fractal.co for help.",
}

export const NavigationError: FractalError = {
  hash: "NAVIGATION_ERROR",
  title: "There was an error loading this window.",
  message: "Please restart Fractal or contact support@fractal.co for help.",
}



export const WindowHashAuth = "AUTH"

export const WindowHashUpdate = "UPDATE"


const { buildRoot } = config

const base = {
  webPreferences: { preload: path.join(buildRoot, "preload.js") },
  resizable: false,
  titleBarStyle: "hidden",
}

const width = {
  xs: { width: 16 * 24 },
  sm: { width: 16 * 32 },
  md: { width: 16 * 40 },
  lg: { width: 16 * 56 },
  xl: { width: 16 * 64 },
  xl2: { width: 16 * 80 },
  xl3: { width: 16 * 96 },
}

const height = {
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

export const createErrorWindow = (error: FractalError) =>
  createWindow(error.hash, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)
