// This file manages creation of renderer process windows. It is called from the
// main process, and passes all the configuration needed to load files into
// Electron renderer windows.
import path from "path"
import { app, BrowserWindow, BrowserWindowConstructorOptions } from "electron"
import config from "@app/config/environment"
import { FractalEnvironments } from "../../config/configs"
import { authenticationURL, authInfo, auth0Event } from "@app/utils/auth"
import {
  WindowHashAuth,
  WindowHashSignout,
  WindowHashUpdate,
} from "@app/utils/constants"
import { protocolLaunch } from "@app/utils/protocol"

const { buildRoot } = config

export const base = {
  webPreferences: {
    preload: path.join(buildRoot, "preload.js"),
    partition: "auth0",
  },
  resizable: false,
  titleBarStyle: "default",
  backgroundColor: "#111111",
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
  md: { height: 16 * 44 },
  lg: { height: 16 * 56 },
  xl: { height: 16 * 64 },
  xl2: { height: 16 * 80 },
  xl3: { height: 16 * 96 },
}

export const getWindows = () => BrowserWindow.getAllWindows()

export const showAppDock = () => {
  // On non-macOS systems, app.dock is null, so we
  // do nothing here.
  app?.setActivationPolicy("regular")
  app?.dock?.show().catch((err) => console.error(err))
}

export const hideAppDock = () => {
  // On non-macOS systems, app.dock is null, so we
  // do nothing here.
  app?.setActivationPolicy("accessory")
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
export const createWindow = (args: {
  show: string
  options: Partial<BrowserWindowConstructorOptions>
  customURL?: string
  closeOtherWindows?: boolean
}) => {
  const { title } = config
  const currentOpenWindows = getWindows()
  // We don't want to show a new BrowserWindow right away. We should manually
  // show the window after it has finished loading.
  const win = new BrowserWindow({ ...args.options, show: false, title })

  // Electron doesn't have a API for passing data to renderer windows. We need
  // to pass "init" data for a variety of reasons, but mainly so we can decide
  // which React component to render into the window. We're forced to do this
  // using query parameters in the URL that we pass. The alternative would
  // be to use separate index.html files for each window, which we want to avoid.
  const params = `?show=${args.show}`

  if (app.isPackaged && args.customURL === undefined) {
    win
      .loadFile("build/index.html", { search: params })
      .catch((err) => console.log(err))
  } else {
    win
      .loadURL(
        args.customURL !== undefined
          ? args.customURL
          : `http://localhost:8080${params}`,
        {
          userAgent:
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.212 Safari/537.36",
        }
      )
      .catch((err) => console.error(err))
  }

  // We accept some callbacks in case the caller needs to run some additional
  // functions on open/close.
  // Electron recommends showing the window on the ready-to-show event:
  // https://www.electronjs.org/docs/api/browser-window
  win.once("ready-to-show", () => {
    win.show()
  })

  if (args.closeOtherWindows)
    currentOpenWindows.forEach((win: BrowserWindow) => {
      win.close()
    })

  return win
}

export const createAuthWindow = () => {
  const win = createWindow({
    show: WindowHashAuth,
    options: {
      ...base,
      ...width.xs,
      height: 16 * 37,
    } as BrowserWindowConstructorOptions,
    customURL: authenticationURL,
  })

  // Authentication
  const {
    session: { webRequest },
  } = win.webContents

  const filter = {
    urls: ["http://localhost/callback*"],
  }

  // eslint-disable-next-line @typescript-eslint/no-misused-promises
  webRequest.onBeforeRequest(filter, async ({ url }) => {
    const data = await authInfo(url)
    auth0Event.emit("auth-info", data)
  })

  return win
}

export const createUpdateWindow = () =>
  createWindow({
    show: WindowHashUpdate,
    options: {
      ...base,
      ...width.sm,
      ...height.md,
      skipTaskbar: true,
    } as BrowserWindowConstructorOptions,
    closeOtherWindows: true,
  })

export const createErrorWindow = (hash: string) => {
  createWindow({
    show: hash,
    options: {
      ...base,
      ...width.md,
      ...height.xs,
    } as BrowserWindowConstructorOptions,
    closeOtherWindows: true,
  })
}

export const createSignoutWindow = () => {
  createWindow({
    show: WindowHashSignout,
    options: {
      ...base,
      ...width.md,
      ...height.xs,
    } as BrowserWindowConstructorOptions,
  })
}

export const createProtocolWindow = () => {
  const windows = getWindows()

  protocolLaunch()
    .then(() => {
      windows.forEach((win: BrowserWindow) => {
        win.close()
      })
    })
    .catch((err) => console.error(err))
}
