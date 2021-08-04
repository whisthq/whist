/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file contains utility functions to create Electron windows. This file manages
 * creation of renderer process windows. It is called from the main process, and passes all
 * the configuration needed to load files into Electron renderer windows.
 */

import path from "path"
import events from "events"
import { app, BrowserWindow, BrowserWindowConstructorOptions } from "electron"
import config from "@app/config/environment"
import { FractalEnvironments } from "../../config/configs"
import { FractalCallbackUrls } from "@app/config/urls"
import {
  authPortalURL,
  authInfoParse,
  authInfoCallbackRequest,
  paymentPortalRequest,
  paymentPortalParse,
} from "@fractal/core-ts"
import {
  WindowHashAuth,
  WindowHashSignout,
  WindowHashUpdate,
  WindowHashPayment,
  WindowHashTypeform,
  WindowHashProtocol,
} from "@app/utils/constants"
import {
  protocolLaunch,
  childProcess,
  protocolStreamKill,
} from "@app/utils/protocol"

// Custom Event Emitter for Auth0 events
export const auth0Event = new events.EventEmitter()

const { buildRoot } = config

// Because the protocol window is a window that's not recognized by Electron's BrowserWindow
// module, we create our own event emitter to handle things like keeping track of how many
// windows we have open. This is used in effects/app.ts to decide when to close the application.
export const windowMonitor = new events.EventEmitter()

const emitWindowInfo = (args: {
  crashed: boolean
  event: string
  hash: string
}) => {
  windowMonitor.emit("window-info", {
    numberWindowsRemaining: getNumberWindows(),
    crashed: args.crashed,
    event: args.event,
    hash: args.hash,
  })
}

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

export const getElectronWindows = () => BrowserWindow.getAllWindows()

export const getNumberWindows = () => {
  const numElectronWindows = getElectronWindows().length
  const numProtocolWindows = childProcess === undefined ? 0 : 1
  return numElectronWindows + numProtocolWindows
}

export const closeElectronWindows = (windows?: BrowserWindow[]) => {
  const windowsToClose = windows ?? getElectronWindows()
  windowsToClose.forEach((win: BrowserWindow) => {
    win.close()
  })
}

export const closeAllWindows = (windows?: BrowserWindow[]) => {
  closeElectronWindows(windows)
  protocolStreamKill()
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
  hash: string
  customURL?: string
  closeOtherWindows?: boolean
}) => {
  const { title } = config
  const currentElectronWindows = getElectronWindows()
  // We don't want to show a new BrowserWindow right away. We should manually
  // show the window after it has finished loading.
  const win = new BrowserWindow({ ...args.options, show: false, title })

  let ua = win.webContents.userAgent
  ua = ua.replace(/Electron\/*/, "")
  ua = ua.replace(/Chrome\/*/, "")
  win.webContents.userAgent = ua

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
          : `http://localhost:8080${params}`
      )
      .catch((err) => console.error(err))
  }

  // We accept some callbacks in case the caller needs to run some additional
  // functions on open/close.
  // Electron recommends showing the window on the ready-to-show event:
  // https://www.electronjs.org/docs/api/browser-window
  win.once("ready-to-show", () => {
    win.show()
    emitWindowInfo({ crashed: false, event: "open", hash: args.hash })
  })

  win.on("closed", () => {
    emitWindowInfo({ crashed: false, event: "close", hash: args.hash })
  })

  if (args.closeOtherWindows ?? false) closeAllWindows(currentElectronWindows)

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
    hash: WindowHashAuth,
    customURL: authPortalURL(),
    closeOtherWindows: true,
  })

  // Authentication
  const {
    session: { webRequest },
  } = win.webContents

  const filter = {
    urls: [FractalCallbackUrls.authCallBack],
  }

  // eslint-disable-next-line @typescript-eslint/no-misused-promises
  webRequest.onBeforeRequest(filter, async ({ url }) => {
    const response = await authInfoCallbackRequest({ authCallbackURL: url })
    auth0Event.emit("auth-info", {
      ...authInfoParse(response),
      refreshToken: response?.json?.refresh_token,
    })
    return response
  })

  return win
}

export const createPaymentWindow = async ({
  accessToken,
}: {
  accessToken: string
}) => {
  const response = await paymentPortalRequest({ accessToken })
  const { paymentPortalURL } = paymentPortalParse(response)
  const win = createWindow({
    show: WindowHashPayment,
    options: {
      ...base,
      ...width.lg,
      ...height.md,
      alwaysOnTop: true,
    } as BrowserWindowConstructorOptions,
    hash: WindowHashPayment,
    customURL: paymentPortalURL,
  })

  const {
    session: { webRequest },
  } = win.webContents

  const filter = {
    urls: [FractalCallbackUrls.paymentCallBack],
  }
  // eslint-disable-next-line @typescript-eslint/no-misused-promises
  webRequest.onBeforeRequest(filter, async ({ url }) => {
    win.close()
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
    hash: WindowHashUpdate,
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
    hash: hash,
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
      alwaysOnTop: true,
    } as BrowserWindowConstructorOptions,
    hash: WindowHashSignout,
  })
}

export const createTypeformWindow = () =>
  createWindow({
    show: WindowHashTypeform,
    options: {
      ...base,
      ...width.lg,
      ...height.md,
      skipTaskbar: true,
      alwaysOnTop: true,
    } as BrowserWindowConstructorOptions,
    hash: WindowHashTypeform,
    closeOtherWindows: false,
    customURL: "https://form.typeform.com/to/Yfs4GkeN",
  })

export const createProtocolWindow = async () => {
  const currentElectronWindows = getElectronWindows()

  const protocol = await protocolLaunch()

  protocol.on("spawn", () => {
    emitWindowInfo({
      crashed: false,
      event: "open",
      hash: WindowHashProtocol,
    })
  })

  protocol.on("close", (code: number) => {
    // Javascript's EventEmitter is synchronous, so we emit the number of windows and
    // crash status in a single event to so that the listener can consume both pieces of
    // information simultaneously
    emitWindowInfo({
      crashed: (code ?? 0) === 1,
      event: "close",
      hash: WindowHashProtocol,
    })
  })

  // When the protocol is running, we want all other Electron windows to be closed
  closeElectronWindows(currentElectronWindows)
}

export const relaunch = (options?: { args: string[] }) => {
  protocolStreamKill()
  options === undefined ? app.relaunch() : app.relaunch(options)
  app.exit()
}
