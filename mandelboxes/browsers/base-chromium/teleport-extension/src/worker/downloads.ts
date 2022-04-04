import {
  ContentScriptMessage,
  ContentScriptMessageType,
  NativeHostMessage,
  NativeHostMessageType,
} from "@app/constants/ipc"

// Listen for file state changes and propagate to the filesystem.
const initFileSyncHandler = (nativeHostPort: chrome.runtime.Port) => {
  // Disable the downloads shelf at the bottom.
  chrome.downloads.setShelfEnabled(false)

  // Listen for chrome downloads to notify teleport host about
  chrome.downloads.onChanged.addListener(
    (downloadDelta: chrome.downloads.DownloadDelta) => {
      if (downloadDelta?.state?.current !== "complete") return

      // This file has completed, so let's act on the `DownloadItem`.
      chrome.downloads.search(
        {
          id: downloadDelta.id,
        },
        (downloadItems: chrome.downloads.DownloadItem[]) => {
          chrome.tabs.query(
            { active: true, currentWindow: true },
            (tabs: chrome.tabs.Tab[]) => {
              chrome.tabs.sendMessage(tabs[0].id ?? 0, <ContentScriptMessage>{
                type: ContentScriptMessageType.DOWNLOAD_COMPLETE,
              })
            }
          )

          // Since we query by unique `id`, this is the only array element.
          const downloadItem = downloadItems[0]
          const filename = downloadItem.filename
          nativeHostPort.postMessage(<NativeHostMessage>{
            type: NativeHostMessageType.DOWNLOAD_COMPLETE,
            value: filename,
          })
        }
      )
    }
  )
}

export { initFileSyncHandler }
