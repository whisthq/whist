/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file auth.ts
 * @brief This file contains utility functions to create Electron windows. This file manages
 * creation of renderer process windows. It is called from the main process, and passes all
 * the configuration needed to load files into Electron renderer windows.
 */

import path from "path"
import events from "events"
import {
  app,
  BrowserWindow,
  BrowserWindowConstructorOptions,
  BrowserView,
} from "electron"
import config from "@app/config/environment"
import { WhistEnvironments } from "../../../config/configs"
import { WhistCallbackUrls } from "@app/config/urls"
import {
  authPortalURL,
  authInfoParse,
  authInfoCallbackRequest,
  paymentPortalRequest,
  paymentPortalParse,
  accessToken,
} from "@whist/core-ts"
import {
  WindowHashAuth,
  WindowHashSignout,
  WindowHashPayment,
  WindowHashProtocol,
  WindowHashOnboarding,
  WindowHashBugTypeform,
  WindowHashUpdate,
  WindowHashLoading,
  WindowHashOmnibar,
  WindowHashImport,
} from "@app/constants/windows"
import {
  protocolLaunch,
  protocolStreamKill,
  isNetworkUnstable,
} from "@app/main/utils/protocol"

// Custom Event Emitter for Auth0 events
export const auth0Event = new events.EventEmitter()
export const stripeEvent = new events.EventEmitter()

const { buildRoot } = config

// Because the protocol window is a window that's not recognized by Electron's BrowserWindow
// module, we create our own event emitter to handle things like keeping track of how many
// windows we have open. This is used in effects/app.ts to decide when to close the application.
export const windowMonitor = new events.EventEmitter()

// Because we launch the protocol at start but keep it hidden, we need to keep track of if it's
// running
let protocolRunning = false

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
  xxs: { height: 16 * 4 },
  xs: { height: 16 * 20 },
  sm: { height: 16 * 28 },
  md: { height: 16 * 44 },
  lg: { height: 16 * 56 },
  xl: { height: 16 * 64 },
  xl2: { height: 16 * 80 },
  xl3: { height: 16 * 96 },
}

export const getElectronWindows = () => BrowserWindow.getAllWindows()

export const getNumberWindows = () => {
  const numElectronWindows = getElectronWindows().length
  const numProtocolWindows = protocolRunning ? 1 : 0
  return numElectronWindows + numProtocolWindows
}

export const closeElectronWindows = (windows?: BrowserWindow[]) => {
  const windowsToClose = windows ?? getElectronWindows()

  windowsToClose.forEach((win: BrowserWindow) => {
    try {
      win.destroy()
    } catch (err) {
      console.error(err)
    }
  })
}

export const closeAllWindows = (windows?: BrowserWindow[]) => {
  closeElectronWindows(windows)
  protocolStreamKill()
}

export const getWindowTitle = () => {
  const { deployEnv } = config
  if (deployEnv === WhistEnvironments.PRODUCTION) {
    return "Whist"
  }
  return `Whist (${deployEnv})`
}

export const getWindowByHash = (hash: string) => {
  for (const win of getElectronWindows()) {
    if (win.webContents.getURL()?.split("show=")?.[1] === hash) return win
  }
  return undefined
}

// This is a "base" window creation function. It won't be called directly from
// the application, instead we'll use it to contain the common functionality
// that we want to share between all windows.
export const createWindow = (args: {
  options: Partial<BrowserWindowConstructorOptions>
  hash: string
  customURL?: string
  closeElectronWindows?: boolean
  closeProtocolWindow?: boolean
  focus?: boolean
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
  const params = `?show=${args.hash}`

  if (app.isPackaged && args.customURL === undefined) {
    win
      .loadFile("build/index.html", { search: params })
      .catch((err) => console.error(err))
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
    args.focus ?? true ? win.show() : win.hide()
    emitWindowInfo({ crashed: false, event: "open", hash: args.hash })
  })

  win.on("closed", () => {
    emitWindowInfo({ crashed: false, event: "close", hash: args.hash })
  })

  if (args.closeElectronWindows ?? false)
    closeElectronWindows(currentElectronWindows)

  if (args.closeProtocolWindow ?? false) protocolStreamKill()

  return win
}

export const createAuthWindow = () => {
  const padding = 60
  const auth0Height = height.sm.height + 80
  const windowWidth = Math.round(width.xs.width + padding)
  const windowHeight = Math.round(auth0Height + padding)
  const win = createWindow({
    options: {
      ...base,
      width: windowWidth,
      height: windowHeight,
      frame: false,
      border: false,
      titleBarStyle: "hidden",
    } as BrowserWindowConstructorOptions,
    hash: WindowHashAuth,
    closeElectronWindows: true,
  })

  const view = new BrowserView()

  win.setBrowserView(view)
  view.setBounds({
    x: windowWidth / 2 - width.xs.width / 2,
    y: windowHeight / 2 - auth0Height / 2,
    ...width.xs,
    height: auth0Height,
  })
  let ua = view.webContents.userAgent
  ua = ua.replace(/Electron\/*/, "")
  ua = ua.replace(/Chrome\/*/, "")
  view.webContents.userAgent = ua

  view.webContents.loadURL(authPortalURL()).catch((err) => console.error(err))

  // Authentication
  const {
    session: { webRequest },
  } = view.webContents

  const filter = {
    urls: [WhistCallbackUrls.authCallBack],
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

export const createPaymentWindow = async (accessToken: accessToken) => {
  const padding = 50
  const paymentHeight = height.md.height + 20
  const windowWidth = Math.round(width.lg.width + padding)
  const windowHeight = Math.round(paymentHeight + padding)

  const response = await paymentPortalRequest(accessToken)
  const { paymentPortalURL } = paymentPortalParse(response)

  const win = createWindow({
    options: {
      ...base,
      width: windowWidth,
      height: windowHeight,
      frame: false,
      border: false,
      titleBarStyle: "hidden",
    } as BrowserWindowConstructorOptions,
    hash: WindowHashPayment,
    closeElectronWindows: true,
  })

  const view = new BrowserView()

  win.setBrowserView(view)
  view.setBounds({
    x: windowWidth / 2 - width.lg.width / 2,
    y: windowHeight / 2 - paymentHeight / 2,
    ...width.lg,
    height: paymentHeight,
  })

  view.webContents
    .loadURL(paymentPortalURL ?? "")
    .catch((err) => console.error(err))

  const {
    session: { webRequest },
  } = view.webContents

  const filter = {
    urls: [WhistCallbackUrls.paymentCallBack],
  }
  // eslint-disable-next-line @typescript-eslint/no-misused-promises
  webRequest.onBeforeRequest(filter, async ({ url }) => {
    if (
      url === "http://localhost/callback/payment" ||
      url === "http://localhost/callback/payment?success=true"
    ) {
      // if it’s from the customer portal or a successful checkout
      stripeEvent.emit("stripe-auth-refresh")
    } else if (url === "http://localhost/callback/payment?success=false") {
      stripeEvent.emit("stripe-payment-error")
    }
  })
}

export const createErrorWindow = (
  hash: string,
  closeOtherWindows?: boolean
) => {
  createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.xs,
      frame: false,
      titleBarStyle: "hidden",
      transparent: true,
      alwaysOnTop: true,
    } as BrowserWindowConstructorOptions,
    hash: hash,
    closeElectronWindows: closeOtherWindows ?? true,
    closeProtocolWindow: closeOtherWindows ?? true,
  })
}

export const createSignoutWindow = () => {
  createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.xs,
      frame: false,
      titleBarStyle: "hidden",
      transparent: true,
    } as BrowserWindowConstructorOptions,
    hash: WindowHashSignout,
  })
}

export const createOnboardingWindow = () =>
  createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.sm,
      skipTaskbar: true,
      frame: false,
      titleBarStyle: "hidden",
    } as BrowserWindowConstructorOptions,
    hash: WindowHashOnboarding,
    closeElectronWindows: true,
  })

export const createBugTypeform = () =>
  createWindow({
    options: {
      ...base,
      ...width.lg,
      ...height.md,
      skipTaskbar: true,
      minimizable: false,
      frame: false,
      titleBarStyle: "hidden",
      backgroundColor: "#182129",
    } as BrowserWindowConstructorOptions,
    hash: WindowHashBugTypeform,
    closeElectronWindows: false,
  })

export const createSpeedtestWindow = () =>
  createWindow({
    options: {
      ...base,
      ...width.lg,
      ...height.md,
      skipTaskbar: true,
      minimizable: false,
      frame: false,
      titleBarStyle: "hidden",
    } as BrowserWindowConstructorOptions,
    hash: "",
    closeElectronWindows: false,
    customURL: "https://speed.cloudflare.com/",
  })

export const createProtocolWindow = async () => {
  const protocol = await protocolLaunch()

  protocol.on("close", (code: number) => {
    // Javascript's EventEmitter is synchronous, so we emit the number of windows and
    // crash status in a single event to so that the listener can consume both pieces of
    // information simultaneously
    protocolRunning = false
    emitWindowInfo({
      crashed: (code ?? 0) === 1,
      event: "close",
      hash: WindowHashProtocol,
    })
  })

  protocol?.stdout?.on("data", (message: string) => {
    const unstable = isNetworkUnstable(message)
    windowMonitor.emit("network-is-unstable", unstable)

    if (message.includes("Connected on")) {
      protocolRunning = true
      emitWindowInfo({
        crashed: false,
        event: "open",
        hash: WindowHashProtocol,
      })

      const windowsToClose = getElectronWindows().filter(
        (win) =>
          ![WindowHashOmnibar].includes(
            win.webContents.getURL()?.split("show=")?.[1]
          )
      )

      closeElectronWindows(windowsToClose)

      app?.dock?.hide()
    }
  })
}

export const relaunch = (options?: { args: string[] }) => {
  protocolStreamKill()
  options === undefined ? app.relaunch() : app.relaunch(options)
  app.exit()
}

export const createUpdateWindow = () =>
  createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.xs,
      frame: false,
      titleBarStyle: "hidden",
      transparent: true,
    } as BrowserWindowConstructorOptions,
    hash: WindowHashUpdate,
    closeElectronWindows: true,
    closeProtocolWindow: true,
  })

export const createLoadingWindow = () =>
  createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.xs,
      frame: false,
      titleBarStyle: "hidden",
      transparent: true,
    } as BrowserWindowConstructorOptions,
    hash: WindowHashLoading,
  })

export const createLicenseWindow = (url: string) =>
  createWindow({
    options: {
      ...base,
      ...width.lg,
      ...height.md,
      skipTaskbar: true,
      minimizable: false,
    } as BrowserWindowConstructorOptions,
    hash: "",
    closeElectronWindows: false,
    customURL: url,
  })

export const createOmnibar = () => {
  const win = createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.xs,
      frame: false,
      transparent: true,
      alwaysOnTop: true,
      resizable: false,
      fullscreenable: false,
      minimizable: false,
      titleBarStyle: "hidden",
      closable: false,
    },
    hash: WindowHashOmnibar,
  })

  win.focus()
  return win
}

export const destroyOmnibar = () => {
  const win = getWindowByHash(WindowHashOmnibar)

  win?.destroy()
}

export const createImportWindow = () =>
  createWindow({
    options: {
      ...base,
      ...width.md,
      ...height.sm,
      skipTaskbar: true,
      frame: false,
      titleBarStyle: "hidden",
    } as BrowserWindowConstructorOptions,
    hash: WindowHashImport,
  })
