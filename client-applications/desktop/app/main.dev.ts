/* eslint global-require: off, no-console: off */

/**
 * This module executes inside of electron's main process. You can start
 * electron renderer process from here and communicate with the other processes
 * through IPC.
 *
 * When running `yarn build` or `yarn build-main`, this file is compiled to
 * `./app/main.prod.js` using webpack. This gives us some performance wins.
 */

import path from "path"
import { app, BrowserWindow } from "electron"
import { autoUpdater } from "electron-updater"

// This is the window where the renderer thread will render our React app
let mainWindow: BrowserWindow | null = null
// Detects whether there's an auto-update
let updating = false
// Detects whether fractal:// has been typed into a browser
let customURL = null
// Toggles whether the desktop app is allowed to quit (to prevent concurrent apps)
let canClose = true

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = "true"
process.env.GOOGLE_API_KEY = "AIzaSyA2FUwAOXKqIWMqKN5DNPBUaqYMOWdBADQ"

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

// Custom logging to hide console logs in prod
const debugLog = (callback: any) => {
    debugLog(process.env.NODE_ENV)
    if (process.env.NODE_ENV === "development") {
        debugLog(callback)
    }
}

// Function to create the browser window
const createWindow = async () => {
    const os = require("os")
    if (os.platform() === "win32") {
        mainWindow = new BrowserWindow({
            show: false,
            width: 1000,
            height: 680,
            frame: false,
            center: true,
            resizable: false,
            webPreferences: {
                nodeIntegration: true,
            },
        })
    } else if (os.platform() === "darwin") {
        mainWindow = new BrowserWindow({
            show: false,
            width: 1000,
            height: 680,
            titleBarStyle: "hidden",
            center: true,
            resizable: false,
            maximizable: false,
            webPreferences: {
                nodeIntegration: true,
            },
        })
    } else {
        // if (os.platform() === "linux") case
        mainWindow = new BrowserWindow({
            show: false,
            width: 1000,
            height: 680,
            titleBarStyle: "hidden",
            center: true,
            resizable: false,
            maximizable: false,
            webPreferences: {
                nodeIntegration: true,
            },
            icon: path.join(__dirname, "/build/icon.png"),
        })
    }
    mainWindow.loadURL(`file://${__dirname}/app.html`)
    // mainWindow.webContents.openDevTools()

    // @TODO: Use 'ready-to-show' event
    //        https://github.com/electron/electron/blob/master/docs/api/browser-window.md#using-ready-to-show-event
    mainWindow.webContents.on("did-frame-finish-load", () => {
        // Checks if fractal:// was typed in, if it was, uses IPC to forward the URL
        // to the renderer thread
        if (os.platform() === "win32") {
            // Keep only command line / deep linked arguments

            const url = process.argv.slice(1)
            mainWindow.webContents.send("customURL", url.toString())
        } else if (customURL) {
            mainWindow.webContents.send("customURL", customURL)
        }
        // Open dev tools in development
        if (
            process.env.NODE_ENV === "development" ||
            process.env.DEBUG_PROD === "true"
        ) {
            mainWindow.webContents.openDevTools()
        }
        if (!mainWindow) {
            throw new Error('"mainWindow" is not defined')
        }
        if (process.env.START_MINIMIZED) {
            mainWindow.minimize()
        } else {
            mainWindow.show()
            mainWindow.focus()
            mainWindow.webContents.send("update", updating)
        }
    })

    // Listener to detect if the protocol was launched to hide the
    // app from the task tray
    const electron = require("electron")
    const ipc = electron.ipcMain

    ipc.on("canClose", (event, argv) => {
        canClose = argv
        if (canClose && mainWindow) {
            mainWindow.show()
            mainWindow.focus()
            if (app && app.dock) {
                app.dock.show()
            }
        } else if (!canClose && mainWindow) {
            mainWindow.hide()
            if (app && app.dock) {
                app.dock.hide()
            }
        }
        event.returnValue = argv
    })

    mainWindow.on("close", (event) => {
        if (!canClose) {
            event.preventDefault()
        }
    })

    mainWindow.on("closed", () => {
        mainWindow = null
    })

    if (process.env.NODE_ENV === "development") {
        // Skip autoupdate check
    } else {
        autoUpdater.checkForUpdates()
    }
}

// Calls the create window above, conditional on the app not already running (single instance lock)

const gotTheLock = app.requestSingleInstanceLock()

if (!gotTheLock) {
    app.quit()
} else {
    app.on("second-instance", (_, argv) => {
        // Someone tried to run a second instance, we should focus our window.
        if (mainWindow) {
            if (mainWindow.isMinimized()) mainWindow.restore()
            mainWindow.focus()
            const url = argv.slice(1)
            mainWindow.webContents.send("customURL", url.toString())
        }
    })
    app.on("ready", createWindow)

    app.on("activate", () => {
        // On macOS it's common to re-create a window in the app when the
        // dock icon is clicked and there are no other windows open.
        if (mainWindow === null) createWindow()
    })
}

// Additional listeners for app launch, close, etc.

app.on("window-all-closed", () => {
    // Respect the OSX convention of having the application in memory even
    // after all windows have been closed
    if (process.platform !== "darwin") {
        app.quit()
    }
})

app.setAsDefaultProtocolClient("fractal")

// Set up launch from browser with fractal:// protocol for Mac
app.on("open-url", function (event, data) {
    event.preventDefault()
    customURL = data.toString()
    if (mainWindow && mainWindow.webContents) {
        mainWindow.webContents.send("customURL", customURL)
    }
})

// Autoupdater listeners, will fire if S3 app version is greater than current version

autoUpdater.autoDownload = false

autoUpdater.on("update-available", () => {
    updating = true
    mainWindow.webContents.send("update", updating)
    autoUpdater.downloadUpdate()
})

autoUpdater.on("update-not-available", () => {
    updating = false
    mainWindow.webContents.send("update", updating)
})

autoUpdater.on("error", (_ev, err) => {
    updating = false
    mainWindow.webContents.send("error", err)
})

autoUpdater.on("download-progress", (progressObj) => {
    mainWindow.webContents.send("download-speed", progressObj.bytesPerSecond)
    mainWindow.webContents.send("percent", progressObj.percent)
    mainWindow.webContents.send("transferred", progressObj.transferred)
    mainWindow.webContents.send("total", progressObj.total)
})

autoUpdater.on("update-downloaded", () => {
    mainWindow.webContents.send("downloaded", true)
    autoUpdater.quitAndInstall()
    updating = false
})
