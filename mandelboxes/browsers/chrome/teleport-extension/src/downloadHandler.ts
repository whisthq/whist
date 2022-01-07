// Listen for file download state changes.
const initDownloadHandler = () => {
    // Disable the downloads shelf at the bottom.
    chrome.downloads.setShelfEnabled(false)

    chrome.downloads.onChanged.addListener(
        (downloadDelta: chrome.downloads.DownloadDelta) => {
            if (downloadDelta?.state?.current !== "complete") {
                // If this is not a completed file, ignore.
                return
            }

            console.log(downloadDelta)

            // This file has completed, so let's act on the `DownloadItem`.
            chrome.downloads.search(
                {
                    id: downloadDelta.id,
                },
                (downloadItems: chrome.downloads.DownloadItem[]) => {
                    // Since we query by unique `id`, this is the only array element.
                    const downloadItem = downloadItems[0]
                    const filename = downloadItem.filename
                }
            )
        }
    )
}

initDownloadHandler()
