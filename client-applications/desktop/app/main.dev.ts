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
import { unlink } from "fs";
import { createServer, Server, Socket } from "net"
import { Mutex } from "async-mutex"
import * as Sentry from "@sentry/electron"
import { FractalIPC } from "./shared/types/ipc"
import { FractalDirectory } from "./shared/types/client"

if (process.env.NODE_ENV === "production") {
    Sentry.init({
        dsn:
            "https://5b0accb25f3341d280bb76f08775efe1@o400459.ingest.sentry.io/5412323",
        release: `client-applications@${app.getVersion()}`,
    })
}

// This is the window where the renderer thread will render our React app
let mainWindow: BrowserWindow | null = null
// Detects whether there's an auto-update
let updating = false
// Detects whether fractal:// has been typed into a browser
let customURL: string | null = null
// Toggles whether to show the Electron main window
let showMainWindow = false
// Whether FractalClient is currently minimized
let protocolMinimized = false
// Server for socket communication with FractalClient
let protocolSocketServer: Server | null = null
// Unix IPC socket address for communication with FractalClient
let socketPath = "fractal-client.sock"
// Current socket data - to deal with the fact that the named pipe is in byte mode, not message mode.
//    Messages from the client are not kept separate by default, and should be recorded as a stream.
let socketReceivedData = ""
// Mutex to keep the received socket data in order
const socketMutex = new Mutex()
// Inventory of client sockets: will be two on Windows, 1 elsewhere
const clientSockets: Socket[] = []
// Indicates whether a TOGGLE message is being processed by the protocol
let toggling = false

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

// Function to create the browser window
const createWindow = async () => {
    const os = require("os")
    if (os.platform() === "win32") {
        mainWindow = new BrowserWindow({
            // show: false, // Windows does not show the taskbar icon if the window is hidden
            opacity: 0.0,
            frame: false,
            center: true,
            resizable: true,
            webPreferences: {
                nodeIntegration: true,
                enableRemoteModule: true,
            },
        })
        mainWindow.setIgnoreMouseEvents(true)
        socketPath = `\\\\.\\pipe\\${socketPath}`
    } else if (os.platform() === "darwin") {
        mainWindow = new BrowserWindow({
            show: false,
            titleBarStyle: "hidden",
            center: true,
            resizable: true,
            webPreferences: {
                nodeIntegration: true,
                enableRemoteModule: true,
            },
        })
        socketPath = require("path").join(
            FractalDirectory.getRootDirectory(),
            "protocol-build/desktop",
            socketPath
        )
        unlink(socketPath, (err) => {
            if (err) Sentry.captureException(err)
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
            },
            icon: path.join(__dirname, "/build/icon.png"),
            transparent: true,
        })
        socketPath = require("path").join(
            FractalDirectory.getRootDirectory(),
            "protocol-build/desktop",
            socketPath
        )
        unlink(socketPath, (err) => {
            if (err) Sentry.captureException(err)
        })
    }
    mainWindow.loadURL(`file://${__dirname}/app.html`)
    // mainWindow.webContents.openDevTools()

    // Create a socket server to send and receive message to the protocol
    protocolSocketServer = createServer((socket) => {
        // When a client connects, store it in inventory
        clientSockets.push(socket)

        // When a client disconnects, remove it from inventory
        socket.on("end", () => {
            const socketIndex = clientSockets.indexOf(socket)
            if (socketIndex >= 0) {
                delete clientSockets[socketIndex]
            }
        })

        // Set the socket to be readable and writable to the same stream
        socket.pipe(socket)

        // Handle data received on this socket
        socket.on("data", (data) => {
            const messagePairs: { sender: string; message: string }[] = []
            socketMutex
                .runExclusive(() => {
                    // Append received data to the received data record
                    socketReceivedData += data.toString()
                    // Parse each message, separated by '\n'
                    while (socketReceivedData.includes("\n")) {
                        const parts = socketReceivedData
                            .split("\n")[0]
                            .split(":")
                        messagePairs.push({
                            sender: parts[0],
                            message: parts[1],
                        })
                        socketReceivedData = socketReceivedData.substring(
                            socketReceivedData.indexOf("\n") + 1
                        )
                    }
                })
                .then(() => {
                    if (mainWindow) {
                        messagePairs.forEach((pair) => {
                            const sender = pair.sender
                            const message = pair.message
                            if (sender === "client") {
                                if (message === "MINIMIZE") {
                                    if (mainWindow) {
                                        mainWindow.minimize()
                                    }
                                    protocolMinimized = true
                                } else if (message === "FOCUS") {
                                    if (mainWindow) {
                                        mainWindow.minimize()
                                    }
                                    protocolMinimized = false
                                } else if (message === "UNFOCUS") {
                                    // This is currently unused, but left as an option
                                    //    because it is a message sent by the protocol
                                } else if (message === "TOGGLE_ACK") {
                                    // When the protocol has acknowledged the TOGGLE communication sequence
                                    //    beginning, then blur the client app window to trigger a blur event
                                    toggling = true
                                    if (mainWindow) {
                                        mainWindow.blur()
                                    }
                                } else if (message === "QUIT") {
                                    if (protocolSocketServer) {
                                        protocolSocketServer.close()
                                    }
                                    app.quit()
                                }
                            }
                        })
                    }
                    return null
                })
                .catch((err) => {
                    if (err) Sentry.captureException(err)
                })
        })
        // These errors are usually thrown as a result of the other end disconnecting
        socket.on("error", (err) => {
            console.log(`Socket error (likely ignorable): ${err}`)
            clientSockets.forEach((connectedSocket) => {
                connectedSocket.end()
            })
        })
    })

    // These errors are usually thrown as a result of the other end disconnecting
    protocolSocketServer.on("error", (err) => {
        console.log(`Socket server error (likely ignorable): error ${err}`)
        if (protocolSocketServer) {
            protocolSocketServer.close()
        }
    })

    // Start the server to listen for connections
    protocolSocketServer.listen(socketPath, () => {
        if (os.platform() === "win32") {
            if (mainWindow) {
                // This event is triggered when the taskbar icon is pressed
                mainWindow.on("focus", () => {
                    let message = ""

                    if (protocolMinimized) {
                        // If the protocol is minimized, then the protocol should focus
                        message = "server:FOCUS"
                        protocolMinimized = false
                    } else {
                        // If the protocol window is open, then begin a TOGGLE communication sequence
                        message = "server:TOGGLE"
                    }

                    if (message !== "") {
                        clientSockets.forEach((socket) => {
                            socket.write(message)
                        })
                    }
                })

                // If the window has been blurred during a TOGGLE communication sequence, then indicate to
                //    the protocol that the TOGGLE communication and activity sequence is complete.
                mainWindow.on("blur", () => {
                    if (toggling) {
                        clientSockets.forEach((writeSocket) => {
                            writeSocket.write("server:TOGGLE_COMPLETE")
                        })
                    }
                    toggling = false
                })
            }
        } else if (os.platform() === "darwin") {
            // This event is triggered when the dock icon is pressed, indicating that
            //    the protocol window should be focused
            app.on("activate", () => {
                clientSockets.forEach((socket) => {
                    socket.write("server:FOCUS")
                })
            })
        }

        // When the app is about to quit, send a message to the protocol that it should quit
        app.on("will-quit", () => {
            clientSockets.forEach((socket) => {
                socket.write("server:QUIT")
                socket.end()
            })
        })
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
            // If the window is to be hidden, but we're on Windows, then make the window
            //    transparent and set mouse events to pass through the window.
            if (os.platform() === "win32") {
                mainWindow.setOpacity(0.0)
                mainWindow.setIgnoreMouseEvents(true)
                mainWindow.minimize()
            }
        } else if (process.env.START_MINIMIZED) {
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
            // Make sure the window is not transparent and that it captures mouse events
            mainWindow.setOpacity(1.0)
            mainWindow.setIgnoreMouseEvents(false)
        } else if (!showMainWindow && mainWindow) {
            // To keep the icon on the taskbar in Windows, don't hide the window
            //    - just make it transparent
            if (os.platform() === "win32") {
                mainWindow.setOpacity(0.0)
                mainWindow.setIgnoreMouseEvents(true)
                mainWindow.minimize()
            } else {
                mainWindow.hide()
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
    if (protocolSocketServer) {
        protocolSocketServer.close()
    }
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
