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
import { createConnection, createServer, Server, Socket } from "net"
import * as Sentry from "@sentry/electron"
import Store from "electron-store"
import { FractalIPC } from "./shared/types/ipc"

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
let updating = false
// Detects whether fractal:// has been typed into a browser
let customURL: string | null = null
// Toggles whether to show the Electron main window
let showMainWindow = false
// Toggles whether FractalClient should be minimized or
//    restored (maximized). When the showMainWindow is false,
//    this value gets toggled on "minimize" and "focus"
//    events
let protocolMinimized = false

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

// Create IPC socket to communicate with protocol
let socketPath = "/tmp/fractal-client.sock";

let protocolSocket: Server = createServer((c) => {
    console.log('client connected')
    c.on('end', () => {
        console.log('client disconnected')
    })
    c.write('hello from client app')
    c.pipe(c)
})
protocolSocket.on('error', (err) => {
    console.log(`error ${err}`)
})
protocolSocket.listen(socketPath, () => {
    console.log('server bound')
})

// Function to create the browser window
const createWindow = async () => {
    const os = require("os")
    if (os.platform() === "win32") {
        mainWindow = new BrowserWindow({
            show: false,
            frame: false,
            center: true,
            resizable: true,
            webPreferences: {
                nodeIntegration: true,
                enableRemoteModule: true,
                contextIsolation: false,
            },
        })
    } else if (os.platform() === "darwin") {
        mainWindow = new BrowserWindow({
            show: false,
            titleBarStyle: "hidden",
            center: true,
            resizable: true,
            webPreferences: {
                nodeIntegration: true,
                enableRemoteModule: true,
                contextIsolation: false,
            },
        })
    } else {
        // if (os.platform() === "linux") case
        mainWindow = new BrowserWindow({
            show: false,
            titleBarStyle: "hidden",
            center: true,
            resizable: true,
            maximizable: false,
            webPreferences: {
                nodeIntegration: true,
                enableRemoteModule: true,
                contextIsolation: false,
            },
            icon: path.join(__dirname, "/build/icon.png"),
            transparent: true,
        })
    }
    mainWindow.loadURL(`file://${__dirname}/app.html`)
    // mainWindow.webContents.openDevTools()

    // on client app FOCUS => protocol FOCUS
    // on client app MAXIMIZE => protocol MAXIMIZE
    // on client app MINIMIZE => protocol MINIMIZE
    // on client app CLOSE => protocol CLOSE
    // on protocol MAXIMIZE => client app MAXIMIZE
    // on protocol MINIMIZE => client app MINIMIZE
    // TWO-WAY COMMUNICATION (requires socket?)
    mainWindow.on('focus', () => {
        if (!showMainWindow) {
            protocolMinimized = false
        }
    })

    mainWindow.on('minimize', () => {
        if (!showMainWindow) {
            mainWindow.restore()
            protocolMinimized = !protocolMinimized
        }
    })

    // LOOK HERE: https://stackoverflow.com/questions/39841942/communicating-between-nodejs-and-c-using-node-ipc-and-unix-sockets

    // @TODO: Use 'ready-to-show' event
    //        https://github.com/electron/electron/blob/master/docs/api/browser-window.md#using-ready-to-show-event
    mainWindow.webContents.on("did-frame-finish-load", () => {
        // Checks if fractal:// was typed in, if it was, uses IPC to forward the URL
        // to the renderer thread
        if (os.platform() === "win32" && mainWindow) {
            // Keep only command line / deep linked arguments
            if (process.argv) {
                const url = process.argv.slice(1)
                mainWindow.webContents.send(
                    FractalIPC.CUSTOM_URL,
                    url.toString()
                )
            }
        } else if (customURL && mainWindow) {
            mainWindow.webContents.send(FractalIPC.CUSTOM_URL, customURL)
        }
        // Open dev tools in development
        if (
            process.env.NODE_ENV === "development" ||
            process.env.DEBUG_PROD === "true"
        ) {
            if (mainWindow) {
                // mainWindow.webContents.openDevTools()
            }
        }
        if (!mainWindow) {
            throw new Error('"mainWindow" is not defined')
        }
        if (!showMainWindow) {
            mainWindow.setOpacity(0.0)
            mainWindow.setIgnoreMouseEvents(true)
        }
        if (process.env.START_MINIMIZED) {
            mainWindow.minimize()
        } else {
            mainWindow.show()
            mainWindow.focus()
            mainWindow.maximize()
        }
        mainWindow.webContents.send(FractalIPC.UPDATE, updating)
    })

    // Listener to detect if the protocol was launched to hide the
    // app from the task tray
    const electron = require("electron")
    const ipc = electron.ipcMain

    ipc.on(FractalIPC.SHOW_MAIN_WINDOW, (event, argv) => {
        showMainWindow = argv
        if (showMainWindow && mainWindow) {
            mainWindow.maximize()
            mainWindow.show()
            mainWindow.focus()
            mainWindow.restore()
            mainWindow.setOpacity(1.0)
            mainWindow.setIgnoreMouseEvents(false)
        } else if (!showMainWindow && mainWindow) {
            mainWindow.setOpacity(0.0)
            mainWindow.setIgnoreMouseEvents(true)
        }
        event.returnValue = argv
    })

    ipc.on(FractalIPC.LOAD_BROWSER, (event, argv) => {
        const url = argv
        const win = new BrowserWindow({ width: 800, height: 600 })
        win.on("close", () => {
            if (win) {
                event.preventDefault()
            }
        })
        win.loadURL(url)
        win.show()
        event.returnValue = argv
    })

    ipc.on(FractalIPC.CLOSE_OTHER_WINDOWS, (event, argv) => {
        BrowserWindow.getAllWindows().forEach((win) => {
            if (win.id !== 1) {
                win.close()
            }
        })
        event.returnValue = argv
    })

    ipc.on(FractalIPC.FORCE_QUIT, () => {
        app.exit(0)
        app.quit()
    })

    mainWindow.on("close", (event) => {
        if (!showMainWindow) {
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
    app.quit()
})

app.on("quit", () => {
    protocolSocket.close()
})

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

// Autoupdater listeners, will fire if S3 app version is greater than current version

autoUpdater.autoDownload = false

autoUpdater.on("update-available", () => {
    updating = true
    if (mainWindow) {
        mainWindow.webContents.send(FractalIPC.UPDATE, updating)
    }
    autoUpdater.downloadUpdate()
})

autoUpdater.on("update-not-available", () => {
    updating = false
    if (mainWindow) {
        mainWindow.webContents.send(FractalIPC.UPDATE, updating)
    }
})

// autoUpdater.on("error", (_ev, err) => {
//     updating = false
//     if(mainWindow) {
//         mainWindow.webContents.send("error", err)
//     }

// })

autoUpdater.on("download-progress", (progressObj) => {
    if (mainWindow) {
        mainWindow.webContents.send(
            FractalIPC.DOWNLOAD_SPEED,
            progressObj.bytesPerSecond
        )
        mainWindow.webContents.send(
            FractalIPC.PERCENT_DOWNLOADED,
            progressObj.percent
        )
        mainWindow.webContents.send(
            FractalIPC.PERCENT_TRANSFERRED,
            progressObj.transferred
        )
        mainWindow.webContents.send(
            FractalIPC.TOTAL_DOWNLOADED,
            progressObj.total
        )
    }
})

autoUpdater.on("update-downloaded", () => {
    if (mainWindow) {
        mainWindow.webContents.send(FractalIPC.DOWNLOADED, true)
    }
    autoUpdater.quitAndInstall()
    updating = false
})
