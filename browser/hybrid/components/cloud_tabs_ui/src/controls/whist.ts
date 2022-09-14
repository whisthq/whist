import { getTabId } from "./session"
import { state } from "../shared/state"

let whistParameters: string | null = null

const initializeWhistTagHandler = () => {
  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (parsed.type === "MANDELBOX_INFO") {
      // Keep the whist element in-focus
      whistParameters = JSON.stringify(parsed.value)
    }
  })
}

const initializeRequestMandelbox = () => {
  const whistTag: any = document.querySelector("whist")
  let mandelboxRequested = false

  setInterval(() => {
    if (
      !whistTag.isWhistConnected() &&
      !mandelboxRequested &&
      state.wasConnected
    ) {
      mandelboxRequested = true
      ;(chrome as any).whist.broadcastWhistMessage(
        JSON.stringify({
          type: "MANDELBOX_NEEDED",
        })
      )
    }

    if(!whistTag.isWhistConnected()) {
      state.wasConnected = false 
    }
    
    if (whistTag.isWhistConnected()) {
      state.wasConnected = true
      mandelboxRequested = false
    }
  }, 100)
}

const initializeWhistFreezeHandler = () => {
  const whistTag: any = document.querySelector("whist")

  chrome.windows.onFocusChanged.addListener((windowId: number) => {
    if (windowId < 0 || windowId !== sessionStorage.getNumber("windowId")) {
      whistTag.freeze()
    } else {
      chrome.tabs.query(
        {
          active: true,
          windowId,
        },
        (tabs) => {
          const tabId = tabs[0]?.id
          if (tabId !== sessionStorage.getNumber("tabId")) whistTag.freeze()
        }
      )
    }
  })

  chrome.tabs.onActivated.addListener((activeInfo: { tabId: number }) => {
    if (activeInfo.tabId !== sessionStorage.getNumber("tabId"))
      whistTag.freeze()
  })
}

const initializeWhistThawHandler = () => {
  const whistTag: any = document.querySelector("whist")

  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (
      (parsed.type === "ACTIVATE_TAB" || parsed.type === "UPDATE_TAB") &&
      parsed.value.id === getTabId()
    ) {
      if (whistParameters !== null && !whistTag.isWhistConnected()) {
        whistTag.focus()
        whistTag.whistConnect(whistParameters)
      }
      // We delay by 100ms because this is the amount of time we expect the protocol to lag behind
      // the socket.io message, since the protocol has extra video encode/decode latency.
      // Lower values were tested and they led to flashes of other tabs being seen.
      setTimeout(() => {
        whistTag.thaw()
      }, 100)
    }
  })
}

export {
  initializeWhistTagHandler,
  initializeWhistFreezeHandler,
  initializeWhistThawHandler,
  initializeRequestMandelbox
}
