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

let updating = false
let mainWindow: BrowserWindow | null = null
var customURL = null

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

const debugLog = (callback: any) => {
    debugLog(process.env.NODE_ENV)
    if (process.env.NODE_ENV === "development") {
        debugLog(callback)
    }
}

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
        if (os.platform() === "win32") {
            // Keep only command line / deep linked arguments

            const url = process.argv.slice(1)
            mainWindow.webContents.send("customURL", url.toString())
        } else {
            if (customURL) {
                mainWindow.webContents.send("customURL", customURL)
            }
        }
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

    mainWindow.on("closed", () => {
        mainWindow = null
    })

    if (process.env.NODE_ENV === "development") {
        // Skip autoupdate check
    } else {
        autoUpdater.checkForUpdates()
    }
}

/**
 * Add event listeners...
 */

app.on("window-all-closed", () => {
    // Respect the OSX convention of having the application in memory even
    // after all windows have been closed
    if (process.platform !== "darwin") {
        app.quit()
    }
})

const gotTheLock = app.requestSingleInstanceLock()

if (!gotTheLock) {
    app.quit()
} else {
    app.on("second-instance", () => {
        // Someone tried to run a second instance, we should focus our window.
        if (mainWindow) {
            if (mainWindow.isMinimized()) mainWindow.restore()
            mainWindow.focus()
        }
    })
    app.on("ready", createWindow)

    app.on("activate", () => {
        // On macOS it's common to re-create a window in the app when the
        // dock icon is clicked and there are no other windows open.
        if (mainWindow === null) createWindow()
    })
}

app.setAsDefaultProtocolClient("fractal")

// Set up launch from browser with fractal:// protocol
app.on("open-url", function (event, data) {
    event.preventDefault()
    customURL = data.toString()
})

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
