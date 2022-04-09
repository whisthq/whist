import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"
import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

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
const createTab = (url: string) => {
  chrome.tabs.create({
    url: url,
    active: true,
  })
}

// Switch currently-active tab to match the requested tab
const activateTab = (tabId: number) => {
  chrome.tabs.update(tabId, { active: true })
}

// Listen for tab drag-out detach events and instantly
// cancel/undo them.
const initTabDetachSuppressor = () => {
  chrome.tabs.onDetached.addListener(tryRestoreTabLocation)
}

const initCreateNewTabHandler = (nativeHostPort: chrome.runtime.Port) => {
  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (
      msg.type === NativeHostMessageType.CREATE_NEW_TAB &&
      msg.value?.url !== undefined
    )
      createTab(msg.value.url)
  })
}

const initActivateTabHandler = (nativeHostPort: chrome.runtime.Port) => {
  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (
      msg.type === NativeHostMessageType.ACTIVATE_TAB &&
      msg.value?.id !== undefined
    )
      activateTab(msg.value.id)
  })
}

const initTabUpdateListener = () => {
  chrome.tabs.onUpdated.addListener((tabId, changeInfo) => {
    // read changeInfo data and do something with it
    // like send the new url to contentscripts.js
    if (changeInfo.url) {
      chrome.tabs.sendMessage(tabId, <ContentScriptMessage>{
        type: ContentScriptMessageType.TAB_UPDATED,
      })
    }
  })
}

export {
  initTabDetachSuppressor,
  initCreateNewTabHandler,
  initActivateTabHandler,
  initTabUpdateListener,
}
