import { BrowserWindow } from "electron"
import { autoUpdater } from "electron-updater"
import { FractalIPC } from "../shared/types/ipc"

/**
 * Function that starts listeners for auto update
 * @param mainWindow renderer thread
 * @param updating
 */
export const initiateAutoUpdateListeners = (
    mainWindow: BrowserWindow | null = null,
    updating: boolean
) => {
    // Autoupdater listeners, will fire if S3 app version is greater than current version
    const { dialog } = require("electron")

    console.log("INITIATING AUTO LISTENERS")
    autoUpdater.autoDownload = false

    autoUpdater.on("update-available", () => {
        updating = true
        if (mainWindow) {
            mainWindow.webContents.send(FractalIPC.UPDATE, updating)
        }
        autoUpdater.downloadUpdate()
    })

    autoUpdater.on("update-not-available", () => {
        console.log("UPDATE NOT AVAILABLE")

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

    return updating
}
