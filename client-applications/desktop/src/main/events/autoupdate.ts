import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

export const eventUpdateAvailable = fromEvent(
    autoUpdater as EventEmitter,
    "update-available"
)

export const eventUpdateNotAvailable = fromEvent(
    autoUpdater as EventEmitter,
    "update-not-available"
)

export const eventDownloadProgress = fromEvent(
    autoUpdater as EventEmitter,
    "download-progress"
)

export const eventUpdateDownloaded = fromEvent(
    autoUpdater as EventEmitter,
    "update-downloaded"
)

// export const listenAutoUpdate: Event = (setState) => {
//     // Autoupdater listeners, will fire if S3 app version is greater than current version

//     autoUpdater.autoDownload = false

//     autoUpdater.on("update-available", () => {
//         setState({ needsUpdate: true })
//         autoUpdater.downloadUpdate()
//     })

//     autoUpdater.on("update-not-available", () => {
//         setState({ needsUpdate: false })
//     })

//     autoUpdater.on("download-progress", (progressObj) => {
//         setState({ updateInfo: JSON.stringify(progressObj) })
//     })

//     autoUpdater.on("update-downloaded", () => {
//         autoUpdater.quitAndInstall()
//     })
// }
