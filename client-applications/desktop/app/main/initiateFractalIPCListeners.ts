import { app, BrowserWindow } from "electron"
import { ChildProcess } from "child_process"
import { FractalIPC } from "../shared/types/ipc"
import { launchProtocol, writeStream } from "../shared/utils/files/exec"
import { protocolOnStart, protocolOnExit } from "./launchProtocol"
import LoadingMessage from "../pages/launcher/constants/loadingMessages"

/**
 * Initializes ipc listeners upon window creation
 * @param mainWindow browser window
 * @param showMainWindow determines whether to display the window or not
 */
export const initiateFractalIPCListeners = (
    mainWindow: BrowserWindow | null = null,
    showMainWindow: boolean
) => {
    const electron = require("electron")
    const ipc = electron.ipcMain

    let protocol: ChildProcess
    let userID: string
    let protocolLaunched: number
    let createContainerRequestSent: number

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
        console.log("QUITTING IN LISTENERS")
        // app.exit(0)
        // app.quit()
    })

    ipc.on(FractalIPC.LAUNCH_PROTOCOL, (event, argv) => {
        protocolLaunched = Date.now()
        const launchThread = async () => {
            protocol = await launchProtocol(
                protocolOnStart,
                protocolOnExit,
                userID,
                protocolLaunched,
                createContainerRequestSent
            )
        }

        launchThread()
        event.returnValue = argv
    })

    ipc.on(FractalIPC.SEND_CONTAINER, (event, argv) => {
        const container = argv
        console.log("SENDING TO PROTOCOL", container)
        const portInfo = `32262:${container.port32262}.32263:${container.port32263}.32273:${container.port32273}`
        writeStream(protocol, `ports?${portInfo}`)
        writeStream(protocol, `private-key?${container.secretKey}`)
        writeStream(protocol, `ip?${container.publicIP}`)
        writeStream(protocol, `finished?0`)
        createContainerRequestSent = Date.now()
        console.log("DESTROYING BROWSER WINDOW")
        // mainWindow?.destroy()
        console.log("DESTROYED BROWSER WINDOW")
        event.returnValue = argv
    })

    ipc.on(FractalIPC.PENDING_PROTOCOL, (event, argv) => {
        writeStream(protocol, `loading?${LoadingMessage.PENDING}`)
        event.returnValue = argv
    })

    ipc.on(FractalIPC.KILL_PROTOCOL, (event, argv) => {
        console.log("KILLING PROTOCOL")
        writeStream(protocol, "kill?0")
        protocol.kill("SIGINT")
        event.returnValue = argv
    })
}
