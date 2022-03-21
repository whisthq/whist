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
  tabId: number,
  newtabUrl: string
) => {
  try {
    await chrome.tabs.create({
      openerTabId: tabId,
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
          createNewTab(tabId, newtabUrl)
          resolve()
        }, 50)
      )
    }
  }
}

// Switch currently-active tab to match the requested tab
const switchActiveTab = async (
  tabId: number,
  tabToFocusUrl: string
) => {
  try {
    var found: boolean = false
    var desiredTabId: number = -1
    // Loop over all tabs to find the one we want to switch to
    chrome.tabs.query({}, function (tabs: chrome.tabs.Tab[]) {
      for (var i = 0; i < tabs.length; i++) {
        if (tabs[i].url.search(tabToFocusUrl) > -1) {
          found = true
          desiredTabId = tabs[i].id
        }
      }
      if (found) {
        chrome.tabs.update(desiredTabId, { active: true })
      } else {
        throw "Error: Could not find tab to switch to."
      }
    })
  } catch (err) {
    if (err == "Error: Failed to switch to the tab for the provided URL.") {
      await new Promise<void>((resolve) =>
        setTimeout(() => {
          switchActiveTab(tabId, tabToFocusUrl)
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

export {
  initTabDetachSuppressor,
  initTabCreationHandler,
  initTabSwitchingHandler,
}
