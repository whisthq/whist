/* eslint global-require: off, no-console: off */

/**
 * This module executes inside of electron's main process. You can start
 * electron renderer process from here and communicate with the other processes
 * through IPC.
 *
 * When running `yarn build` or `yarn build-main`, this file is compiled to
 * `./app/main.prod.js` using webpack. This gives us some performance wins.
 */

import { app, BrowserWindow } from "electron"
import * as Sentry from "@sentry/electron"
import Store from "electron-store"
import { FractalIPC } from "./shared/types/ipc"
import { createWindow, initiateWindowListeners } from "./main/launchWindow"
import { initiateFractalIPCListeners } from "./main/initiateFractalIPCListeners"
import { initiateAutoUpdateListeners } from "./main/initiateAutoUpdateListeners"

if (process.env.NODE_ENV === "production") {
    Sentry.init({
        dsn:
            "https://5b0accb25f3341d280bb76f08775efe1@o400459.ingest.sentry.io/5412323",
        release: `client-applications@${app.getVersion()}`,
    })
}

// Initialize electron-store https://github.com/sindresorhus/electron-store#initrenderer
Store.initRenderer()

// This is the window where the renderer thread will render our React app
let mainWindow: BrowserWindow | null = null
// Detects whether there's an auto-update
// eslint-disable-next-line @typescript-eslint/no-namespace
declare global {
    namespace NodeJS {
        interface Global {
            updateStatus: boolean
        }
    }
}
global.updateStatus = false
// Detects whether fractal:// has been typed into a browser
let customURL: string | null = null
// Toggles whether to show the Electron main window
const showMainWindow = false

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = "true"

if (process.env.NODE_ENV === "production") {
    const sourceMapSupport = require("source-map-support")
    sourceMapSupport.install()
}

if (
    process.env.NODE_ENV === "development" ||
    process.env.DEBUG_PROD === "true"
) {
    require("electron-debug")()
}

// add this handler before emitting any events
process.on("uncaughtException", (err) => {
    console.log("UNCAUGHT EXCEPTION - keeping process alive:", err) // err.message is "foobar"
})

/**
 * Launches window and protocol through ipc listeners
 * @param mainWindow mainWindow
 * @param customURL
 * @param showMainWindow
 */
const launchWindow = async () => {
    mainWindow = await createWindow(mainWindow)

    // this needs to be in main.dev.ts, having the file elswhere causes bugs when loading app.html
    setTimeout(() => {
        mainWindow?.loadURL(`file://${__dirname}/app.html`)
    }, 2000)

    // mainWindow.webContents.openDevTools()
    initiateAutoUpdateListeners(mainWindow)
    initiateWindowListeners(mainWindow, customURL, showMainWindow)
    initiateFractalIPCListeners(mainWindow, showMainWindow)
}

// Calls the create window above, conditional on the app not already running (single instance lock)

const gotTheLock = app.requestSingleInstanceLock()
if (!gotTheLock) {
    app.quit()
} else {
    app.on("second-instance", (_, argv) => {
        // Someone tried to run a second instance, we should focus our window.
        if (mainWindow && showMainWindow) {
            if (mainWindow.isMinimized()) mainWindow.restore()
            mainWindow.maximize()
            mainWindow.show()
            mainWindow.focus()
            const url = argv.slice(1)
            mainWindow.webContents.send(FractalIPC.CUSTOM_URL, url.toString())
        }
    })

    app.allowRendererProcessReuse = true

    app.on("ready", async () => {
        await launchWindow()
    })

    app.on("activate", async () => {
        // On macOS it's common to re-create a window in the app when the
        // dock icon is clicked and there are no other windows open.
        if (mainWindow === null) await launchWindow()
    })
}

// Additional listeners for app launch, close, etc.

app.setAsDefaultProtocolClient("fractal")

// Set up launch from browser with fractal:// protocol for Mac
app.on("open-url", (event, data) => {
    event.preventDefault()
    customURL = data.toString()
    if (mainWindow && mainWindow.webContents) {
        mainWindow.webContents.send(FractalIPC.CUSTOM_URL, customURL)
        if (showMainWindow) {
            mainWindow.show()
            mainWindow.focus()
            mainWindow.restore()
        }
    }
})
