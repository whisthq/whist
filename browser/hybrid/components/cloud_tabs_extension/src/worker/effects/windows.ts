import { serverWindowCreated, windowFocused } from "@app/worker/events/windows"

serverWindowCreated.subscribe(
  ([window]: [chrome.windows.Window & { url: string }]) => {
    chrome.windows.create({
      url: window.url,
      incognito: window.incognito,
      state: window.state,
      type: window.type,
    } as any)
  }
)

windowFocused.subscribe((windowId: number) => {
  chrome.tabs.query(
    {
      active: true,
      windowId,
    },
    (tabs) => {
      const tabId = tabs[0]?.id
      ;(chrome as any).whist.broadcastWhistMessage(
        JSON.stringify({
          type: "WINDOW_FOCUSED",
          value: {
            windowId,
            tabId,
          },
        })
      )
    }
  )
})
