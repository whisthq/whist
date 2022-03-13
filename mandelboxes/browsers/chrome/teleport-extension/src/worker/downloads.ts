// Message format for native application communication
interface HostMessage {
  action: string
}

// Listen for file state changes and propagate to the filesystem.
const initFileSyncHandler = () => {
  // Disable the downloads shelf at the bottom.
  chrome.downloads.setShelfEnabled(false)

  const hostPort = chrome.runtime.connectNative("whist_teleport_extension_host")

  // Listen for exit indication from host application
  hostPort.onMessage.addListener((msg: HostMessage) => {
    if (msg.action == "exit") {
      hostPort.disconnect()
    }
  })

  // Listen for chrome downloads to notify teleport host about
  chrome.downloads.onChanged.addListener(
    (downloadDelta: chrome.downloads.DownloadDelta) => {
      if (downloadDelta?.state?.current !== "complete") {
        // If this is not a completed file, ignore.
        return
      }

      // This file has completed, so let's act on the `DownloadItem`.
      chrome.downloads.search(
        {
          id: downloadDelta.id,
        },
        (downloadItems: chrome.downloads.DownloadItem[]) => {
          // Since we query by unique `id`, this is the only array element.
          const downloadItem = downloadItems[0]
          const filename = downloadItem.filename
          hostPort.postMessage({ downloadStatus: "complete", filename })
        }
      )
    }
  )

  // Listen for popup upload button click
  chrome.runtime.onMessage.addListener(
    (msg: string, sender: chrome.runtime.MessageSender, sendResponse: any) => {
      hostPort.postMessage({ fileUploadTrigger: true })
      sendResponse({})
    }
  )
}

export { initFileSyncHandler }
