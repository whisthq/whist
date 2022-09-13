import { Socket } from "socket.io-client"

// Listen for file state changes and propagate to the filesystem.
const initFileSyncHandler = (socket: Socket) => {
  // Disable the downloads shelf at the bottom.
  chrome.downloads.setShelfEnabled(false)

  // Listen for chrome downloads to notify teleport host about
  chrome.downloads.onCreated.addListener(
    (download: chrome.downloads.DownloadItem) => {
      socket.emit("download-started", download.finalUrl)
    }
  )
}

export { initFileSyncHandler }
