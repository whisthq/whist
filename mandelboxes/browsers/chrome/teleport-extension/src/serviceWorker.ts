// Listen for file state changes and propagate to the filesystem.
const initFileSyncHandler = () => {
  // Disable the downloads shelf at the bottom.
  chrome.downloads.setShelfEnabled(false)

  const hostPort = chrome.runtime.connectNative("whist_teleport_extension_host")
  hostPort.onMessage.addListener(console.log)

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

  // Run when popup signals an upload button click
  chrome.runtime.onMessage.addListener(
    (msg: string, sender: chrome.runtime.MessageSender, sendResponse: any) => {
      hostPort.postMessage({ fileUploadTrigger: true})
      sendResponse({});
    }
  )
}

// Try to cancel or undo a tab drag-out
const tryRestoreTabLocation = async (
  tabId: number,
  detachInfo: chrome.tabs.TabDetachInfo
) => {
  try {
    await chrome.tabs.move(tabId, {
      index: detachInfo.oldPosition,
      windowId: detachInfo.oldWindowId,
    })
  } catch (err) {
    if (
      err ==
      "Error: Tabs cannot be edited right now (user may be dragging a tab)."
    ) {
      await new Promise<void>((resolve) =>
        setTimeout(() => {
          tryRestoreTabLocation(tabId, detachInfo)
          resolve()
        }, 50)
      )
    }
  }
}

// Listen for tab drag-out detach events and instantly
// cancel/undo them.
const initTabDetachSuppressor = () => {
  chrome.tabs.onDetached.addListener(tryRestoreTabLocation)
}

initFileSyncHandler()
initTabDetachSuppressor()
