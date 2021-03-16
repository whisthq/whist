import { BrowserWindow } from "electron"
import { autoUpdater } from "electron-updater"
import { FractalIPC } from "../shared/types/ipc"

/**
 * Function that starts listeners for auto update
 * @param mainWindow renderer thread
 * @param updating
 */
export const initiateAutoUpdateListeners = (
    mainWindow: BrowserWindow | null = null
) => {
    // Autoupdater listeners, will fire if S3 app version is greater than current version

    autoUpdater.autoDownload = false

    autoUpdater.on("update-available", () => {
        global.updateStatus = true

        if (mainWindow) {
            mainWindow.webContents.send(FractalIPC.UPDATE, global.updateStatus)
        }
        autoUpdater.downloadUpdate()
    })

    autoUpdater.on("update-not-available", () => {
        global.updateStatus = false
        if (mainWindow) {
            mainWindow.webContents.send(FractalIPC.UPDATE, global.updateStatus)
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
        global.updateStatus = false
    })
}
