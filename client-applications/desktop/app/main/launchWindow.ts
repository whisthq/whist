import path from "path"
import { app, BrowserWindow } from "electron"
import { autoUpdater } from "electron-updater"

import { FractalIPC } from "../shared/types/ipc"

/**
 * Function used to launch the renderer thread to display the client app
 * @param mainWindow BrowserWindow instance to launch renderer thread
 * @param customURL detects whether fractal:// has been typed into a browser
 * @param updating detects whether there's an auto-update
 * @param showMainWindow toggles whether to show the elecron main window
 *
 * Returns: new BrowserWindow instance
 */
export const createWindow = async (
    mainWindow: BrowserWindow | null = null,
    customURL: string | null = null,
    updating: boolean,
    showMainWindow: boolean
) => {
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
            show: true,
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
    mainWindow.loadURL(`file://${__dirname}/../app.html`)
    // mainWindow.webContents.openDevTools()

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
        if (showMainWindow) {
            if (process.env.START_MINIMIZED) {
                mainWindow.minimize()
            } else {
                mainWindow.show()
                mainWindow.focus()
                mainWindow.maximize()
            }
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
            if (app && app.dock) {
                app.dock.show()
            }
        } else if (!showMainWindow && mainWindow) {
            // mainWindow.hide()
            if (app && app.dock) {
                app.dock.hide()
            }
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

    mainWindow.on("maximize", () => {})

    mainWindow.on("minimize", () => {})

    if (process.env.NODE_ENV === "development") {
        // Skip autoupdate check
    } else {
        autoUpdater.checkForUpdates()
    }

    return mainWindow
}
