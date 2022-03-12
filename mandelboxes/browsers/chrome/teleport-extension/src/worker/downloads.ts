// Message format for native application communication
interface HostMessage {
  action: string
}

/*******************************************************************
 * File synchronization
 *******************************************************************/

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

  // Listen for Chrome downloads to notify teleport host about
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
}

/*******************************************************************
 * Tab Management
 *******************************************************************/

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

// Switch focused tab to match the requested URL
const createNewTab = async (
  newtabUrl: string
) => {
  try {
    await chrome.tabs.create({
      active: true, // Switch to the new tab
      url: newtabUrl,
    })
  } catch (err) {
    if (
      err ==
      "Error: Failed to create and switch to the new tab for the provided URL."
    ) {
      await new Promise<void>((resolve) =>
        setTimeout(() => {
          createNewTab(newtabUrl)
          resolve()
        }, 50)
      )
    }
  }
}

// Switch currently-active tab to match the requested tab
const switchActiveTab = async (
  tabToFocusUrl: string,
) => {
  try {
    var found = false;
    var tabId;
    // Loop over all tabs to find the one we want to switch to
    chrome.tabs.query({}, function (tabs) {
      for (var i = 0; i < tabs.length; i++) {
          if (tabs[i].url.search(tabToFocusUrl) > -1) {
              found = true;
              tabId = tabs[i].id;
          }
      }
      if (found) {
        chrome.tabs.update(tabId, {active: true});
      } else {
        throw "Error: Could not find tab to switch to."
      }
    })
  } catch (err) {
    if (
      err ==
      "Error: Failed to switch to the tab for the provided URL."
    ) {
      await new Promise<void>((resolve) =>
        setTimeout(() => {
          switchActiveTab(tabToFocusURL)
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

// Listen for tab creation events, create a new tab to
// the requested URL, and switch focus to it
const initTabCreationHandler = () => {
  chrome.tabs.onCreated.addListener(createNewTab)
}

// Listen for tab switching events and switch the focused
// tab to the requested URL
const initTabSwitchingHandler = () => {
  chrome.tabs.onUpdated.addListener(switchActiveTab)
}

/*******************************************************************
 * Entrypoint
 *******************************************************************/

initFileSyncHandler()
initTabDetachSuppressor()
initTabCreationHandler()
initTabSwitchingHandler()
