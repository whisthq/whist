import path from "path"
import { app, BrowserWindow, BrowserWindowConstructorOptions } from "electron"
import {
  WindowHashAuth,
  WindowHashUpdate,
  WindowHashProtocolError,
  WindowHashCreateContainerErrorNoAccess,
  WindowHashCreateContainerErrorUnauthorized,
  WindowHashCreateContainerErrorInternal,
  WindowHashAssignContainerError,
} from "@app/utils/constants"
import config, { FractalCIEnvironment } from "@app/config/environment"
import { getAuthenticationURL, loadTokens } from "@app/utils/auth"

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

type CreateWindowFunction = (
  onReady?: (win: BrowserWindow) => any,
  onClose?: (win: BrowserWindow) => any
) => BrowserWindow

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
  customUrl?: string,
  onReady?: (win: BrowserWindow) => any,
  onClose?: (win: BrowserWindow) => any
) => {
  const { title } = config
  const win = new BrowserWindow({ ...options, show: false, title })

  const params = "?show=" + show

  if (app.isPackaged && customUrl === undefined) {
    win
      .loadFile("build/index.html", { search: params })
      .catch((err) => console.log(err))
  } else {
    win
      .loadURL(
        customUrl !== undefined ? customUrl : "http://localhost:8080" + params
      )
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

export const createAuthWindow: CreateWindowFunction = () => {
  const win = createWindow(
    WindowHashAuth,
    {
      ...base,
      ...width.md,
      ...height.lg,
    } as BrowserWindowConstructorOptions,
    getAuthenticationURL()
  )

  // Authentication
  const {
    session: { webRequest },
  } = win.webContents

  const filter = {
    urls: ["http://localhost/callback*"],
  }

  // eslint-disable-next-line @typescript-eslint/no-misused-promises
  webRequest.onBeforeRequest(filter, async ({ url }) => {
    await loadTokens(url)
  })

  return win
}

export const createContainerErrorWindowNoAccess: CreateWindowFunction = () =>
  createWindow(WindowHashCreateContainerErrorNoAccess, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)

export const createContainerErrorWindowUnauthorized: CreateWindowFunction = () =>
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
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)

export const createProtocolErrorWindow: CreateWindowFunction = () =>
  createWindow(WindowHashProtocolError, {
    ...base,
    ...width.md,
    ...height.xs,
  } as BrowserWindowConstructorOptions)

export const createUpdateWindow: CreateWindowFunction = () =>
  createWindow(WindowHashUpdate, {
    ...base,
    ...width.sm,
    ...height.md,
  } as BrowserWindowConstructorOptions)
